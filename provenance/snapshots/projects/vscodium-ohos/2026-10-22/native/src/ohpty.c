// ohpty: minimal node-pty-compatible native module for HarmonyOS.
// Implements the subset of node-pty's native interface that VS Code's
// lib/unixTerminal.js uses: fork / resize / process (+ open stub).
//
// - forkpty + execl("/bin/sh"-style execvp) per DevPython-OH terminal_context::Fork
// - ptsname() is NOT used (returns NULL on OHOS, crashes node-pty)
// - plain C N-API only (no node-addon-api, no C++ exceptions, no std::thread)
// - napi symbols resolve via the napi-dyn dlsym-forwarding layer

#include <node_api.h>
#include <pty.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <stdio.h>
extern char **environ;
static napi_value napi_value_string_create(napi_env env, const char *s);

static void ohos_trace(const char *msg) {
  int fd = open("/data/storage/el2/base/files/ohpty-trace.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
  if (fd >= 0) { write(fd, msg, strlen(msg)); write(fd, "\n", 1); close(fd); }
}

// ---- exit watcher ----

typedef struct {
  pid_t pid;
  napi_threadsafe_function tsfn;
} ExitWatch;

typedef struct {
  int exit_code;
  int signal_code;
  napi_threadsafe_function tsfn;
} ExitEvent;

static void exit_cb_dispatch(napi_env env, napi_value cb, void *context, void *data) {
  (void)context;
  ExitEvent *ev = (ExitEvent *)data;
  napi_value argv[2];
  napi_create_int32(env, ev->exit_code, &argv[0]);
  napi_create_int32(env, ev->signal_code, &argv[1]);
  napi_value global;
  napi_get_global(env, &global);
  napi_call_function(env, global, cb, 2, argv, NULL);
  napi_release_threadsafe_function(ev->tsfn, napi_tsfn_release);
  free(ev);
}

static void *watch_thread(void *arg) {
  ExitWatch *w = (ExitWatch *)arg;
  int status = 0;
  while (waitpid(w->pid, &status, 0) == -1 && errno == EINTR) { }
  ExitEvent *ev = (ExitEvent *)calloc(1, sizeof(ExitEvent));
  ev->exit_code = 0;
  ev->signal_code = 0;
  ev->tsfn = w->tsfn;
  if (WIFEXITED(status)) ev->exit_code = WEXITSTATUS(status);
  if (WIFSIGNALED(status)) ev->signal_code = WTERMSIG(status);
  napi_call_threadsafe_function(w->tsfn, ev, napi_tsfn_blocking);
  free(w);
  return NULL;
}

// ---- helpers ----

static napi_value throw_err(napi_env env, const char *msg) {
  napi_throw_error(env, NULL, msg);
  return NULL;
}

static int set_nonblock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) return -1;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// fork(file, args, env, cwd, cols, rows, uid, gid, utf8, helperPath, onexit)
static napi_value OhFork(napi_env env, napi_callback_info info) {
  ohos_trace("OhFork: enter");
  size_t argc = 11;
  napi_value argv_js[11];
  if (napi_get_cb_info(env, info, &argc, argv_js, NULL, NULL) != napi_ok || argc < 11) {
    return throw_err(env, "Usage: pty.fork(file, args, env, cwd, cols, rows, uid, gid, utf8, helperPath, onexit)");
  }

  // file
  char file[256];
  size_t len;
  if (napi_get_value_string_utf8(env, argv_js[0], file, sizeof file, &len) != napi_ok) return throw_err(env, "bad file");

  // args array
  uint32_t nargs = 0;
  napi_get_array_length(env, argv_js[1], &nargs);
  char **args = (char **)calloc(nargs + 2, sizeof(char *));
  args[0] = strdup(file);
  for (uint32_t i = 0; i < nargs; i++) {
    napi_value v;
    napi_get_element(env, argv_js[1], i, &v);
    char tmp[1024];
    napi_get_value_string_utf8(env, v, tmp, sizeof tmp, &len);
    args[i + 1] = strdup(tmp);
  }

  // env object -> KEY=VALUE strings; missing/invalid env arg => inherit parent environ
  napi_value env_arg = argv_js[2];
  napi_valuetype env_type;
  bool use_custom_env = napi_typeof(env, env_arg, &env_type) == napi_ok && env_type == napi_object;
  {
    char tb[128];
    snprintf(tb, sizeof tb, "OhFork: env_arg type=%d use_custom=%d", (int)env_type, (int)use_custom_env);
    ohos_trace(tb);
  }
  char **envp = NULL;
  if (use_custom_env) {
    napi_value names;
    napi_get_property_names(env, env_arg, &names);
    uint32_t nenv = 0;
    if (names) napi_get_array_length(env, names, &nenv);
    if (nenv == 0) {
      use_custom_env = false; // empty object: behave like inherit
    } else {
      envp = (char **)calloc(nenv + 1, sizeof(char *));
      for (uint32_t i = 0; i < nenv; i++) {
        napi_value k, v;
        napi_get_element(env, names, i, &k);
        napi_get_property(env, env_arg, k, &v);
        char kb[256], vb[2048];
        napi_get_value_string_utf8(env, k, kb, sizeof kb, &len);
        napi_get_value_string_utf8(env, v, vb, sizeof vb, &len);
        char *pair = (char *)malloc(strlen(kb) + strlen(vb) + 2);
        sprintf(pair, "%s=%s", kb, vb);
        envp[i] = pair;
        // trace key envs
        if (strcmp(kb, "TERM") == 0 || strcmp(kb, "TMPDIR") == 0 || strcmp(kb, "HOME") == 0 || strcmp(kb, "PATH") == 0) {
          char tb2[300];
          snprintf(tb2, sizeof tb2, "OhFork env: %s=%.200s", kb, vb);
          ohos_trace(tb2);
        }
      }
      char tb3[64];
      snprintf(tb3, sizeof tb3, "OhFork: envp entries=%u", nenv);
      ohos_trace(tb3);

      // Force-inject OHOS defaults for vars VS Code failed to supply.
      static const char *defaults[][2] = {
        { "HOME", "/storage/Users/currentUser" },
        { "TERM", "xterm-256color" },
        { "TMPDIR", "/data/storage/el2/base/files/vscode-portable/tmp" },
        { "ENV", "/storage/Users/currentUser/.zshrc" },
        { "PATH", "/storage/Users/currentUser/apps/ohos-aarch64-install/bin:/storage/Users/currentUser/.harmonybrew/bin:/storage/Users/currentUser/.harmonybrew/sbin:/usr/local/bin:/data/app/bin:/data/service/hnp/bin:/usr/bin:/vendor/bin:/bin:/storage/Users/currentUser/apps/fastfetch/bin:/storage/Users/currentUser/apps/ohfetch/bin" },
      };
      for (size_t d = 0; d < sizeof(defaults) / sizeof(defaults[0]); d++) {
        const char *key = defaults[d][0];
        size_t kl = strlen(key);
        bool present = false;
        for (uint32_t i = 0; i < nenv; i++) {
          if (strncmp(envp[i], key, kl) == 0 && envp[i][kl] == '=') { present = true; break; }
        }
        if (!present) {
          char *pair = (char *)malloc(kl + strlen(defaults[d][1]) + 2);
          sprintf(pair, "%s=%s", key, defaults[d][1]);
          envp = (char **)realloc(envp, (nenv + 2) * sizeof(char *));
          envp[nenv++] = pair;
          envp[nenv] = NULL;
          char tb4[80];
          snprintf(tb4, sizeof tb4, "OhFork: injected default %s", key);
          ohos_trace(tb4);
        }
      }
    }
  }

  char cwd[512];
  cwd[0] = 0;
  napi_get_value_string_utf8(env, argv_js[3], cwd, sizeof cwd, &len);

  int32_t cols = 80, rows = 24;
  napi_get_value_int32(env, argv_js[4], &cols);
  napi_get_value_int32(env, argv_js[5], &rows);

  ohos_trace("OhFork: args parsed");

  struct winsize ws = {};
  ws.ws_col = (unsigned short)cols;
  ws.ws_row = (unsigned short)rows;

  int master = -1;
  pid_t pid = forkpty(&master, NULL, NULL, &ws);
  if (pid == 0) {
    // child: reset signal handlers
    struct sigaction sa = {};
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    for (int i = 1; i < NSIG; i++) sigaction(i, &sa, NULL);
    if (cwd[0]) {
      if (chdir(cwd) == -1) { perror("chdir"); _exit(1); }
    }
    if (use_custom_env) {
      char **old = environ;
      environ = envp;
      execvp(args[0], args);
      environ = old;
    } else {
      execvp(args[0], args);
    }
    perror("execvp");
    _exit(1);
  }
  if (pid == -1) {
    return throw_err(env, "forkpty(3) failed");
  }
  if (set_nonblock(master) == -1) {
    return throw_err(env, "Could not set master fd to nonblocking");
  }
  ohos_trace("OhFork: forkpty ok");

  // result object { fd, pid, pty }
  napi_value obj, vfd, vpid, vpty;
  napi_create_object(env, &obj);
  napi_create_int32(env, master, &vfd);
  napi_create_int32(env, pid, &vpid);
  napi_create_string_utf8(env, "", 0, &vpty); // ptsname() is NULL on OHOS; unused by VS Code
  napi_set_property(env, obj, napi_value_string_create(env, "fd"), vfd);
  napi_set_property(env, obj, napi_value_string_create(env, "pid"), vpid);
  napi_set_property(env, obj, napi_value_string_create(env, "pty"), vpty);

  // exit watcher via threadsafe function
  napi_value resource_name;
  napi_create_string_utf8(env, "ohpty_exit", NAPI_AUTO_LENGTH, &resource_name);
  ExitWatch *w = (ExitWatch *)calloc(1, sizeof(ExitWatch));
  w->pid = pid;
  if (napi_create_threadsafe_function(env, argv_js[10], NULL, resource_name, 0, 1, NULL, NULL, NULL, exit_cb_dispatch, &w->tsfn) == napi_ok) {
    pthread_t th;
    if (pthread_create(&th, NULL, watch_thread, w) == 0) {
      pthread_detach(th);
    }
  } else {
    free(w);
  }
  ohos_trace("OhFork: done");
  return obj;
}

static napi_value napi_value_string_create(napi_env env, const char *s) {
  napi_value v;
  napi_create_string_utf8(env, s, NAPI_AUTO_LENGTH, &v);
  return v;
}

// resize(fd, cols, rows)
static napi_value OhResize(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value argv_js[3];
  napi_get_cb_info(env, info, &argc, argv_js, NULL, NULL);
  int32_t fd, cols, rows;
  napi_get_value_int32(env, argv_js[0], &fd);
  napi_get_value_int32(env, argv_js[1], &cols);
  napi_get_value_int32(env, argv_js[2], &rows);
  struct winsize ws = {};
  ws.ws_col = (unsigned short)cols;
  ws.ws_row = (unsigned short)rows;
  if (ioctl(fd, TIOCSWINSZ, &ws) == -1) {
    return throw_err(env, "ioctl(2) failed");
  }
  return NULL;
}

// process(fd) / process(fd, pty) -> process name (best effort)
static napi_value OhGetProc(napi_env env, napi_callback_info info) {
  (void)info;
  return napi_value_string_create(env, "");
}

static napi_value OhOpen(napi_env env, napi_callback_info info) {
  (void)info;
  return throw_err(env, "pty.open not supported on ohos");
}

static napi_value Init(napi_env env, napi_value exports) {
  napi_value fn;
  napi_create_function(env, "fork", NAPI_AUTO_LENGTH, OhFork, NULL, &fn);
  napi_set_property(env, exports, napi_value_string_create(env, "fork"), fn);
  napi_create_function(env, "open", NAPI_AUTO_LENGTH, OhOpen, NULL, &fn);
  napi_set_property(env, exports, napi_value_string_create(env, "open"), fn);
  napi_create_function(env, "resize", NAPI_AUTO_LENGTH, OhResize, NULL, &fn);
  napi_set_property(env, exports, napi_value_string_create(env, "resize"), fn);
  napi_create_function(env, "process", NAPI_AUTO_LENGTH, OhGetProc, NULL, &fn);
  napi_set_property(env, exports, napi_value_string_create(env, "process"), fn);
  return exports;
}

NAPI_MODULE(pty, Init)

// ============================================================================
// pyqtlauncher.cpp — OpenShot Video Editor (PyQt5) launcher for HarmonyOS
//
// 架构（pyqt-fluent-widgets demo 真机验证版，_staging/python/SUMMARY.md §3.2）：
//   QAbility (ArkTS) → libqohos.so (QPA) → dlsym("main") → 本文件
//     → CPython 3.12 (DevPython-OH, HAP 内 libs/arm64/Python)
//       → resfile/src/launch.py → PyQt5 → OpenShotApp
//
// launcher 坑清单（demo 9 条的真机教训，逐条落实）：
//   1. Py_InitializeEx(0) 而非 Py_Initialize() —— 失败返回错误码而非 exit()
//      （appspawn 拦截 exit() 报 abort）。
//   2. Py_SetProgramName 先于 Py_Initialize（CPython 3.12）。
//   3. PYTHONHOME 与 PYTHONPATH 双设（stdlib + lib-dynload + site-packages 三段式）
//      —— 只设 PYTHONHOME 不够。
//   4. HOME/XDG_* 无条件指向 el2 可写沙箱（appspawn 预置 HOME=/ 会写只读根）。
//   5. stderr 在 OHOS 进程里是 /dev/null —— fd 1/2 dup2 重定向到沙箱日志文件
//      + faulthandler.enable()，否则未捕获异常无任何痕迹。
//   6. argv[0] 是 .so 路径不能用于推导资源位置 —— 用 dladdr 推导自身 .so 所在目录。
//   7. Qt 头文件（qobject.h 的 slots 宏）破坏 CPython object.h —— 本文件不
//      include 任何 Qt 头（Qt 由 Python 层使用），天然规避。
//   8. 深色模式桥接（setColorThemeMode）为 Step-7 可选增强，OpenShot 自带
//      主题系统，本期不做。
//   9. launch.py 自己处理 SystemExit（已加 OPENSHOT_OHOS 守卫，干净返回）。
// ============================================================================

#include <Python.h>

#include <dlfcn.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

#include "hilog/log.h"

namespace {

// hilog 域/标签（真机日志过滤用: hilog | grep OpenShotLauncher）
constexpr unsigned int kLogDomain = 0x0011;
constexpr const char *kLogTag = "OpenShotLauncher";

void log_info(const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    OH_LOG_Print(LOG_APP, LOG_INFO, kLogDomain, kLogTag, "%{public}s", buf);
}

// ---- 设备侧路径（应用 mount namespace 视角）----
// HAP 的 entry/libs/arm64-v8a/ 安装后映射为 /data/storage/el1/bundle/libs/arm64/
constexpr const char *kDefaultLibDir = "/data/storage/el1/bundle/libs/arm64";
// HAP 的 entry/src/main/resources/resfile/ 安装后映射为
// /data/storage/el1/bundle/entry/resources/resfile/
constexpr const char *kDefaultLaunchScript =
    "/data/storage/el1/bundle/entry/resources/resfile/src/launch.py";

// 可写沙箱（demo 真机验证路径）。appspawn 预置 HOME=/（只读根），必须无条件覆盖。
// OpenShot 全部用户数据走 ~/.openshot_qt（classes/info.py）。
constexpr const char *kFilesDir = "/data/storage/el2/base/files";

// Python 运行时日志（未捕获异常/faulthandler 落点）
constexpr const char *kBootstrapLog = "/data/storage/el2/base/files/openshot.log";

// dladdr 推导本 .so 所在目录（libs/arm64），失败时回落硬编码。
// 用本函数自身的地址定位所在 .so（&main 在此处尚未声明，不可用）。
std::string selfLibraryDir() {
    Dl_info info = {};
    if (dladdr(reinterpret_cast<void *>(&selfLibraryDir), &info) && info.dli_fname) {
        std::string path = info.dli_fname;
        const size_t slash = path.rfind('/');
        if (slash != std::string::npos) {
            return path.substr(0, slash);
        }
    }
    return kDefaultLibDir;
}

void ensure_dir(const std::string &path) {
    // 已存在时失败无碍；不递归创建（父目录由系统保证存在）
    mkdir(path.c_str(), 0700);
}

bool file_readable(const std::string &path) {
    return access(path.c_str(), R_OK) == 0;
}

}  // namespace

// QPA 胶水通过 dlsym("main") 拉起应用；visibility 防止被 hidden 裁掉
// （Qt6 前车之鉴，Qt5 无害）。
extern "C" __attribute__((visibility("default")))
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    log_info("launcher: entry");

    const std::string libDir = selfLibraryDir();
    const std::string pythonHome = libDir + "/Python";
    const char *script = getenv("OPENSHOT_LAUNCH_SCRIPT");  // 调试钩子
    if (!script || !*script) {
        script = kDefaultLaunchScript;
    }

    log_info("launcher: libDir=%s", libDir.c_str());
    log_info("launcher: pythonHome=%s", pythonHome.c_str());
    log_info("launcher: script=%s", script);

    if (!file_readable(script)) {
        log_info("FATAL: launch script not readable: %s", script);
        return 1;
    }

    // ---- 1. 环境布置（必须在 Py_Initialize 之前）----
    ::setenv("OPENSHOT_OHOS", "1", 1);   // Python 层 IS_OHOS 判定锚点
    ::setenv("OPENSHOT_QT_API", "pyqt5", 1);  // PythonForArkUI 系仅 PyQt5
    ::setenv("PYTHONHOME", pythonHome.c_str(), 1);

    // Four-part PYTHONPATH: PYTHONHOME alone does not put site-packages on
    // sys.path, and the embedded PyRun_SimpleFileEx (unlike `python script.py`)
    // does NOT add the script's directory either — launch.py does
    // `from classes import info`, so resfile/src must be on sys.path too.
    {
        std::string scriptDir = script;
        const size_t slash = scriptDir.rfind('/');
        if (slash != std::string::npos) {
            scriptDir.resize(slash);
        }
        const std::string pp = scriptDir
            + ":" + pythonHome + "/lib/python3.12"
            + ":" + pythonHome + "/lib/python3.12/lib-dynload"
            + ":" + pythonHome + "/lib/python3.12/site-packages";
        ::setenv("PYTHONPATH", pp.c_str(), 1);
    }

    // 可写沙箱（无条件覆盖：appspawn 预置 HOME=/）
    ::setenv("HOME", kFilesDir, 1);
    ::setenv("XDG_CACHE_HOME", "/data/storage/el2/base/files/.cache", 1);
    ::setenv("XDG_CONFIG_HOME", "/data/storage/el2/base/files/.config", 1);
    ::setenv("XDG_DATA_HOME", "/data/storage/el2/base/files/.local/share", 1);
    {
        const std::string tmpdir = std::string(kFilesDir) + "/tmp";
        ensure_dir(kFilesDir);
        ensure_dir(tmpdir);
        ::setenv("TMPDIR", tmpdir.c_str(), 1);
    }

    // Qt 插件扫描根：styles/ imageformats/ iconengines/ 在应用 native lib 目录下
    setenv("QT_PLUGIN_PATH", libDir.c_str(), 0 /* 已设置则不覆盖 */);

    // ---- 2. 启动 CPython ----
    {
        wchar_t *program = Py_DecodeLocale("openshot", nullptr);
        if (program != nullptr) {
            Py_SetProgramName(program);  // 3.12：必须先于 Py_Initialize；所有权归 CPython
        }
    }
    if (!Py_IsInitialized()) {
        Py_InitializeEx(0);  // 0 = 失败不装信号处理器、不 exit()
    }
    if (!Py_IsInitialized()) {
        log_info("FATAL: Py_InitializeEx(0) failed");
        return 3;
    }

    // ---- 3. fd 1/2 重定向到沙箱日志（stderr 原本是 /dev/null）----
    {
        static const char kRedirect[] =
            "import os, sys\n"
            "try:\n"
            "    _fd = os.open('/data/storage/el2/base/files/openshot.log',"
            " os.O_WRONLY | os.O_CREAT | os.O_APPEND, 0o666)\n"
            "    os.dup2(_fd, 1)\n"
            "    os.dup2(_fd, 2)\n"
            "    if _fd > 2:\n"
            "        os.close(_fd)\n"
            "except Exception:\n"
            "    pass\n"
            "try:\n"
            "    import faulthandler\n"
            "    faulthandler.enable(sys.stderr)\n"
            "except Exception:\n"
            "    pass\n";
        if (PyRun_SimpleString(kRedirect) != 0) {
            log_info("WARN: fd redirect bootstrap failed (continuing)");
        }
    }

    // ---- 4. sys.argv = [script, *extra] ----
    // extra args are forwarded by the Ability glue (aa start -U channel,
    // e.g. "test:ALL" for the device-side test suite) and must reach
    // launch.py untouched — it sanitizes them (argparse REMAINDER).
    {
        std::vector<wchar_t *> pyArgv;
        std::vector<wchar_t *> owned;
        wchar_t *pyArgv0 = Py_DecodeLocale(script, nullptr);
        if (pyArgv0 != nullptr) {
            pyArgv.push_back(pyArgv0);
        } else {
            log_info("WARN: Py_DecodeLocale(script) failed");
        }
        for (int i = 1; i < argc; ++i) {
            wchar_t *wi = Py_DecodeLocale(argv[i], nullptr);
            if (wi != nullptr) {
                pyArgv.push_back(wi);
                owned.push_back(wi);
            }
        }
        if (!pyArgv.empty()) {
            PySys_SetArgvEx(static_cast<int>(pyArgv.size()), pyArgv.data(), 0);
        }
        for (wchar_t *w : owned) {
            PyMem_RawFree(w);
        }
        PyMem_RawFree(pyArgv0);
    }

    // ---- 5. 执行 launch.py ----
    FILE *pyFile = std::fopen(script, "rb");
    if (pyFile == nullptr) {
        log_info("FATAL: fopen(%s) failed", script);
        Py_FinalizeEx();
        return 5;
    }

    // PyRun_SimpleFileEx 以 __main__ 模块执行 → launch.py 的
    // `if __name__ == "__main__": main()` 成立
    log_info("launcher: handing control to launch.py ...");
    const int rc = PyRun_SimpleFileEx(pyFile, script, 1 /* closefile */);
    if (rc != 0) {
        log_info("launcher: launch.py FAILED rc=%d (see openshot.log)", rc);
    } else {
        log_info("launcher: launch.py finished cleanly");
    }

    // ---- 6. 收尾 ----
    if (Py_FinalizeEx() < 0) {
        return 120;
    }
    log_info("launcher: exit");
    return rc == 0 ? 0 : 6;
}

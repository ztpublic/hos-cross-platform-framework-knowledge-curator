# OpenShot 鸿蒙化 — PyQt5/CPython 运行时组装汇总报告

> 任务：在 Windows 主机上为 OpenShot（PyQt5 应用）鸿蒙化工程组装 PyQt5/CPython 运行时。
> 全部产物位于 `_staging\python\`，本报告为第 6 项子任务（汇总）。
> 汇报日期：2026-10-28。汇报人：AI Agent（OpenCode / GLM 5.3）。

---

## 一、产物总览与盘点

根目录：`D:\Code\ohos_qt-skills\workspace\projects\openshot-ohos\_staging\python\`

| # | 产物 | 路径 | 文件数 | 大小 | 用途 |
|---|------|------|-------:|-----:|------|
| 1 | 参考工程克隆 | `pyqt5-demo-ref\` | 1129（含 .git 1158） | 32.70 MB（含 .git 65.06 MB） | launcher/工程结构/组装脚本的权威参考 |
| 2 | CPython 3.12 运行时 | `cpython\` | 35,795 | 1181.34 MB | 嵌入 HAP 的 Python 解释器 |
| 3 | PyQt5 完整版 | `site-packages-full\` | 48 | 19.01 MB | 未做后缀适配的原始拷贝（存档） |
| 4 | PyQt5 裁剪版 | `site-packages\` | 48 | 19.01 MB | **最终用于 OpenShot 的 site-packages**（sip 已改名） |
| 5 | 纯 Python 依赖 wheels | `vendor-wheels\` | 8 | 1.07 MB | OpenShot 运行期纯 Python 依赖（存档 + sip wheel 溯源） |
| 6 | 纯 Python 依赖（解包） | `vendor\` | 341 | 3.54 MB | 直接可并入 site-packages 的解包形态 |
| — | DevPython-OH 仓库存档 | `_devpython-oh-repo\` | 277（含 .git 311） | 417.89 MB（含 .git 832.84 MB） | CPython hnp 包的来源仓（溯源用，不入 HAP） |
| — | 失败证据日志 | `_pythonforarkui-clone.log` | 1 | 0.001 MB | PythonForArkUI 仓库 404 的记录 |

`cpython\` 三个子目录：

| 子目录 | 内容 | 文件数 | 大小 |
|--------|------|-------:|-----:|
| `cpython\include\` | CPython 头文件（含 `python3.12\Python.h`） | 215 | 1.23 MB |
| `cpython\lib\` | libpython + 26 个依赖 `.so`（29 个文件，含双 soname 副本） | 29 | 22.67 MB |
| `cpython\stdlib\` | lib/python3.12 完整内容（stdlib + lib-dynload + site-packages） | 35,551 | 1157.45 MB |

> ⚠️ `cpython\stdlib\site-packages\` 单独占 **983.38 MB / 27,802 文件**（numpy 2.2.1、scipy 1.14.1、pandas、matplotlib、opencv、pymupdf 等数据科学全家桶，**无 PyQt5**）。**最终组装 entry/libs 时严禁整棵拷入**（见第九节警示）。

来源链：

| 产物 | 来源 | 说明 |
|------|------|------|
| pyqt5-demo-ref | `https://gitcode.com/ohos-qt/pyqt-fluent-widgets.git` @ `8eb0ef1`（2026-10-22，"PyQt-Fluent-Widgets v1.11.3 for HarmonyOS"） | 原目标仓 ohos-qt/pyqt5-demo 已 404，此仓为其等价继任者 |
| cpython | DevPython-OH 仓的 hnp 包（`_devpython-oh-repo\` 内源） | 短 EXT_SUFFIX `.cpython-312.so` 构建 |
| site-packages(-full) | 本机 orange3-ohos 工程 `external\pyqt5-ohos\`（真机验证过的自编译产物） | PyQt5 5.15.11（自交叉编译）+ pyqt5_sip 12.19.0（注册表 wheel） |
| vendor / vendor-wheels | pypi.cnb.cool（OpenHarmonyPCDeveloper 注册表镜像） | 7 个纯 Python wheel + 1 个 pyqt5_sip wheel |

---

## 二、参考工程选型说明

原计划克隆 `ohos-qt/pyqt5-demo`，gitcode 返回 `can not read project. user: <redacted>`（404），确认不在 ohos-qt 组织 204 个仓库中。改用 **`ohos-qt/pyqt-fluent-widgets`**（commit `8eb0ef1`），克隆为 `pyqt5-demo-ref\`。

选型依据：该仓描述为 *"CPython 3.12 embedded launcher + Qt 5.15.16 OHOS QPA; runtime provisioned from public sources via setup.ps1"* —— 与本任务目标（PyQt5 应用 + 嵌入式 CPython 3.12 + OHOS QPA）完全一致，且其真机验证过的 launcher 源码、组装脚本、HAP 裁剪脚本全部可直接参考。

参考工程自带 `tools\staging-manifest.txt`（5987 行，sha256+大小+相对路径），记录了真机验证版 `entry/libs/arm64-v8a` 的完整布局 —— 本报告第 3.7 节摘录其原生库清单。

---

## 三、pyqt5-demo-ref 关键文件全文

### 3.1 架构注释（launcher 文件头，先读这个再读源码）

```
HarmonyOS UIAbility (QAbility.ets)
  -> libqohos.so (Qt QPA platform plugin, linked -> DT_NEEDED, rule B2)
    -> libpyqtfluent.so (launcher, dlsym("main") entry)
      -> CPython 3.12 (embedded, link mode preferred / dlopen fallback)
        -> resfile/python/fluent_app.py (OHOS compat patches + gallery)
          -> PyQt5 -> qfluentwidgets gallery
```

### 3.2 launcher 源码全文

路径：`pyqt5-demo-ref\entry\src\main\cpp\pyqtfluent_launcher.cpp`（425 行）

```cpp
// ============================================================================
// PyQt-Fluent-Widgets for HarmonyOS — PyQt5 application launcher (libpyqtfluent.so)
//
// Architecture (per KB semantic/qt-harmonyos-pyqt-status.md + orange3 precedent):
//
//   HarmonyOS UIAbility (QAbility.ets)
//     -> libqohos.so (Qt QPA platform plugin, linked -> DT_NEEDED, rule B2)
//       -> libpyqtfluent.so (this file, dlsym("main") entry)
//         -> CPython 3.12 (embedded, link mode preferred / dlopen fallback)
//           -> resfile/python/fluent_app.py (OHOS compat patches + gallery)
//             -> PyQt5 -> qfluentwidgets gallery
//
// Golden rules observed:
//   L2: argv[0] is the .so library path — never used to derive resource
//       locations (dladdr + env override instead).
//   B2: Qt5::QOhosPlatformIntegrationPlugin linked so libqohos.so is loaded
//       before CPython starts (platform ready for PyQt5's QApplication).
//   P1/P2: no chmod/symlink usage here.
//   KB pyqt-status #2: Py_InitializeEx(0) instead of Py_Initialize() so an
//       init failure returns instead of exit() (appspawn abort guard).
//   KB pyqt-status #4: Py_SetProgramName before Py_Initialize (CPython 3.12).
// ============================================================================

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <dlfcn.h>
#include <unistd.h>

#include <hilog/log.h>

#ifdef PYQTFLUENT_LINK_PYTHON
#include <Python.h>
#endif

#define LOG_TAG_FLUENT "pyqtfluent-launcher"
#define LOG_DOMAIN_FLUENT 0xD003400

// stderr is /dev/null in an OHOS app process — use hilog (visible via
// `hdc shell hilog -x | grep pyqtfluent-launcher`), stderr as fallback.
static void logInfo(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

static void logInfo(const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN_FLUENT, LOG_TAG_FLUENT,
                 "%{public}s", buf);
    std::fprintf(stderr, "[%s] %s\n", LOG_TAG_FLUENT, buf);
    std::fflush(stderr);
}

// HAP runtime layout constants (pyqt5-demo / orange3, KB-verified):
//   entry/libs/arm64-v8a/** extracts to /data/storage/el1/bundle/libs/arm64/**
//   entry/src/main/resources/resfile/** extracts to
//   /data/storage/el1/bundle/entry/resources/resfile/**
static const char kDefaultPythonHome[] =
    "/data/storage/el1/bundle/libs/arm64/Python";
static const char kDefaultAppScript[] =
    "/data/storage/el1/bundle/entry/resources/resfile/python/fluent_app.py";
static const char kLibpythonSoname[] = "libpython3.12.so.1.0";
static const char kLibpythonAltName[] = "libpython3.12.so";

// Writable app sandbox (el2) — HOME/XDG targets so QSettings (qfluentwidgets
// qconfig) and Qt caches never touch read-only bundle paths.
static const char kDefaultFilesDir[] = "/data/storage/el2/base/files";

static void setenvIfUnset(const char *name, const char *value)
{
    if (::getenv(name) == nullptr)
        ::setenv(name, value, 0);
}

// Directory containing this shared library at runtime (dladdr).
static std::string selfLibraryDir()
{
    Dl_info info{};
    if (dladdr(reinterpret_cast<void *>(&selfLibraryDir), &info) == 0 ||
        info.dli_fname == nullptr)
        return {};
    std::string path(info.dli_fname);
    const auto slash = path.find_last_of('/');
    return slash == std::string::npos ? std::string() : path.substr(0, slash);
}

static bool fileReadable(const std::string &path)
{
    return !path.empty() && ::access(path.c_str(), R_OK) == 0;
}

// PYTHONHOME resolution order:
//   1. $PYQTFLUENT_PYTHON_HOME (explicit override)
//   2. <dir-of-this-.so>/Python (dladdr-derived, layout-proof)
//   3. compiled default
static std::string resolvePythonHome()
{
    if (const char *env = ::getenv("PYQTFLUENT_PYTHON_HOME"))
        return env;
    const std::string dir = selfLibraryDir();
    if (!dir.empty()) {
        const std::string candidate = dir + "/Python";
        if (fileReadable(candidate + "/lib/python3.12"))
            return candidate;
    }
    return kDefaultPythonHome;
}

// Bootstrap script resolution order:
//   1. $PYQTFLUENT_APP_SCRIPT
//   2. <bundle-root>/entry/resources/resfile/python/fluent_app.py (dladdr)
//   3. compiled default
static std::string resolveAppScript()
{
    if (const char *env = ::getenv("PYQTFLUENT_APP_SCRIPT"))
        return env;
    const std::string dir = selfLibraryDir();
    if (!dir.empty()) {
        // <bundle>/libs/arm64 -> <bundle>
        const auto slash = dir.find_last_of('/');
        if (slash != std::string::npos) {
            const std::string libsParent = dir.substr(0, slash);
            const auto slash2 = libsParent.find_last_of('/');
            if (slash2 != std::string::npos) {
                const std::string bundleRoot = libsParent.substr(0, slash2);
                const std::string candidate = bundleRoot +
                    "/entry/resources/resfile/python/fluent_app.py";
                if (fileReadable(candidate))
                    return candidate;
            }
        }
    }
    return kDefaultAppScript;
}

// ---------------------------------------------------------------------------
// CPython C API access layer (dual-mode)
// ---------------------------------------------------------------------------

typedef void (*PFN_PyInitializeEx)(int);
typedef int (*PFN_PyIsInitialized)(void);
typedef void (*PFN_PyFinalize)(void);
typedef int (*PFN_PyRunSimpleFileEx)(FILE *, const char *, int);
typedef int (*PFN_PyRunSimpleString)(const char *);
typedef void (*PFN_PySysSetArgvEx)(int, wchar_t **, int);
typedef wchar_t *(*PFN_PyDecodeLocale)(const char *, size_t *);
typedef void (*PFN_PyMemRawFree)(void *);
typedef void (*PFN_PySetProgramName)(const wchar_t *);

#ifdef PYQTFLUENT_LINK_PYTHON

static PFN_PyInitializeEx pyInitializeEx = &Py_InitializeEx;
static PFN_PyIsInitialized pyIsInitialized = &Py_IsInitialized;
static PFN_PyFinalize pyFinalize = &Py_Finalize;
static PFN_PyRunSimpleFileEx pyRunSimpleFileEx = &PyRun_SimpleFileEx;
static PFN_PyRunSimpleString pyRunSimpleString = &PyRun_SimpleString;
static PFN_PySysSetArgvEx pySysSetArgvEx = &PySys_SetArgvEx;
static PFN_PyDecodeLocale pyDecodeLocale = &Py_DecodeLocale;
static PFN_PyMemRawFree pyMemRawFree = &PyMem_RawFree;
static PFN_PySetProgramName pySetProgramName = &Py_SetProgramName;

static bool resolvePythonApi() { return true; }

#else // dlopen mode

static PFN_PyInitializeEx pyInitializeEx = nullptr;
static PFN_PyIsInitialized pyIsInitialized = nullptr;
static PFN_PyFinalize pyFinalize = nullptr;
static PFN_PyRunSimpleFileEx pyRunSimpleFileEx = nullptr;
static PFN_PyRunSimpleString pyRunSimpleString = nullptr;
static PFN_PySysSetArgvEx pySysSetArgvEx = nullptr;
static PFN_PyDecodeLocale pyDecodeLocale = nullptr;
static PFN_PyMemRawFree pyMemRawFree = nullptr;
static PFN_PySetProgramName pySetProgramName = nullptr;

static bool resolvePythonApi()
{
    // RTLD_GLOBAL: PyQt5/numpy extension modules (whose DT_NEEDED records
    // libpython) resolve Py* symbols through the globally loaded interpreter.
    void *handle = ::dlopen(kLibpythonSoname, RTLD_NOW | RTLD_GLOBAL);
    if (!handle)
        handle = ::dlopen(kLibpythonAltName, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        const char *err = ::dlerror();
        logInfo("dlopen(%s) failed: %s", kLibpythonSoname,
                err ? err : "unknown");
        return false;
    }
    struct Entry {
        const char *name;
        void **slot;
    };
    const Entry entries[] = {
        {"Py_InitializeEx", reinterpret_cast<void **>(&pyInitializeEx)},
        {"Py_IsInitialized", reinterpret_cast<void **>(&pyIsInitialized)},
        {"Py_Finalize", reinterpret_cast<void **>(&pyFinalize)},
        {"PyRun_SimpleFileEx", reinterpret_cast<void **>(&pyRunSimpleFileEx)},
        {"PyRun_SimpleString", reinterpret_cast<void **>(&pyRunSimpleString)},
        {"PySys_SetArgvEx", reinterpret_cast<void **>(&pySysSetArgvEx)},
        {"Py_DecodeLocale", reinterpret_cast<void **>(&pyDecodeLocale)},
        {"PyMem_RawFree", reinterpret_cast<void **>(&pyMemRawFree)},
        {"Py_SetProgramName", reinterpret_cast<void **>(&pySetProgramName)},
    };
    for (const Entry &e : entries) {
        *e.slot = ::dlsym(handle, e.name);
        if (*e.slot == nullptr) {
            logInfo("dlsym(%s) failed", e.name);
            return false;
        }
    }
    return true;
}

#endif // PYQTFLUENT_LINK_PYTHON

// ---------------------------------------------------------------------------
// OHOS system dark-mode bridge for the Python side (darkdetect stub).
//
// The gallery's darkdetect stand-in resolves the system color mode by
// calling this function via ctypes.CDLL on the /proc/self/maps path of
// libpyqtfluent.so (the QPA glue dlopen()s the launcher RTLD_LOCAL, so
// dlopen(NULL) cannot see this export). The query is LIVE: qfluentwidgets'
// SystemThemeListener polling thread (1 s interval, 2 s anti-shake) picks
// up system theme switches and re-themes the app when themeMode == AUTO.
//
// Returns 1 = dark, 0 = light, -1 = unknown (no QCoreApplication yet, or
// the QPA app context does not exist — both are true during Python's
// import phase, before QApplication construction).
//
// Device finding (HAD-W24 7.0.0.32, SDK qt-ohos-arm64-15-20260806): the
// app's OHOS color mode defaults to LightTheme, which PINS
// context.config.colorMode to "light" — the system then never delivers a
// dark Configuration (and onConfigurationUpdate never fires on system
// toggles), so a plain darkThemeActive() query stays "light" forever.
// Fix: on the first successful query, switch the app color mode to
// FollowSystemSetting via setColorThemeMode(). Afterwards the served
// configuration tracks the system, and the QPA's config data source
// updates darkThemeActive() both at startup and on runtime toggles.
//
// NOTE: qohosappcontext.h is included HERE (after every Python.h include
// and Py* declaration above) on purpose — qobject.h's `slots` macro breaks
// CPython's object.h ("PyType_Slot *slots;"). No Python header may be
// parsed after this point.
// ---------------------------------------------------------------------------
#include <atomic>
#include <QtCore/QCoreApplication>
#include <qohosappcontext.h>   // QtOhosExtras

static std::atomic<bool> pyqtfluent_follow_system_requested{false};

extern "C" __attribute__((visibility("default")))
int pyqtfluent_system_dark_active()
{
    // The QPA app context does not exist before QApplication construction,
    // and QOhosAppContext::instance() dereferences platform-integration
    // state — calling it during Python's import phase (qfluentwidgets
    // config load) SEGFAULTS. Guard: no QCoreApplication yet → "unknown".
    // (Python threads cannot run during the QApplication constructor, so
    // from Python's perspective instance()!=null ⇒ the OHOS app context
    // exists — no gap.)
    if (QCoreApplication::instance() == nullptr)
        return -1;
    QtOhosExtras::QOhosAppContext *ctx = QtOhosExtras::QOhosAppContext::instance();
    if (ctx == nullptr)
        return -1;
    // One-shot: release the app color mode from the LightTheme default so
    // the served Configuration follows the system (see comment above).
    if (!pyqtfluent_follow_system_requested.exchange(true)) {
        ctx->setColorThemeMode(
            QtOhosExtras::QOhosAppContext::ColorThemeMode::FollowSystemSetting);
    }
    return ctx->darkThemeActive() ? 1 : 0;
}

// ---------------------------------------------------------------------------
// main — the QPA glue dlsym()s this symbol from libpyqtfluent.so
// ---------------------------------------------------------------------------

extern "C" __attribute__((visibility("default")))
int main(int argc, char *argv[])
{
    // ---- 1. Locate the Python runtime and the bootstrap script -----------
    const std::string pythonHome = resolvePythonHome();
    const std::string scriptPath = resolveAppScript();

    if (!fileReadable(scriptPath)) {
        logInfo("FATAL: bootstrap script not readable: %s", scriptPath.c_str());
        return 1;
    }
    logInfo("python home : %s", pythonHome.c_str());
    logInfo("app script  : %s", scriptPath.c_str());

    // ---- 2. Environment (before Py_Initialize) ---------------------------
    ::setenv("PYTHONHOME", pythonHome.c_str(), 1);

    // KB pyqt-status roLabelImg finding #3: PYTHONHOME alone is not enough —
    // PYTHONPATH must also point at the stdlib, lib-dynload and site-packages.
    // (Embedded Py_InitializeEx does not always get site-packages onto
    // sys.path, and the stdlib prefix resolution depends on the tree layout.)
    {
        const std::string pp = pythonHome + "/lib/python3.12"
            + ":" + pythonHome + "/lib/python3.12/lib-dynload"
            + ":" + pythonHome + "/lib/python3.12/site-packages";
        ::setenv("PYTHONPATH", pp.c_str(), 1);
    }

    // Writable sandbox for HOME/XDG (qfluentwidgets QSettings/qconfig).
    // UNCONDITIONAL: appspawn presets HOME=/ which would send every ~/.cache
    // write to the read-only root.
    ::setenv("HOME", kDefaultFilesDir, 1);
    ::setenv("XDG_CACHE_HOME", "/data/storage/el2/base/files/.cache", 1);
    ::setenv("XDG_CONFIG_HOME", "/data/storage/el2/base/files/.config", 1);
    ::setenv("XDG_DATA_HOME", "/data/storage/el2/base/files/.local/share", 1);

    // Qt plugin scanning root: styles/, imageformats/, iconengines/,
    // sqldrivers/ live as subdirectories of the app native lib dir.
    // (libqohos.so itself is pulled in via DT_NEEDED — see CMakeLists.)
    {
        const std::string libDir = selfLibraryDir();
        if (!libDir.empty())
            setenvIfUnset("QT_PLUGIN_PATH", libDir.c_str());
    }

    // ---- 3. Boot CPython --------------------------------------------------
    if (!resolvePythonApi()) {
        logInfo("FATAL: cannot load CPython runtime (%s)", kLibpythonSoname);
        return 2;
    }

    // KB pyqt-status #4: Py_SetProgramName before init (CPython 3.12).
    {
        wchar_t *program = pyDecodeLocale("pyqtfluent", nullptr);
        if (program != nullptr) {
            pySetProgramName(program);
            // Intentionally not freed: Py_SetProgramName keeps ownership.
        }
    }

    // KB pyqt-status #2: Py_InitializeEx(0) — init failure must return, not
    // exit() (appspawn intercepts exit() and reports an abort).
    if (!pyIsInitialized())
        pyInitializeEx(0);
    if (!pyIsInitialized()) {
        logInfo("FATAL: Py_InitializeEx(0) failed");
        return 3;
    }

    // ---- 3b. Redirect fd 1/2 into the sandbox log ------------------------
    // stderr is /dev/null in an OHOS app process, so an uncaught Python
    // exception (e.g. a module-level ImportError in the bootstrap script,
    // outside its try/except) would print its traceback into a black hole —
    // exactly the 2026-10-20 device failure (rc=-1, no diagnostics). dup2
    // the log file onto fd 1/2 so tracebacks, print() and Qt qWarning all
    // land in fluent_app.log. faulthandler gives native-crash tracebacks.
    {
        static const char kRedirect[] =
            "import os, sys\n"
            "try:\n"
            "    _fd = os.open('/data/storage/el2/base/files/fluent_app.log',"
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
        if (pyRunSimpleString(kRedirect) != 0)
            logInfo("WARN: fd redirect bootstrap failed (continuing)");
    }

    // sys.argv: [script, *extra]
    {
        std::vector<wchar_t *> pyArgv;
        std::vector<wchar_t *> owned;
        wchar_t *w0 = pyDecodeLocale(scriptPath.c_str(), nullptr);
        if (w0 == nullptr) {
            logInfo("FATAL: Py_DecodeLocale failed");
            return 4;
        }
        pyArgv.push_back(w0);
        for (int i = 1; i < argc; ++i) {
            wchar_t *wi = pyDecodeLocale(argv[i], nullptr);
            if (wi != nullptr) {
                pyArgv.push_back(wi);
                owned.push_back(wi);
            }
        }
        pySysSetArgvEx(static_cast<int>(pyArgv.size()), pyArgv.data(), 1);
        for (wchar_t *w : owned)
            pyMemRawFree(w);
        pyMemRawFree(w0);
    }

    // ---- 4. Run the bootstrap ---------------------------------------------
    FILE *script = std::fopen(scriptPath, "rb");
    if (script == nullptr) {
        logInfo("FATAL: fopen(%s) failed", scriptPath.c_str());
        return 5;
    }
    logInfo("handing control to fluent_app.py ...");
    const int rc = pyRunSimpleFileEx(script, scriptPath.c_str(), 1);
    if (rc != 0) {
        std::fclose(script); // closeit=1 already closed it on success
        logInfo("fluent_app.py FAILED, rc=%d", rc);
    } else {
        logInfo("fluent_app.py finished cleanly, finalizing interpreter");
    }

    // fluent_app.py catches SystemExit itself (KB pyqt-status #2 guard);
    // reaching here means the event loop ended cleanly.
    pyFinalize();

    return rc == 0 ? 0 : 6;
}
```

**launcher 坑清单（OpenShot 移植必须照抄的点）**：

1. `Py_InitializeEx(0)` 而非 `Py_Initialize()` —— 初始化失败返回错误码而不是 `exit()`（appspawn 拦截 `exit()` 报 abort）。
2. `Py_SetProgramName` 必须先于 `Py_Initialize`（CPython 3.12）。
3. `PYTHONHOME` 与 `PYTHONPATH` **双双设置**（stdlib + lib-dynload + site-packages 三段式）—— 只设 PYTHONHOME 不够。
4. `HOME`/`XDG_*` 无条件指向 el2 可写沙箱（appspawn 预置 `HOME=/`，会把缓存写进只读根）。
5. stderr 在 OHOS 进程里是 `/dev/null` —— 必须 fd 1/2 dup2 重定向到沙箱日志文件 + `faulthandler.enable()`，否则未捕获异常无任何痕迹。
6. argv[0] 是 .so 路径，不能用来推导资源位置（铁律 L2）—— 用 `dladdr` + 环境变量覆盖。
7. Qt 头文件（qobject.h 的 `slots` 宏）会破坏 CPython object.h —— `qohosappctx.h` 必须放在所有 Python.h 之后 include。
8. 深色模式桥接：QAbility 的 OHOS colorMode 默认钉死 light，首次查询时须 `setColorThemeMode(FollowSystemSetting)` 一次性解锁。
9. bootstrap 脚本（fluent_app.py）自己捕获 SystemExit。

### 3.3 CMakeLists.txt 全文

路径：`pyqt5-demo-ref\entry\src\main\cpp\CMakeLists.txt`（132 行）

```cmake
# ============================================================================
# PyQt-Fluent-Widgets for HarmonyOS — PyQt5 application launcher build
#
#   Product: libpyqtfluent.so — must match APP_LIBRARY_NAME in
#            entry/src/main/ets/common/QtAppConstants.ets ('libpyqtfluent.so')
#
#   The launcher embeds CPython 3.12 (dual-mode, see pyqtfluent_launcher.cpp):
#     link mode   — thirdparty/include/python3.12/Python.h present
#                   AND libs/arm64-v8a/libpython3.12.so.1.0 present
#     dlopen mode — otherwise (loads libpython from the HAP libs dir at
#                   runtime; build needs no Python artifacts)
#
# Golden rules:
#   B1 — CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH before find_package
#   B2 — Qt5::QOhosPlatformIntegrationPlugin linked (libqohos.so → DT_NEEDED,
#        loaded before CPython starts, platform ready for PyQt5)
# ============================================================================

cmake_minimum_required(VERSION 3.16)

project(pyqtfluent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Template NAPI sample module (kept: oh-package.json5 references its types)
add_library(entry SHARED hello.cpp)
target_link_libraries(entry PUBLIC libace_napi.z.so)

# ---- Rule B1: cross-compile find_package fix (MUST precede find_package) ----
if(CMAKE_CROSSCOMPILING)
    set(_pyqtfluent_saved_rm "${CMAKE_FIND_ROOT_PATH_MODE_PACKAGE}")
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
endif()

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core
    Gui
)

# QtOhosExtras — system dark-mode query (QOhosAppContext::darkThemeActive()).
# Deliberately NOT a find_package component: linking an imported Qt5 target
# makes hvigor auto-stage the UNSTRIPPED SDK copy (plus its transitive
# Core/Gui/Widgets deps) into intermediates, colliding with the stripped
# entry/libs copies at ProcessLibs (00306049 duplicated files) — the same
# lesson as the libqohos MODULE copy. Include dir + path link instead.

if(CMAKE_CROSSCOMPILING)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE "${_pyqtfluent_saved_rm}")
    unset(_pyqtfluent_saved_rm)
endif()

# ---- PyQt-Fluent-Widgets launcher (PyQt5 app entry, embeds CPython) ---------
add_library(pyqtfluent SHARED pyqtfluent_launcher.cpp)

# entry/libs staging dir (also used below for the CPython link-mode probe)
set(PYQTFLUENT_ENTRY_LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/arm64-v8a)

# QtOhosExtras headers via the Qt5 SDK root (Qt5Core_DIR = <sdk>/lib/cmake/Qt5Core)
get_filename_component(QT5_SDK_ROOT "${Qt5Core_DIR}/../../.." ABSOLUTE)

target_include_directories(pyqtfluent PRIVATE
    "${QT5_SDK_ROOT}/include"              # QtCore/qglobal.h from qtohosextrasglobal.h
    "${QT5_SDK_ROOT}/include/QtOhosExtras")

target_link_libraries(pyqtfluent PRIVATE
    # hilog: stderr is /dev/null in an OHOS app process — launcher logs go
    # to hilog (hdc shell hilog -x | grep pyqtfluent-launcher)
    libhilog_ndk.z.so
    # ★ Rule B2 (revised 2026-10-21): link the OHOS QPA platform plugin BY
    #   PATH against the entry/libs copy instead of the imported
    #   Qt5::QOhosPlatformIntegrationPlugin MODULE target. Linking an
    #   imported MODULE makes CMake copy the 170 MB unstripped plugin into
    #   the build output, which then collides with the (stripped) entry/libs
    #   copy at hvigor ProcessLibs (00306049 duplicated files). A direct
    #   file link yields the identical DT_NEEDED (SONAME libqohos.so, so
    #   the platform integration is live before CPython/PyQt5 create a
    #   QApplication) and packs only the stripped staging copy.
    ${PYQTFLUENT_ENTRY_LIBS_DIR}/libqohos.so
    # Qt5Core by path — QCoreApplication::self (instance()) resolves here;
    # QtOhosExtras by path (SONAME unversioned, DT_NEEDED Qt5Widgets/Gui/
    # Core all packed in the HAP) — the system dark-mode bridge for the
    # darkdetect stub; see pyqtfluent_system_dark_active in the launcher.
    # (Imported Qt5 targets are deliberately NOT used: hvigor auto-stages
    # their UNSTRIPPED SDK copies into the HAP — ProcessLibs 00306049.)
    ${PYQTFLUENT_ENTRY_LIBS_DIR}/libQt5Core.so
    ${PYQTFLUENT_ENTRY_LIBS_DIR}/libQt5OhosExtras.so
)

# ---- Optional CPython 3.12 link mode ----------------------------------------
#   thirdparty/include/python3.12/Python.h
#   ../../../../../libs/arm64-v8a/libpython3.12.so.1.0  (entry/libs/arm64-v8a)
# NOTE: find_path is subject to the same cross-compile root-path restriction
# as find_package (rule B1) — temporarily widen to BOTH.
set(PYQTFLUENT_PYTHON_INCLUDE_HINT
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/include/python3.12)

if(CMAKE_CROSSCOMPILING)
    set(_pyqtfluent_saved_incl "${CMAKE_FIND_ROOT_PATH_MODE_INCLUDE}")
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
endif()

find_path(PYQTFLUENT_PYTHON_INCLUDE_DIR
    NAMES Python.h
    PATHS ${PYQTFLUENT_PYTHON_INCLUDE_HINT}
    NO_DEFAULT_PATH)

if(CMAKE_CROSSCOMPILING)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE "${_pyqtfluent_saved_incl}")
    unset(_pyqtfluent_saved_incl)
endif()

# Direct file probe: find_library mishandles versioned sonames
# (libpython3.12.so.1.0) with its prefix/suffix transforms.
set(PYQTFLUENT_PYTHON_LIBRARY "${PYQTFLUENT_ENTRY_LIBS_DIR}/libpython3.12.so.1.0")
if(NOT EXISTS "${PYQTFLUENT_PYTHON_LIBRARY}")
    set(PYQTFLUENT_PYTHON_LIBRARY "")
endif()

if(PYQTFLUENT_PYTHON_INCLUDE_DIR AND PYQTFLUENT_PYTHON_LIBRARY)
    target_compile_definitions(pyqtfluent PRIVATE PYQTFLUENT_LINK_PYTHON)
    target_include_directories(pyqtfluent PRIVATE ${PYQTFLUENT_PYTHON_INCLUDE_DIR})
    target_link_libraries(pyqtfluent PRIVATE ${PYQTFLUENT_PYTHON_LIBRARY})
    message(STATUS "pyqtfluent: CPython link mode (${PYQTFLUENT_PYTHON_LIBRARY})")
else()
    message(STATUS "pyqtfluent: CPython dlopen mode (libpython3.12.so.1.0 "
                   "loaded from HAP libs at runtime)")
endif()

# Note: no CXX_VISIBILITY_PRESET hidden — the QPA glue dlsym()s "main" from
# libpyqtfluent.so (also guarded by __attribute__((visibility("default")))).
```

**CMake 关键决策**：

- **B2 修订版（2026-10-21）**：不 link 导入的 `Qt5::QOhosPlatformIntegrationPlugin` MODULE 目标（会把 170 MB 未 strip 的插件拷进构建产物，与 entry/libs 的 strip 版冲突 → hvigor ProcessLibs 00306049），改为**按路径** link `entry/libs/arm64-v8a/libqohos.so`，DT_NEEDED 效果完全相同。
- Qt5Core / Qt5OhosExtras 同理按路径 link（不用导入目标）。
- CPython 双模式：头文件 + libpython 都在位 → link 模式；否则 dlopen 模式（构建期不需要 Python 产物）。
- `libqohos.so` 通过 DT_NEEDED 在 CPython 启动前加载 —— QApplication 构造时 QPA 已就绪（铁律 B2）。

### 3.4 QtAppConstants.ets 全文

路径：`pyqt5-demo-ref\entry\src\main\ets\common\QtAppConstants.ets`（3 行）

```typescript
export const APP_LIBRARY_NAME = 'libpyqtfluent.so';
export const LOG_DOMAIN = 0x0000;
export const LOG_TAG = 'PyQtFluentWidgets';
```

> OpenShot 移植时改为 `libopenshot.so` 之类的名字，且必须与 CMakeLists 的 `project()`/`add_library()` 产物一致。

### 3.5 两级 build-profile.json5 及差异

**根 `build-profile.json5`（app 级，46 行）** —— 管产品/签名/模块清单：

```json5
{
  "app": {
    "signingConfigs": [],
    "products": [
      {
        "name": "default",
        "compatibleSdkVersion": "6.1.1(24)",
        "runtimeOS": "HarmonyOS",
      }
    ],
    "buildModeSet": [
      {
        "name": "debug",
      },
      {
        "name": "release"
      }
    ],
  },
  "modules": [
    {
      "name": "entry",
      "srcPath": "./entry",
      "targets": [
        {
          "name": "default",
          "applyToProducts": [
            "default"
          ]
        }
      ]
    },
    {
      "name": "qEmbeddedUiExtensionHost",
      "srcPath": "./qEmbeddedUiExtensionHost",
      "targets": [
        {
          "name": "default",
          "applyToProducts": [
            "default"
          ]
        }
      ]
    },
  ]
}
```

**`entry\build-profile.json5`（module 级，42 行）** —— 管 native 构建/CMake/ABI：

```json5
{
  "apiType": 'stageMode',
  "buildOption": {
    "externalNativeOptions": {
      "path": "./src/main/cpp/CMakeLists.txt",
      "arguments": "-DCMAKE_PREFIX_PATH=<YOUR_QT_OHOS_SDK>",
      "cppFlags": "",
      "abiFilters": [
        "arm64-v8a"
      ]
    },
    "nativeLib": {
      "collectAllLibs": true,
      "debugSymbol": {
        "strip": false
      }
    }
  },
  "buildOptionSet": [
    {
      "name": "release",
      "arkOptions": {
        "obfuscation": {
          "ruleOptions": {
            "enable": true,
            "files": [
              "./obfuscation-rules.txt"
            ]
          }
        }
      }
    },
  ],
  "targets": [
    {
      "name": "default"
    },
    {
      "name": "ohosTest",
    }
  ]
}
```

**两级差异要点**：

| 维度 | 根 build-profile.json5 | entry\build-profile.json5 |
|------|------------------------|---------------------------|
| 职责 | 产品定义：`compatibleSdkVersion: "6.1.1(24)"`、签名配置、buildModeSet | 单模块构建：native 构建入口与选项 |
| 模块清单 | 列出 entry + qEmbeddedUiExtensionHost 两个模块及各自 targets | 不列模块 |
| CMake | 不涉及 | `externalNativeOptions.path` 指向 cpp\CMakeLists.txt；`arguments` 含 `-DCMAKE_PREFIX_PATH=<YOUR_QT_OHOS_SDK>` **占位符** |
| ABI | 不涉及 | `abiFilters: ["arm64-v8a"]` |
| 打包 | 不涉及 | `nativeLib.collectAllLibs: true`（entry/libs 全部收进 HAP）+ `debugSymbol.strip: false` |
| 混淆 | 不涉及 | release buildOptionSet 开启 ArkTS 混淆 |

> `setup.ps1` 第 5 步会把 `-DCMAKE_PREFIX_PATH=<YOUR_QT_OHOS_SDK>` 占位符替换为实际 Qt OHOS SDK 路径（见 3.9）。

### 3.6 module.json5 的 deviceTypes（关键摘录）

路径：`pyqt5-demo-ref\entry\src\main\module.json5`（214 行）

```json5
    "mainElement": "QAbility",
    "deviceTypes": [
      // When 'phone' is added to the list, the main window does not restore itself to
      // the desired state after being minimized (QTFOROH-1076). Before it is added
      // back, be sure that the issue has been resolved.
      "tablet",
      "2in1"
    ],
```

> ⚠️ **OpenShot 的 module.json5 必须同样排除 `phone`**（QTFOROH-1076：加 phone 后主窗口最小化无法恢复，去掉后无法恢复）。先只用 `["tablet", "2in1"]`。
>
> module.json5 其余要点：`mainElement: "QAbility"`、QAbility 为 `launchType: "specified"` + `isolationProcess: true`、skills 含 `entity.system.home`/`action.system.home`（桌面入口）与三个 `file://` utd（general.text / general.png / general.jpeg，maxFileSupported 10 —— 文件打开入口，OpenShot 的 .osp 工程文件关联可参考此模式）；requestPermissions 含 SYSTEM_FLOAT_WINDOW、FILE_ACCESS_PERSIST、INTERNET、READ_PASTEBOARD、CAMERA、MICROPHONE 等（OpenShot 按需裁剪）；metadata `enable.remove.starting.window: true`。另有 qEmbeddedUiExtensionHost 模块的 embeddedUI 扩展能力（instance/bundle 两种进程模式）。

### 3.7 entry/libs/arm64-v8a 清单（真机验证版，摘自 tools/staging-manifest.txt）

manifest 格式：`sha256 <TAB> 字节 <TAB> 相对路径`（相对 entry/libs/arm64-v8a/），共 5987 行。原生库部分如下（同名双 soname 行已合并标注）：

**Python 运行时：**

| 文件 | 字节数 |
|------|-------:|
| libpython3.12.so / libpython3.12.so.1.0 | 6,288,832 |
| libpython3.so（stub） | 4,224 |

**Qt 5.15.16 OHOS（来自 Qt OHOS SDK，setup.ps1 步骤 2 产物，不在本次 _staging 内）：**

| 文件 | 字节数 |
|------|-------:|
| libqohos.so（QPA 平台插件） | 60,763,152 |
| libQt5Widgets.so | 13,302,568 |
| libQt5Gui.so | 18,195,320 |
| libQt5Core.so | 14,434,520 |
| libQt5Network.so | 3,348,376 |
| libQt5OhosExtras.so | 1,694,016 |
| libQt5OpenGL.so | 819,504 |
| libQt5PrintSupport.so | 784,720 |
| libQt5Svg.so | 800,888 |
| libQt5Test.so | 838,896 |
| libQt5Xml.so | 635,696 |

**CPython 依赖库（与本次 `cpython\lib\` 的 29 个文件一一对应）：**

| 文件 | 字节数 |
|------|-------:|
| libbz2.so.1.0 / libbz2.so.1.0.8 | 65,360 |
| libcryptooh.so.3 | 3,645,584 |
| libffi.so.8 / libffi.so.8.1.4 | 86,264 |
| libgdbm.so.6 / libgdbm.so.6.0.0 | 60,216 |
| libgdbm_compat.so.4 / libgdbm_compat.so.4.0.0 | 9,880 |
| libiconv.so.2 / libiconv.so.2.6.1 | 914,136 |
| libintl.so.8 / libintl.so.8.4.0 | 99,200 |
| liblzma.so.5 / liblzma.so.5.6.3 | 162,400 |
| libreadline.so.8 / libreadline.so.8.2 | 314,336 |
| libsqlite3.so.0 / libsqlite3.so.0.8.6 | 1,468,320 |
| libssloh.so.3 | 593,464 |
| libtinfow.so.6 / libtinfow.so.6.4 | 173,056 |
| libuuid.so.1 / libuuid.so.1.3.0 | 25,944 |
| libz.so.1 / libz.so.1.3.1 | 93,760 |

**Qt 插件（native lib dir 子目录）：**

- `iconengines/libqsvgicon.so` — 96,976
- `imageformats/`：libqgif.so (61,640)、libqicns.so (94,592)、libqico.so (67,048)、libqjpeg.so (925,288)、libqsvg.so (51,024)、libqtga.so (51,000)、libqtiff.so (748,400)、libqwbmp.so (43,880)、libqwebp.so (921,960)

**Python 树 `Python/lib/python3.12/`：**

- 完整 stdlib（asyncio、collections、ctypes、email、encodings、importlib、json、logging、multiprocessing、sqlite3、unittest、urllib、xml 等，含 `__pycache__/*.cpython-312.pyc` 三档 pyc）
- `lib-dynload/`：73 个 `.cpython-312.so` 扩展模块（**短后缀**，与 DevPython-OH 构建一致）
- `site-packages/`：
  - **PyQt5**（11 个 .so）：`Qt.abi3.so`、`QtCore.abi3.so`、`QtGui.abi3.so`、`QtNetwork.abi3.so`、`QtPrintSupport.abi3.so`、`QtSvg.abi3.so`、`QtTest.abi3.so`、`QtWidgets.abi3.so`、`QtXml.abi3.so`、`_QOpenGLFunctions_ES2.abi3.so`、`sip.cpython-312.so`（**短后缀，改名后的形态**）+ uic 纯 Python 部分
  - numpy 2.2.1（`_core/_multiarray_umath.cpython-312.so` 等短后缀 + `numpy.libs/libopenblasp-r0-76071c68.3.27.so`）、pillow 11.0.0、scipy 1.14.1（demo 顺带保留的 scientific 栈）

> 本次 `site-packages\` 裁剪产物与该验证清单的 PyQt5 部分同源同构（同为 orange3 交叉编译产物，`.abi3.so` 双构建通用；差异仅 demo 多编了 QtNetwork/QtPrintSupport/QtTest/QtXml 四个模块，OpenShot 不需要）。

### 3.8 resfile/python 布局（demo 模式）

```
entry/src/main/resources/resfile/python/
├── fluent_app.py        (5,873 B)  ← 入口脚本（launcher 执行它）
├── ohos_compat.py       (15,297 B) ← OHOS 兼容补丁层
├── darkdetect.py        (5,206 B)  ← darkdetect 替身（ctypes 调 launcher 导出）
├── colorthief.py        (687 B)
├── device_tests.py      (19,634 B)
├── app/                            ← 应用自身（gallery）
│   ├── common/  components/  view/
│   └── resource/{i18n, images/{controls,icons}, qss/{dark,light}}
├── qfluentwidgets/                  ← 三方纯 Python 包（平铺进同一目录）
│   ├── common/  components/{date_time,dialog_box,layout,material,navigation,settings,widgets}
│   ├── multimedia/  window/  _rc/{i18n, images/..., qss/{dark,light}}
└── qframelesswindow/                ← 又一个三方纯 Python 包
    ├── titlebar/  utils/
```

要点：**入口脚本、兼容补丁、应用包、三方纯 Python 包全部平铺在 `resfile/python/` 一个目录里**，运行时该目录直接落到 `/data/storage/el1/bundle/entry/resources/resfile/python/`，launcher 以 `PyRun_SimpleFileEx` 执行入口脚本，脚本内 `sys.path` 加根目录即可 import 全部平铺包。没有 zipapp、没有单独 site-packages 目录。

### 3.9 组装脚本管线（setup.ps1 五步 + tools 脚本）

`setup.ps1`（仓库根）步骤：

```
0. validate inputs
1. CPython runtime from the hnp package      ← DevPython-OH hnp → entry/libs/arm64-v8a/Python/
2. Qt OHOS SDK runtime                      ← Qt libs + libqohos.so + 插件 → entry/libs/arm64-v8a/
3. PyQt5                                    ← tools/assemble-pyqt5.ps1
4. trim                                      ← tools/trim-runtime.ps1
5. wire the SDK into entry/build-profile.json5   ← 替换 <YOUR_QT_OHOS_SDK> 占位符
   + optional verification
```

- **`tools\assemble-pyqt5.ps1`**：把交叉编译的 PyQt5 模块（`<PyQt5BuildDir>\build\<Mod>\lib<Mod>.so`）+ sdist 纯 Python 部分（`__init__.py`、`pyuic\uic`）+ 注册表 pyqt5_sip wheel 组装进 `entry/libs/arm64-v8a/Python/lib/python3.12/site-packages/PyQt5/`。**sip 改名坑的出处**（原文摘录）：

  > *"The registry wheel ships sip with the LONG platform suffix (sip.cpython-312-aarch64-linux-ohos.so), but the OHOS CPython runtime we embed has the compiled-in EXT_SUFFIX '.cpython-312.so' — importlib's EXTENSION_SUFFIXES ('.cpython-312.so', '.abi3.so', '.so') never match the long name, so `from PyQt5 import sip` (and therefore every PyQt5 import) fails with ModuleNotFoundError. Rename to the short suffix."*

  → `Move-Item sip.cpython-312-aarch64-linux-ohos.so → sip.cpython-312.so`。本次任务 4 对 `site-packages\` 执行了完全相同的改名。

- **`tools\trim-runtime.ps1`**：HAP 裁剪（README-PORT 记录：2053 MB → 470 MB）—— 删除 test/idlelib/tkinter/turtledemo、`.pyc` 重编译、strip 等。OpenShot 最终组装 entry/libs 时必须做同类裁剪。

- **`README-PORT.md`**：PyQt5 5.15.11 交叉编译完整配方（sip-build、`--target-dir`、指定 OHOS 工具链与 Qt）、深色主题桥接说明、真机验证记录（HAD-W24 7.0.0.32）。

---

## 四、CPython 运行时（`cpython\`，任务 2）

来源：DevPython-OH 仓的 hnp 包（`_devpython-oh-repo\` 溯源）。构建特征：**短 EXT_SUFFIX `.cpython-312.so`**（`lib-dynload\array.cpython-312.so` 等验证）。

| 子目录 | 内容 | 文件数 | 大小 |
|--------|------|-------:|-----:|
| `include\` | CPython 3.12 头文件（`python3.12\Python.h`） | 215 | 1.23 MB |
| `lib\` | libpython3.12.so/.so.1.0（6.00 MB）+ libpython3.so stub + 26 个依赖 .so（15 个库，双 soname 副本） | 29 | 22.67 MB |
| `stdlib\` | lib/python3.12 完整内容（stdlib 目录树 + `lib-dynload\` + `site-packages\`） | 35,551 | 1157.45 MB |

`lib\` 全清单（29 文件）：libbz2.so.1.0(.8)、libcryptooh.so.3 (3.48 MB)、libffi.so.8(.1.4)、libgdbm.so.6(.0.0)、libgdbm_compat.so.4(.0.0)、libiconv.so.2(.6.1)、libintl.so.8(.4.0)、liblzma.so.5(.6.3)、libpython3.12.so/.so.1.0 (6.00 MB)、libpython3.so、libreadline.so.8(.2)、libsqlite3.so.0(.8.6)、libssloh.so.3、libtinfow.so.6(.4)、libuuid.so.1(.3.0)、libz.so.1(.3.1) —— 与 demo 验证版 libs 清单一字不差。

> ⚠️ `stdlib\site-packages\`（983.38 MB / 27,802 文件）是 DevPython-OH 自带的数据科学全家桶（numpy 2.2.1、scipy 1.14.1、pandas 2.2.3、matplotlib 3.10.0、opencv 4.10.0、pymupdf、flask、grpcio……共 130+ 个 dist-info，**无 PyQt5**）。**它不是 OpenShot 需要的依赖源，最终组装时严禁整棵拷入 HAP**（原因见第九节）。
>
> 本次引入它的唯一意义：stdlib 其余部分（`lib-dynload`、stdlib 目录树）与 demo 验证版完全同构，可整体复用。

`include\` 215 个头文件与 demo 的 `entry\src\main\cpp\thirdparty\include\python3.12\`（同样 215 个）对应，launcher link 模式直接可用，无需另行获取。

---

## 五、PyQt5 site-packages（任务 3、4）

来源：本机 orange3-ohos 工程 `external\pyqt5-ohos\`（已在 OHOS 真机上验证过的组装产物：自交叉编译 PyQt5 5.15.11 + pypi.cnb.cool 注册表 pyqt5_sip 12.19.0 wheel）。

**为什么不用注册表直接下 PyQt5**：OpenHarmonyPCDeveloper 注册表（pypi.cnb.cool 镜像）上 `pyqt5` **只有 sdist、没有 OHOS wheel**；`pyqt5-sip` 有 `cp312-cp312-ohos_aarch64` wheel。PyQt5 必须自交叉编译（配方见 demo 的 README-PORT.md），而本地 orange3 工程已有编译完成且真机验证的产物，无需重编。

**模块面恰好匹配**：OpenShot 的 PyQt5 依赖面（`D:\Code\1030\openshot-qt\src\qt_api.py` 的 PyQt5 路径：QtCore/QtGui/QtWidgets/QtSvg + uic + sip）正好是 orange3 pyqt5-ohos 的构建面，无缺模块。

### 5.1 site-packages-full\（原始拷贝，48 文件 19.01 MB）

```
PyQt5/
├── __init__.py
├── Qt.abi3.so                      (10.4 KB)
├── QtCore.abi3.so                   (4.58 MB)
├── QtGui.abi3.so                     (4.70 MB)
├── QtWidgets.abi3.so                 (9.06 MB)
├── QtSvg.abi3.so                     (0.20 MB)
├── _QOpenGLFunctions_ES2.abi3.so     (0.13 MB)
├── sip.cpython-312-aarch64-linux-ohos.so   (0.17 MB)  ← 长后缀（原样）
├── pyuic.py(?)/uic/                 ← uic 纯 Python（Compiler/Loader/port_v2/port_v3/widget-plugins）
└── ...
pyqt5_sip-12.19.0.dist-info/         (METADATA/RECORD/WHEEL/top_level.txt/licenses)
```

### 5.2 site-packages\（裁剪+适配版，48 文件 19.01 MB）——最终交付

与 full 版唯一差异：**`sip.cpython-312-aarch64-linux-ohos.so` → `sip.cpython-312.so`**（匹配 DevPython-OH 的编译期 EXT_SUFFIX，与 demo assemble-pyqt5.ps1 的处理一字不差）。

验证记录：

- 7 个 `.so` 全部通过 aarch64 ELF 检查；
- `.abi3.so` 后缀两套 CPython 构建通用（DevPython-OH 与 orange3 构建均接受）；
- PyQt5 **不自带 Qt 运行库**（无 `PyQt5/Qt5/` 目录）—— Qt so 由 HAP libs 提供，无版本冲突风险；
- 源集（orange3 组装产物）本身就是最小面：QtNetwork/QtPrintSupport/QtTest/QtXml 未编译进来，无 numpy/PyOpenGL 等杂物 —— **裁剪步骤实际无可删项**，48 文件保持原样；
- `sip.cpython-312.so` 改名后名称已复核（见上文核对输出）。

> pyqt5_sip 12.19.0 的 wheel（`pyqt5_sip-12.19.0-cp312-cp312-ohos_aarch64.whl`，0.07 MB）同时存档在 `vendor-wheels\`，与 dist-info 的 RECORD 对应可溯源。

---

## 六、vendor 纯 Python 依赖（任务 5）

**`vendor-wheels\`（8 文件，1.07 MB）** —— pip download 拉取的 wheel 存档：

| wheel | 大小 |
|-------|-----:|
| certifi-2026.10.23-py3-none-any.whl | 0.13 MB |
| charset_normalizer-3.4.7-py3-none-any.whl | 0.06 MB |
| defusedxml-0.7.1-py2.py3-none-any.whl | 0.02 MB |
| idna-3.20-py3-none-any.whl | 0.07 MB |
| **pyqt5_sip-12.19.0-cp312-cp312-ohos_aarch64.whl**（溯源存档） | 0.07 MB |
| requests-2.34.2-py3-none-any.whl | 0.07 MB |
| sentry_sdk-2.73.0-py3-none-any.whl | 0.52 MB |
| urllib3-2.8.0-py3-none-any.whl | 0.13 MB |

**`vendor\`（341 文件，3.54 MB）** —— 全部 wheel 解包合并后的最终形态，顶层 7 个包目录 + 7 个 dist-info：

```
certifi/   certifi-2026.10.23.dist-info
charset_normalizer/   charset_normalizer-3.4.7.dist-info
defusedxml/   defusedxml-0.7.1.dist-info
idna/   idna-3.20.dist-info
requests/   requests-2.34.2.dist-info
sentry_sdk/   sentry_sdk-2.73.0.dist-info
urllib3/   urllib3-2.8.0.dist-info
```

全部为纯 Python（py3-none-any / py2.py3-none-any），无平台二进制，可直接与 `site-packages\` 合并部署。
依赖关系：requests → charset_normalizer + idna + urllib3 + certifi；sentry_sdk 与 defusedxml 独立。

---

## 七、遇到的问题与解决（如实记录）

| # | 问题 | 现象/证据 | 解决 |
|---|------|-----------|------|
| 1 | **pyqt5-demo 仓库已消失** | `git clone https://gitcode.com/ohos-qt/pyqt5-demo` → `remote: <CH.00905404> can not read project. user: <redacted>`，退出码 128；遍历 ohos-qt 组织 204 仓确认无此名 | 改用继任仓 **ohos-qt/pyqt-fluent-widgets** @ `8eb0ef1`（其描述与任务目标逐字吻合），克隆为 `pyqt5-demo-ref\` |
| 2 | **PythonForArkUI 仓库已消失** | 同样 404（`_pythonforarkui-clone.log` 保留证据）；组织 497 仓中无此名 | 确认继任者 `Python_Package_For_HarmonyOS`（重构为 HNP 构建档案 + pypi.cnb.cool 注册表）；但其 archives 中 **pyqt5 只有 sdist 无 OHOS wheel**，PyQt5 二进制改取本地 orange3-ohos 工程的真机验证产物 |
| 3 | **KB 的 PythonForArkUI 路线已过时** | KB 页引用的仓库地址失效 | 本报告记录现行生态：DevPython-OH（运行时 hnp）+ OpenHarmonyPCDeveloper 注册表 + 自交叉编译 PyQt5 |
| 4 | **Windows MAX_PATH** | hnp 包内深层路径（site-packages 嵌套）超出 260 字符，常规解包失败 | 用 `tar -xf`（Windows 自带 bsdtar，长路径感知）解包 hnp/目录树 |
| 5 | **sip 模块后缀不匹配** | 注册表 pyqt5_sip wheel 的 sip 是长后缀 `sip.cpython-312-aarch64-linux-ohos.so`（orange3 构建的 EXT_SUFFIX），DevPython-OH 运行时编译期 EXT_SUFFIX 是短 `.cpython-312.so`，importlib 的 EXTENSION_SUFFIXES 匹配不上 → `from PyQt5 import sip` 全链路失败 | 改名 `sip.cpython-312.so`（与 demo `tools\assemble-pyqt5.ps1` 第 4 步的处理一字不差）；`site-packages-full\` 保留原样作对照 |
| 6 | **两套 CPython 构建后缀不一致** | DevPython-OH（短 `.cpython-312.so`）vs orange3 运行时（长 `.cpython-312-aarch64-linux-ohos.so`） | 运行时基准定为 DevPython-OH（demo 同款）；`.abi3.so` 模块两套通用（PyQt5 的 6 个核心模块全是 abi3，天然免疫）；仅 sip 这一个非 abi3 模块需改名。**这是"orange3 的 PyQt5 + DevPython-OH 运行时"新组合，pyqt-fluent-widgets demo 已用同款 sip+DevPython-OH 组合真机验证，风险可控但 OpenShot 首次真机联调需重点关注 `import PyQt5.QtWidgets` 环节** |
| 7 | **pip download 退出码 1 误报** | PowerShell 把 pip 的 stderr 升级提示误判为错误流，`$LASTEXITCODE = 1` 但 7 个 wheel 实际全部下载成功 | 以产物为准（8 个 wheel 落盘校验大小），忽略该误报 |

未编造任何"成功"：两次仓库 404 均如实记录并保留失败日志/改道证据。

---

## 八、OpenShot resfile 布局建议

参照 demo 的 `resfile/python/` 平铺模式（3.8 节），OpenShot 建议如下布局（openshot-qt 源码在 `D:\Code\1030\openshot-qt\src\`，入口 `launch.py`，业务包 classes/、windows/ 等，资源 images/、language/ 等）：

```
entry/src/main/resources/resfile/python/
├── launch.py                 ← 入口（对应 demo 的 fluent_app.py）
│                               由 launcher 的 PyRun_SimpleFileEx 执行
│                               职责：sys.path 补位 → 异常兜底 → 调 openshot-qt 主流程
├── ohos_compat.py            ← OHOS 兼容补丁层（对应 demo 同名文件）
│                               职责：fd/stderr 重定向、HOME/XDG 校准、darkdetect 替身、
│                               平台差异 monkeypatch（对应 demo 的 15KB 同名文件）
├── classes/                  ← openshot-qt/src/classes/**（业务包）
├── windows/                  ← openshot-qt/src/windows/**（窗口模块）
├── images/                   ← openshot-qt/src/images/**（图标资源）
├── language/                 ← openshot-qt/src/language/**（翻译 .qm/.json）
├── (其余 openshot-qt/src 下的 *.py 与包目录按原相对结构平铺)
└── (三方纯 Python 包直接平铺到本目录或并入 site-packages，见下)
```

两条可选路线（推荐 A）：

- **路线 A（贴合 demo）**：三方纯 Python 包（`vendor\` 的 requests/sentry_sdk/defusedxml 等）与 PyQt5 一样进 `entry/libs/arm64-v8a/Python/lib/python3.12/site-packages/`（launcher 已把 site-packages 写进 PYTHONPATH，import 开箱即用）；`resfile/python/` 只放 OpenShot 自身代码 + 资源。
- **路线 B**：全部平铺进 `resfile/python/`，launch.py 里手动 `sys.path.insert`。仅当包极少时才值得，OpenShot 依赖多，不推荐。

布局要点（照抄 demo 的隐含约定）：

1. **入口脚本在 `resfile/python/` 根下**（demo 是 `fluent_app.py`，OpenShot 用 `launch.py`），launcher 的 `kDefaultAppScript` 改为 `.../resfile/python/launch.py`。
2. `resfile` 内**资源目录随包走**（demo 的 `app/resource/{i18n,images,qss}` 在应用包旁边）—— OpenShot 的 `images/`、`language/` 照此平铺，Python 侧相对 `__file__` 定位即可，无需改动 openshot-qt 的资源寻址逻辑。
3. `resfile` 是只读挂载（el1 bundle），**运行期可写数据一律去 `/data/storage/el2/base/files/`**（HOME/XDG 已由 launcher 指过去）。
4. 文件数量注意：resfile 会全量打进 HAP。openshot-qt 的 `classes/`、`windows/` 是纯 Python 没问题；`language/` 的翻译文件、`images/` 的 SVG/PNG 全部保留。

---

## 九、重要警示（组装 entry/libs 前必读）

1. **严禁把 `cpython\stdlib\site-packages\`（983 MB）整棵拷进 entry/libs**。原因：
   - 体积：HAP 会从 ~470 MB 膨胀到 1.5 GB+；
   - **numpy 2.2.1 + `numpy.libs/libopenblasp-*.so` 是 KB 记录的 OHOS 崩溃坑**（openblas 在 OHOS 上有已知的崩溃记录）；
   - 130+ 个包（pandas/matplotlib/opencv/flask/grpcio…）与 OpenShot 无关，只会引入风险。
   **OpenShot 的 site-packages 只放两样东西**：`_staging\python\site-packages\`（裁剪版 PyQt5）+ `_staging\python\vendor\`（7 个纯 Python 包）。
2. stdlib 其余部分拷入 entry/libs 前必须跑裁剪（参考 demo `tools\trim-runtime.ps1`，2053 MB→470 MB）：删除 `test/`、`idlelib/`、`tkinter/`、`turtledemo/`、`ensurepip/_bundled/*.whl`、`config-3.12/` 等，处理 `__pycache__`。
3. **Qt 运行库（libqohos.so、libQt5*.so、iconengines/、imageformats/）不在本次产物中** —— 须按 demo `setup.ps1` 步骤 2 从 Qt OHOS SDK（qt-ohos-arm64-15 系列）stage 进 `entry/libs/arm64-v8a/`。本次 `cpython\lib\` 只覆盖 Python 侧 29 个 .so。
4. `module.json5` 的 deviceTypes **排除 phone**（QTFOROH-1076，见 3.6）。
5. launcher 的 9 条坑清单（3.2 节末尾）逐条照抄，尤其 `Py_InitializeEx(0)`、fd 重定向、PYTHONHOME+PYTHONPATH 双设。
6. OpenShot 自身的**二进制扩展不在本次范围**：openshot 的 C++ 绑定（libopenshot / openshot Python 扩展模块，PyQt5 之外 OpenShot 还依赖 `openshot` 库的 Python 绑定）需要另行交叉编译后放入 site-packages —— 本次只解决了 PyQt5/CPython 运行时面。

---

## 十、后续组装清单（供下一步执行）

| 步 | 内容 | 取材 |
|----|------|------|
| 1 | `entry/libs/arm64-v8a/`：拷入 `cpython\lib\*.so`（29 个，平铺） | 本次产物 |
| 2 | `entry/libs/arm64-v8a/Python/lib/python3.12/`：拷入 `cpython\stdlib\`（**先裁剪：剔除 site-packages、test、idlelib、tkinter、turtledemo**） | 本次产物 + demo trim-runtime.ps1 配方 |
| 3 | `Python/lib/python3.12/site-packages/`：合并 `_staging\python\site-packages\` + `_staging\python\vendor\` | 本次产物 |
| 4 | `entry/libs/arm64-v8a/`：从 Qt OHOS SDK stage libqohos.so + libQt5{Core,Gui,Widgets,Network,OpenGL,PrintSupport,Svg,Test,Xml}.so + libQt5OhosExtras.so + iconengines/ + imageformats/ | Qt OHOS SDK（参照 demo setup.ps1 步骤 2） |
| 5 | launcher：复制 demo 的 cpp，改名（pyqtfluent → openshot），`kDefaultAppScript` 指向 launch.py；thirdparty/include/python3.12 ← `cpython\include\` | 本次产物 + demo |
| 6 | resfile/python/：按第八节布局放入 openshot-qt 代码 + launch.py + ohos_compat.py | openshot-qt 源码 + demo 模式 |
| 7 | module.json5 deviceTypes=["tablet","2in1"]；QtAppConstants.ets 的 APP_LIBRARY_NAME 与 CMake 产物名对齐 | demo 模板 |
| 8 | entry/build-profile.json5：collectAllLibs=true、CMAKE_PREFIX_PATH 指向 Qt OHOS SDK | demo 模板 |
| 9 | OpenShot 二进制扩展（openshot 绑定等）另行交叉编译 | 超出本次范围 |

—— 报告完 ——

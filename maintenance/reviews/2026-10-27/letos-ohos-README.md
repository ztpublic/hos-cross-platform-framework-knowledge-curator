# Letos — HarmonyOS (OHOS) Port

> Fork of [Letos](https://github.com/pawelsalawa/letos) (formerly SQLiteStudio) adding a HarmonyOS / OpenHarmony build via Qt for OHOS.
> Original upstream README preserved in [README_original.md](./README_original.md).

## Status
**Port status:** running on device — 真机运行验证通过（2026-08-15）<br>
**Bundle name:** `com.example.letos`  **Target ABI:** `arm64-v8a` only
**Qt OHOS SDK:** Qt 6.12.0 (prebuilt installer, HarmonyOS arm64_v8a)

## 运行截图
![Letos 主界面](screenshots/letos-ohos.png)
- 设备：2in1 | 验证日期：2026-08-15

## What this fork adds
- **`HarmonyOS/`** — DevEco glue project (AppScope/entry/qEmbeddedUiExtensionHost).
  - `entry/src/main/cpp/CMakeLists.txt` — OHOS cross-compile wrapper, builds `libletos_ohos.so` + `libcoreLetos.so` + `libguiLetos.so` + links Qt6 QPA plugin.
  - `entry/src/main/cpp/sqlite3.h` — shim redirecting `sqlite3_*` → `mc_sqlite3_*` for bundled SQLite3MC amalgamation.
  - `entry/src/main/cpp/patch_needed.py` — post-build DT_NEEDED fixer (paths → bare .so names).
  - `HarmonyOS/patch_and_repack.sh` — one-click build + patch + repackage script.

## Source patches
- `Letos/letos/main.cpp`: `#ifdef Q_OS_OHOS` guards to skip `QProcess::startDetached` restart and `SingleApplication::isSecondary()` check (both incompatible with OHOS process model).

## Prerequisites
1. **Qt 6.12.0 for HarmonyOS** (prebuilt installer: `Qt 6.12.0 → HarmonyOS arm64_v8a` + `MinGW 64-bit` + `CMake + Ninja`). Set env var `QT6_OHOS_SDK` to the `harmonyos_arm64_v8a` directory and `QT6_HOST_PATH` to the `mingw_64` directory.
2. **[DevEco Studio](https://developer.harmonyos.com/cn/develop/deveco-studio/)** (OHOS SDK API 20+).
3. **Python 3** (for `patch_needed.py`).

## Qt6 OHOS SDK runtime dependencies
The Qt6 prebuilt SDK's `.so` files have hard DT_NEEDED on third-party libraries not included in the SDK `lib/`:
- `libicui18n` / `libicuuc` / `libicudata` (Qt6Core)
- `libfontconfig` / `libpng16` / `libfreetype` (Qt6Gui)
- `libbrotlidec` / `libbrotlicommon` (Qt6Network)
- `libexpat` (libfontconfig)

These must be manually copied from the Qt6 additional-packages directory to `HarmonyOS/entry/libs/arm64-v8a/`. See `HarmonyOS/patch_and_repack.sh` for the full list and source paths.

## Build
```bash
cd HarmonyOS && ./setup.ps1 -QtSdk $QT6_OHOS_SDK -HostPath $QT6_HOST_PATH

# Build with DevEco (or hvigorw CLI):
export PATH="${DEVECO_PATH}/jbr/bin:${DEVECO_PATH}/tools/node:$PATH"
export DEVECO_SDK_HOME="${DEVECO_PATH}/sdk"
node "${DEVECO_PATH}/tools/hvigor/bin/hvigorw.js" assembleHap --mode module -p product=default -p buildMode=debug --no-daemon

# Patch DT_NEEDED + repackage HAP:
bash patch_and_repack.sh
```
**Success:** `HarmonyOS/entry/build/default/outputs/default/entry-default-unsigned.hap` + `BUILD SUCCESSFUL`.

## Qt modules used
Core, Gui, Network, Widgets, Svg, UiTools, Xml, Sql, Qml, LinguistTools

## Disabled plugins (OHOS)
- ScriptingTcl (needs Tcl cross-compile)
- ScriptingPython (needs Python cross-compile)
- DbAndroid (needs Android SDK)
- Printing (optional)

## Porting notes
- SQLite3 built from bundled `sqlite3mc_amalgamation.c` (SQLite3 Multiple Ciphers) with `mc_sqlite3_*` API prefix shim.
- `CMAKE_SHARED_LIBRARY_SONAME_FLAG=""` prevents SONAME in `.so` binaries (OHOS expects bare `.so` names).
- QPA plugin linked by bare name (`-lqohos`) to avoid absolute Windows paths in DT_NEEDED.
- Post-build `patch_needed.py` fixes remaining `.so.1` → `.so` and `qtbase/lib/` → bare name references.

## Sync with upstream
```bash
git remote add upstream https://github.com/pawelsalawa/letos.git
git fetch upstream && git merge upstream/master
```

## License
Upstream: GPLv3 (see [LICENSE](./LICENSE) + [README_original.md](./README_original.md)). OHOS glue follows same license.

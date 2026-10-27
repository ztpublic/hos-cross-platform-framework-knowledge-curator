# CodeCountTool HarmonyOS PC Adaptation

This repository keeps the original Qt Widgets desktop application intact and
adds a HarmonyOS PC wrapper under `harmony_pc/`.

The project should be adapted with Qt, not Electron. CodeCountTool is already a
small C++/Qt Widgets application, and its core work is native file traversal,
text reading, table display, and progress updates. Rewriting it as Electron
would replace the working UI and counting logic with a new JavaScript app for
no clear benefit. The HarmonyOS version therefore follows the existing Qt
adaptation pattern used by the local `ohos_phototonic`, `ohos_KTouch`, and
`ohos_LANDrop` examples: ArkTS hosts an XComponent, the Qt OpenHarmony QPA
plugin starts `libentry.so`, and the same C++/Qt source files are built into
that native library.

## Layout

- `main.cpp`, `CountCode.cpp/.h`, `CountCode.ui`, `RippleButton/`, `res.qrc`:
  shared desktop and HarmonyOS Qt code.
- `CMakeLists.txt`, `RippleButton/CMakeLists.txt`: desktop CMake build, now
  compatible with Qt 5 and Qt 6.
- `harmony_pc/`: HarmonyOS PC Stage project opened by DevEco Studio.
- `harmony_pc/entry/src/main/cpp/CMakeLists.txt`: native Harmony build entry.
  It references the parent CodeCountTool sources and builds `libentry.so`.
- `harmony_pc/entry/libs/arm64-v8a/libplugins_platforms_qopenharmony.so`: Qt
  OpenHarmony platform plugin packaged with the HarmonyOS entry module.

## Qt SDK

The entry build profile points to the project-local SDK:

```json5
"arguments": "-DQT_PREFIX=qtforharmony_sdk"
```

From `harmony_pc/`, this resolves to:

```text
harmony_pc/qtforharmony_sdk
```

Do not point this project at another person's SDK directory. Copy
`qtforharmony_sdk` into `harmony_pc/qtforharmony_sdk` before building.

## Porting Notes

- `main.cpp` exports both `main(int, char **)` and `qtmain(int, char **)` for
  the Qt OpenHarmony runtime.
- `QOhWidgetHelper` is enabled on HarmonyOS PC for full-screen, split-screen,
  floating-window modes, and window geometry persistence.
- The desktop build remains a normal `QApplication` executable.
- `RippleButton` was made Qt 5/Qt 6 compatible because the local Qt for
  HarmonyOS SDK is Qt 5.15.12 while the upstream project targeted Qt 6 APIs.
- HarmonyOS PC applies a startup/UI scale close to the upstream screenshots so
  the table, labels, inputs, progress bars, and buttons do not render at the
  tiny raw 864x637 designer size.
- Windows-only `.rc` resources and MSVC linker pragmas are now guarded so the
  HarmonyOS/clang build does not consume them.
- Counting logic now avoids division by zero when no matching file is found and
  uses the actual matched file count in the summary.
- Directory traversal and line counting run on a background worker thread
  (`CountCodeWorker` on a `QThread`). The UI stays responsive on large trees,
  shows live progress in the table area, and the clear button turns into a
  cancel button while a count is running.
- The user-selected root directory is exempt from the build/bin/debug/release
  directory-exclusion rules; only subdirectories are filtered, so picking a
  project folder named e.g. `build_tools` still produces results.

## Build

Desktop check:

```sh
cmake -S . -B build
cmake --build build
```

HarmonyOS PC:

1. Open `harmony_pc/` in DevEco Studio.
2. Configure signing for your local HarmonyOS developer account.
3. Confirm the `QT_PREFIX` argument in `harmony_pc/entry/build-profile.json5`.
4. Build or run the `entry` module on a HarmonyOS PC device.

The final HAP should contain at least:

```text
libentry.so
libplugins_platforms_qopenharmony.so
libQt5Core.so
libQt5Gui.so
libQt5Widgets.so
libQt5OhExtras.so
```

## Current Scope

T0/T1 features for the first HarmonyOS PC build:

- Start, show the main Qt Widgets window, and exit.
- Open one or more source files through `QFileDialog`.
- Open a directory through `QFileDialog`.
- Apply suffix filters and excluded-directory filters.
- Traverse directories recursively and list matching files.
- Count total lines, code lines, comment lines, blank lines, byte size, and
  percentages.
- Clear and recount after filter changes.

Verified on real hardware (HarmonyOS PC):

- Native `QFileDialog` opens the system picker; selected directory URIs are
  normalized and readable through Qt file APIs.
- A 1500-file source tree scans on the background thread with the UI
  responsive throughout; totals match the expected line counts exactly.
- A root directory whose name starts with `build` is counted correctly while
  its `build*`-named subdirectories are still excluded.

Items to verify on real hardware:

- Window resizing and split/floating modes through `QOhWidgetHelper`.

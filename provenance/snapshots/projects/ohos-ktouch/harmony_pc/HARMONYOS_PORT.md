# KTouch HarmonyOS PC Adaptation

This HarmonyOS project follows the proven Qt-for-Harmony wrapper route used by
the Phototonic port. The desktop KTouch source tree remains the owner of the
application logic; `harmony_pc/` only supplies the HarmonyOS Stage Ability,
XComponent host, native CMake entry, bundled Qt-for-Harmony SDK, and a small
set of compatibility shims.

## Fastest Route

The first target is a HarmonyOS MVP, not a full KDE Frameworks port:

1. Keep the existing Qt/C++/QML KTouch application.
2. Build it into `libentry.so` with the Qt 5.15.12 for HarmonyOS SDK bundled in
   `harmony_pc/qtforharmony_sdk`.
3. Reuse the Phototonic-style OpenHarmony QPA wrapper and
   `libplugins_platforms_qopenharmony.so`.
4. Replace KDE Frameworks dependencies with small local shims where the runtime
   needs only simple behavior.
5. Disable editor/preferences/KCM flows that depend on larger KDE modules for
   the first HAP.
6. Package a tiny built-in data set first, then expand courses and keyboard
   layouts after the app is installable.

This gets a device-testable build much faster than rewriting the app in ArkUI or
cross-porting KF6.

## Project Layout

- `harmony_pc/build-profile.json5`: app-level HarmonyOS build profile.
- `harmony_pc/entry/build-profile.json5`: entry module build profile. The native
  build points at `entry/src/main/cpp/CMakeLists.txt`.
- `harmony_pc/entry/src/main/ets`: ArkTS Ability, AbilityStage, and XComponent
  host page.
- `harmony_pc/entry/src/main/cpp/CMakeLists.txt`: native build entry for
  `libentry.so`; it points back to the parent KTouch source tree.
- `harmony_pc/entry/src/main/cpp/openharmony_compat`: KDE/Qt compatibility
  shims used only by the Harmony MVP build.
- `harmony_pc/entry/src/main/cpp/openharmony_data`: minimal built-in KTouch data
  bundle for the first HAP.

## Qt SDK

The module build profile uses the bundled SDK:

```json5
"arguments": "-DQT_PREFIX=qtforharmony_sdk"
```

Change `harmony_pc/entry/build-profile.json5` only if the Qt for HarmonyOS SDK is
moved.

## Native Build Strategy

The native library is built with:

```cmake
add_library(entry SHARED ...)
```

The Harmony MVP compile definitions are:

```cmake
KTOUCH_HARMONY_MVP
OPENHARMONY
NAPI_DISABLE_CPP_EXCEPTIONS
```

The MVP source list includes the KTouch runtime, models, QML, images, profile
data access, and training logic. It intentionally excludes the course/resource
editor and KDE configuration UI.

The exported native entry points are:

```cpp
extern "C" int main(int argc, char **argv);
extern "C" int qtmain(int argc, char **argv);
```

The wrapper currently starts `libentry.so`, and this keeps compatibility with
Qt-for-Harmony examples that resolve either symbol.

## Compatibility Shims

The first HAP avoids pulling KDE Frameworks into the Harmony native toolchain.
The shims in `openharmony_compat/` cover only what the MVP needs:

- `KLocalizedString`: returns untranslated strings and supports simple `%1`
  substitution.
- `OhosI18nContext`: exposes `i18n()` and `i18nc()` to QML as root-context
  methods.
- `KAboutData`: supplies a display name for error dialogs.
- `KColorScheme`: maps KTouch color roles to a Qt palette-based fallback.
- `KCategorizedSortFilterProxyModel`: provides the category role constants used
  by resource models.
- `preferences.h`: persists a small subset of preferences with `QSettings`.

QML fallback modules are packaged under `qrc:/ktouch/qml/org/kde/...` for:

- `org.kde.kirigami`
- `org.kde.coreaddons`
- `org.kde.charts`

## Data Strategy

The MVP packages only enough data to launch and train:

- `openharmony_data/data_ohos.xml`
- `openharmony_data/course_us.xml`
- `openharmony_data/keyboardlayout_us.xml`
- the matching XML schemata

Under `KTOUCH_HARMONY_MVP`, `ResourceDataAccess` reads from
`qrc:/ktouch/ohos-data` and skips libxml2 schema validation. Expanding the HAP to
all KTouch courses is a data-copy task once the first device build is stable.

## HarmonyOS Behavior

- The app launches from the normal home-screen ability.
- The module no longer registers Phototonic's image file-open abilities.
- The native window name is `ktouch_main_window`.
- The Qt wrapper launches `libentry.so` without external file parameters.
- Editor, shortcut, keyboard KCM, and full KDE preferences dialogs are disabled
  in the MVP. The menu keeps a fullscreen toggle.

## Build

Open this directory in DevEco Studio:

```text
harmony_pc/
```

Then build the `entry` HAP.

Command-line build can be attempted when the HarmonyOS command-line tools are
installed:

```sh
cd harmony_pc
hvigorw assembleHap
```

The current command-line output is an unsigned debug HAP:

```text
harmony_pc/entry/build/default/outputs/default/entry-default-unsigned.hap
```

Configure DevEco automatic signing before installing to a physical device. A
direct `hdc install -r` of the unsigned HAP is rejected by the device with
`no signature file`.

The final HAP should contain at least:

```text
libentry.so
libplugins_platforms_qopenharmony.so
libQt5Core.so
libQt5Gui.so
libQt5Widgets.so
libQt5Quick.so
libQt5Qml.so
libQt5QuickWidgets.so
libQt5QuickControls2.so
libQt5Sql.so
libQt5Svg.so
libQt5Xml.so
libQt5OhExtras.so
```

## Current Verification

Local checks performed on macOS:

```sh
cmake -S harmony_pc/entry/src/main/cpp -B build/ktouch-ohos-cmake-check -DQT_PREFIX=qtforharmony_sdk -DCMAKE_BUILD_TYPE=Debug
cmake --build build/ktouch-ohos-cmake-check --target entry -j2
harmony_pc/qtforharmony_sdk/bin/qmllint -I src/qml -I harmony_pc/qtforharmony_sdk/qml src/qml/main.qml
/Users/luqingjiedemac/ohos/command-line-tools/bin/hvigorw assembleHap
```

The host CMake check compiles all KTouch MVP sources, moc files, and rcc
resources, then stops at host-side linking because the macOS linker cannot
resolve the HarmonyOS system library `ace_napi.z`.

The real HarmonyOS command-line build succeeds with the DevEco/HarmonyOS
toolchain and packages a 36M HAP containing `libentry.so`,
`libplugins_platforms_qopenharmony.so`, and the required Qt shared libraries.
The remaining device-install step is signing configuration.

## Next Work

- Configure a DevEco signing profile and install the HAP on a HarmonyOS
  PC/tablet target.
- Verify that QPA calls the exported native entry point.
- Test QML runtime loading, especially the fallback `org.kde.*` modules.
- Expand packaged course and keyboard layout data beyond the US MVP set.
- Decide whether to continue the shim route or port selected KDE Frameworks
  pieces after the app has a working HAP.

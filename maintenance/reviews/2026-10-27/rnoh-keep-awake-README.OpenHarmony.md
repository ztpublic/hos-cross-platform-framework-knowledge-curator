# React Native KeepAwake · HarmonyOS / OpenHarmony Adaptation

> English adaptation guide for `@react-native-ohos-demo/keep-awake`.
> 中文版本：[README.OpenHarmony_CN.md](./README.OpenHarmony_CN.md)

The HarmonyOS/OpenHarmony port of `@thehale/react-native-keep-awake`, used to prevent the screen from sleeping / auto-locking. All 4 upstream named exports are kept, so consumer code is reused unchanged; on top of that, this package adds a **default-export object** (`import keepAwake from …`) that upstream does not have — an additive superset that does not break any existing import.

- Upstream: <https://github.com/thehale/react-native-keep-awake> (v0.2.0, License MPL-2.0)
- HarmonyOS system capability: `window.Window.setWindowKeepScreenOn(isKeepScreenOn: boolean)`
- Target framework: React Native for OpenHarmony (RNOH) **0.82.30**, aligned with react-native **0.82.1**
- Deliverable form: self-contained `harmony/keep_awake` HAR module + prebuilt `harmony/keep_awake.har` + `harmony.autolinking` in `package.json` (auto-linked, no codegen needed on the host)

## Version matrix

| react-native | @react-native-oh/react-native-harmony | @rnoh/react-native-openharmony (ohpm) | Compile SDK |
| --- | --- | --- | --- |
| 0.82.1 | 0.82.30 | 0.82.30 | 6.0.0(20) / API 20+ |

## Installation

1. Install the npm package (demo package name `@react-native-ohos-demo/keep-awake`; replace with the official name after publishing to the org):

   ```bash
   npm install @react-native-ohos-demo/keep-awake
   ```

2. Auto-link the HarmonyOS project (writes the HAR dependency, ArkTS/C++ package factories, and `autolinking.cmake` into the harmony project):

   ```bash
   npx react-native link-harmony --harmony-project-path ./harmony
   ```

   > If the project already integrates `@rnoh/hvigor-plugin`, linking runs automatically at build time and this step can be skipped.

3. Install HarmonyOS-side dependencies:

   ```bash
   cd harmony && ohpm install --all
   ```

## Usage

```tsx
import { activate, deactivate, useKeepAwake, KeepAwake } from '@react-native-ohos-demo/keep-awake';

activate();          // keep the screen on
deactivate();        // stop keeping awake (follow system again)
useKeepAwake();      // inside a component: on on mount, off on unmount
<KeepAwake />        // same, JSX form
```

## Implementation (RNOH two-layer architecture)

- **ArkTS layer** `harmony/keep_awake/src/main/ets/KeepAwakeTurboModule.ets`
  `extends UITurboModule`, module name `KeepAwake`. It resolves the RN main window via `ctx.rnInstance.getRNWindow()` and then calls `setWindowKeepScreenOn(true/false)`. Because `getRNWindow()` returns a Promise, the actual call completes in a microtask, with failures caught and logged via `logger.error`.
- **ArkTS registration** `KeepAwakePackage.ets`: `extends RNOHPackage`, overriding `getUITurboModuleFactoryByNameMap()` with key = `KeepAwake`. `Index.ets` provides both named and default exports so autolinking can use the default import.
- **C++ layer** `src/main/cpp/ReactNativeKeepAwakePackage.h`: defines `KeepAwake : public ArkTSTurboModule` and registers, in the constructor, `methodMap_ = { ARK_METHOD_METADATA(activate, 0), ARK_METHOD_METADATA(deactivate, 0) }`; `ReactNativeKeepAwakePackage : public Package` supplies the `TurboModuleFactoryDelegate`.
  > ⚠️ RNOH exposes methods through `methodMap_` metadata; **a generic `ArkTSTurboModule` does not automatically expose ArkTS methods as JS functions** (you would get `activate is not a function`). This library hard-codes the codegen-equivalent method metadata into cpp to stay self-contained.
- **autolinking** `package.json`:

  ```json
  "harmony": {
    "autolinking": {
      "ohPackageName": "@rnoh/react-native-keep-awake",
      "etsPackageClassName": "KeepAwakePackage",
      "cppPackageClassName": "ReactNativeKeepAwakePackage",
      "cmakeLibraryTargetName": "rnoh__react_native_keep_awake"
    }
  }
  ```

## Parity with Android / iOS

| API | Android | iOS | HarmonyOS | Result |
| --- | --- | --- | --- | --- |
| `activate()` | `window.addFlags(FLAG_KEEP_SCREEN_ON)` (UI thread) | `idleTimerDisabled = YES` (main queue) | `getRNWindow().setWindowKeepScreenOn(true)` | ✅ Essentially equivalent |
| `deactivate()` | `window.clearFlags(FLAG_KEEP_SCREEN_ON)` | `idleTimerDisabled = NO` | `setWindowKeepScreenOn(false)` | ✅ Essentially equivalent |

HarmonyOS `setWindowKeepScreenOn` is a **per-window** toggle, closest in semantics to Android's per-window `FLAG_KEEP_SCREEN_ON`; it is an absolute boolean (not reference-counted), matching upstream. See the code-check report [RN_react-native-keep-awake+代码检查报告.md](./RN_react-native-keep-awake%2B代码检查报告.md).

## On-device verification

- Device: nova 12 (HarmonyOS 6.1.0, API 24)
- Result: tap "Keep awake on" → `Creating Turbo Module: KeepAwake` → window manager receives `OnSetWindowKeepScreenOn isKeepScreenOn: 1`; tap "Keep awake off" → `isKeepScreenOn: 0`. Auto-linking resolves correctly, no `Couldn't find Turbo Module 'KeepAwake'`.

## Known differences / notes

- `getRNWindow()` is async: `activate()` returns `void` synchronously, and the actual flag is set inside its Promise callback (equivalent to Android/iOS posting to the UI thread).
- Applies only to the RN main window; multi-window / sub-window scenarios require custom extension.

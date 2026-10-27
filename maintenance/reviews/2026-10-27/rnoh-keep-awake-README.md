# @react-native-ohos-demo/keep-awake

`@thehale/react-native-keep-awake` 的鸿蒙化（RNOH）适配版，用于阻止屏幕自动熄灭/锁屏。JS API 与上游完全一致，业务代码可无缝复用。

- 上游：<https://github.com/thehale/react-native-keep-awake>（v0.2.0，MPL-2.0）
- 鸿蒙系统能力：`window.Window.setWindowKeepScreenOn(boolean)`
- 适配框架：React Native for OpenHarmony（RNOH）**0.82.30**，对应 react-native **0.82.1**

## 文档

- 鸿蒙适配（中）：[README.OpenHarmony_CN.md](./README.OpenHarmony_CN.md)
- HarmonyOS adaptation (EN): [README.OpenHarmony.md](./README.OpenHarmony.md)
- 逐接口一致性审查：[RN_react-native-keep-awake+代码检查报告.md](./RN_react-native-keep-awake%2B代码检查报告.md)

## JS API（与上游一致）

```ts
import { activate, deactivate, useKeepAwake, KeepAwake } from '@react-native-ohos-demo/keep-awake';

activate();          // 开启屏幕常亮
deactivate();        // 关闭
useKeepAwake();      // 组件内自动开启、卸载时关闭
<KeepAwake />        // 同上，JSX 形式
```

## 交付形态（可发布）

本库为**库内自包含**的鸿蒙适配，宿主无需手动接线、无需跑 codegen：

- `harmony/keep_awake/`：HAR 模块源码（ArkTS `KeepAwakeTurboModule.ets`/`KeepAwakePackage.ets` + C++ `ReactNativeKeepAwakePackage.h`（含 `methodMap_` 方法登记）+ `CMakeLists.txt` + `Index.ets`）
- `harmony/keep_awake.har`：预构建产物，供 autolinking/ohpm 直接引用
- `package.json` 的 `harmony.autolinking`：自动链接配置

接入方式（详见适配文档）：

```bash
npm install @react-native-ohos-demo/keep-awake
npx react-native link-harmony --harmony-project-path ./harmony
cd harmony && ohpm install --all
```

## 适配说明

上游 TurboModule Spec（`src/specs/v2/NativeKeepAwake.ts`）仅两个同步方法 `activate()` / `deactivate()`，无参数、无事件、无第三方原生 SDK 依赖，属最易适配的一类。鸿蒙侧通过 `UITurboModule.ctx.rnInstance.getRNWindow()` 拿到主窗口后调用 `setWindowKeepScreenOn`，语义与 Android `FLAG_KEEP_SCREEN_ON` / iOS `idleTimerDisabled` 等价，已在真机（nova 12）验证 `isKeepScreenOn` 置位/复位可逆通过。

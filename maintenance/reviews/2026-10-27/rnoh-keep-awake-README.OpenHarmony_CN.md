# React Native KeepAwake · 鸿蒙（HarmonyOS / OpenHarmony）适配

> 本文件为 `@react-native-ohos-demo/keep-awake` 的 HarmonyOS/OpenHarmony 适配说明（中文）。
> English version: [README.OpenHarmony.md](./README.OpenHarmony.md)

`@thehale/react-native-keep-awake` 的鸿蒙化版本，用于阻止屏幕自动熄灭/锁屏。JS API 覆盖上游的 4 个具名导出，业务代码可无缝复用；此外本包**额外提供**默认导出对象（`import keepAwake from …`），上游只有具名导出、没有默认导出——属超集，不影响既有 import 写法。

- 上游：<https://github.com/thehale/react-native-keep-awake>（v0.2.0，License MPL-2.0）
- 鸿蒙系统能力：`window.Window.setWindowKeepScreenOn(isKeepScreenOn: boolean)`
- 适配框架：React Native for OpenHarmony（RNOH）**0.82.30**，对应 react-native **0.82.1**
- 交付形态：库内自包含 `harmony/keep_awake` HAR 模块 + `harmony/keep_awake.har` 预构建产物 + `package.json` 的 `harmony.autolinking`（自动链接，无需在宿主跑 codegen）

## 版本配套表

| react-native | @react-native-oh/react-native-harmony | @rnoh/react-native-openharmony（ohpm） | Compile SDK |
| --- | --- | --- | --- |
| 0.82.1 | 0.82.30 | 0.82.30 | 6.0.0(20) / API 20+ |

## 安装

1. 安装 npm 包（本示例包名 `@react-native-ohos-demo/keep-awake`；发布到组织后替换为正式包名）：

   ```bash
   npm install @react-native-ohos-demo/keep-awake
   ```

2. 自动链接鸿蒙工程（把 HAR 依赖、ArkTS/C++ 包工厂、autolinking.cmake 写入鸿蒙工程）：

   ```bash
   npx react-native link-harmony --harmony-project-path ./harmony
   ```

   > 若工程已集成 `@rnoh/hvigor-plugin`，link 会在构建时自动执行，可跳过此步。

3. 安装鸿蒙侧依赖：

   ```bash
   cd harmony && ohpm install --all
   ```

## 用法

```tsx
import { activate, deactivate, useKeepAwake, KeepAwake } from '@react-native-ohos-demo/keep-awake';

activate();          // 开启屏幕常亮
deactivate();        // 关闭常亮（恢复跟随系统）
useKeepAwake();      // 在组件内使用：挂载时开启、卸载时自动关闭
<KeepAwake />        // 同上，JSX 形式
```

## 实现说明（RNOH 双层结构）

- **ArkTS 层** `harmony/keep_awake/src/main/ets/KeepAwakeTurboModule.ets`
  `extends UITurboModule`，模块名 `KeepAwake`。通过 `ctx.rnInstance.getRNWindow()` 拿到 RN 主窗口后调用 `setWindowKeepScreenOn(true/false)`；`getRNWindow()` 返回 Promise，故实际调用在微任务中完成，并对失败做 `logger.error` 兜底。
- **ArkTS 注册** `KeepAwakePackage.ets`：`extends RNOHPackage`，重写 `getUITurboModuleFactoryByNameMap()`，key = `KeepAwake`。`Index.ets` 具名 + 默认双导出，供 autolinking 默认导入。
- **C++ 层** `src/main/cpp/ReactNativeKeepAwakePackage.h`：定义 `KeepAwake : public ArkTSTurboModule` 并在构造中登记 `methodMap_ = { ARK_METHOD_METADATA(activate, 0), ARK_METHOD_METADATA(deactivate, 0) }`；`ReactNativeKeepAwakePackage : public Package` 提供 `TurboModuleFactoryDelegate`。
  > ⚠️ RNOH 的方法暴露依赖 `methodMap_` 元数据，**泛型 `ArkTSTurboModule` 不会自动把 ArkTS 方法暴露为 JS 函数**（会得到 `activate is not a function`）。本库已把 codegen 等价的方法元数据固化进 cpp，保持库自包含。
- **autolinking** `package.json`：

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

## 与 Android / iOS 的一致性

| 接口 | Android | iOS | 鸿蒙 | 结论 |
|------|---------|-----|------|------|
| `activate()` | `window.addFlags(FLAG_KEEP_SCREEN_ON)`（UI 线程） | `idleTimerDisabled = YES`（主队列） | `getRNWindow().setWindowKeepScreenOn(true)` | ✅ 基本一致 |
| `deactivate()` | `window.clearFlags(FLAG_KEEP_SCREEN_ON)` | `idleTimerDisabled = NO` | `setWindowKeepScreenOn(false)` | ✅ 基本一致 |

鸿蒙 `setWindowKeepScreenOn` 为**窗口级**开关，与 Android 的 per-window `FLAG_KEEP_SCREEN_ON` 语义最接近；为绝对布尔值（非引用计数），与上游一致。详见代码检查报告 [RN_react-native-keep-awake+代码检查报告.md](./RN_react-native-keep-awake%2B代码检查报告.md)。

## 真机验证

- 设备：nova 12（HarmonyOS 6.1.0，API 24）
- 结果：点击「开启常亮」→ `Creating Turbo Module: KeepAwake` → 窗口管理收到 `OnSetWindowKeepScreenOn isKeepScreenOn: 1`；点击「关闭常亮」→ `isKeepScreenOn: 0`。自动链接解析成功，无 `Couldn't find Turbo Module 'KeepAwake'`。

## 已知差异 / 注意

- `getRNWindow()` 为异步：`activate()` 同步返回 void，实际置位在其 Promise 回调内完成（与 Android/iOS 的 post 到 UI 线程等效）。
- 仅作用于 RN 主窗口；若业务有多窗口/子窗口需求需自行扩展。

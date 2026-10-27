# expo-device - HarmonyOS / OpenHarmony 支持

[English](README.OpenHarmony.md) | 简体中文

基于 `expo-device@57.0.2` 的 React Native for OpenHarmony 交付包，设备型号、品牌、内存、系统版本等物理信息读取。包含 OpenHarmony 原生实现。交付版本 `57.0.2-ohos-1.0.0`。

上游：https://github.com/expo/expo/tree/9e5319c0f821a27b7924841903abae50e2b41790/packages/expo-device；许可证：MIT。社区：[React Native for OpenHarmony](https://atomgit.com/oh-react-native)。

## 能力一览

| 能力 | iOS | Android | 鸿蒙 | 说明 |
| --- | --- | --- | --- | --- |
| 设备常量 | 支持 | 支持 | 支持 | 模块初始化时读取并缓存系统设备信息。 |
| getDeviceTypeAsync | 支持 | 支持 | 支持 | 返回 UNKNOWN、PHONE、TABLET、DESKTOP 或 TV。 |
| getUptimeAsync | 支持 | 支持 | 支持 | 返回自启动以来的运行时长，单位为毫秒。 |
| getMaxMemoryAsync | Android | Android | 支持 | 返回应用运行时堆上限，单位为字节。 |
| 平台特性 | 有差异 | 支持 | 降级 | 鸿蒙当前返回空数组及 false。 |
| Root / 侧载检测 | 有差异 | 支持 | 不支持 | 鸿蒙实现会以 Promise reject 明确报告不可用。 |

iOS/Android 的支持范围以上游同版本为准；本仓库只声明鸿蒙列，未复测 iOS/Android。

> **关键事实**：这是包含原生实现的鸿蒙适配包，随包提供 [expo_device.har](harmony/expo_device.har)，需要 RNOH 自动链接和 ohpm 依赖安装。

## 版本配套

| React Native | React | RNOH | Compile SDK | DevEco Studio | 受测系统 |
| --- | --- | --- | --- | --- | --- |
| 0.84.1 | 19.2.3 | 0.84.3 | 26.0.0 | 26.0.0 Release | OpenHarmony-7.0.0.105 |

此表只记录实际使用过的组合，不表示其他版本已兼容。

## 快速开始

### 安装

在已有 RNOH 宿主工程中执行：

```sh
npm install git+https://atomgit.com/oh-react-native/expo-device.git#57.0.2-ohos-1.0.0
./node_modules/.bin/react-native link-harmony
cd harmony
ohpm install --all
```

HAR 位于 `node_modules/expo-device/harmony/expo_device.har`。若宿主 CMake 从 `entry/oh_modules` 加载依赖，在 `harmony/entry/oh-package.json5` 增加 `"@react-native-ohos/expo-device": "file:../../node_modules/expo-device/harmony/expo_device.har"` 后再执行 `ohpm install --all`。

### 使用示例

```ts
import * as Device from 'expo-device';

const summary = {
  name: Device.deviceName,
  model: Device.modelName,
  os: Device.osVersion,
  memory: Device.totalMemory,
};
const uptime = await Device.getUptimeAsync();
```

## 接口说明

| 名称 | 类型 | 来源 | 鸿蒙支持 |
| --- | --- | --- | --- |
| `getDeviceTypeAsync` | function(0 参数) | 导出函数 | 待验证 |
| `getUptimeAsync` | function(0 参数) | 导出函数 | 待验证 |
| `getMaxMemoryAsync` | function(0 参数) | 导出函数 | 待验证 |
| `isRootedExperimentalAsync` | function(0 参数) | 导出函数 | 待验证 |
| `isSideLoadingEnabledAsync` | function(0 参数) | 导出函数 | 待验证 |
| `getPlatformFeaturesAsync` | function(0 参数) | 导出函数 | 待验证 |
| `hasPlatformFeatureAsync` | function(1 参数) | 导出函数 | 待验证 |
| `DeviceType` | 具名导出 | 导出 | 待验证 |
| `isDevice` | 具名导出 | 导出 | 待验证 |
| `brand` | 具名导出 | 导出 | 待验证 |
| `manufacturer` | 具名导出 | 导出 | 待验证 |
| `modelId` | 具名导出 | 导出 | 待验证 |
| `modelName` | 具名导出 | 导出 | 待验证 |
| `designName` | 具名导出 | 导出 | 待验证 |
| `productName` | 具名导出 | 导出 | 待验证 |
| `deviceType` | 具名导出 | 导出 | 待验证 |
| `deviceYearClass` | 具名导出 | 导出 | 待验证 |
| `totalMemory` | 具名导出 | 导出 | 待验证 |
| `supportedCpuArchitectures` | 具名导出 | 导出 | 待验证 |
| `osName` | 具名导出 | 导出 | 待验证 |
| `osVersion` | 具名导出 | 导出 | 待验证 |
| `osBuildId` | 具名导出 | 导出 | 待验证 |
| `osInternalBuildId` | 具名导出 | 导出 | 待验证 |
| `osBuildFingerprint` | 具名导出 | 导出 | 待验证 |
| `platformApiLevel` | 具名导出 | 导出 | 待验证 |
| `deviceName` | 具名导出 | 导出 | 待验证 |

以上 26 条取自 `src/index.ts` 的导出、`spec.json` 的方法登记与属性声明。完整属性与平台差异以 [上游 57.0.2 文档](https://github.com/expo/expo/tree/9e5319c0f821a27b7924841903abae50e2b41790/packages/expo-device) 为准。

## 注意事项

- 静态常量与异步查询取值不同：常量在模块初始化时读取，异步方法每次重新查询。不可用的能力返回上游约定的默认值并向调用方传播错误。
- 需要 RNOH 自动链接与 ohpm 依赖安装；HAR 已随包提供于 `harmony/expo_device.har`，消费方无需自行构建。

## 验证记录

构建组合：React Native 0.84.1、React 19.2.3、RNOH 0.84.3、Compile SDK 26.0.0、DevEco Studio 26.0.0 Release。

**本版本未完成真机验证。** 已完成：npm 归档清单核对、包级契约测试（2）、Metro source map 入口检查。原因：当时宿主机未连接可用的鸿蒙设备，未执行安装与业务操作。

README 不将 Metro 通过或 HAP 构建通过表述为真机通过。

## 许可证

[MIT](LICENSE)。

许可差异和第三方声明以本仓库保留的原文为准。

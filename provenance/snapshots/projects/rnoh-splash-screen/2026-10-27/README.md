> 模板版本：v0.4.2

<p align="center">
  <h1 align="center"> <code>react-native-splash-screen</code> </h1>
</p>

本项目基于 [react-native-splash-screen](https://github.com/crazycodeboy/react-native-splash-screen) 开发。

该第三方库的仓库已迁移至 Gitcode，且支持直接从 npm 下载，新的包名为：`@react-native-ohos/react-native-splash-screen` 版本所属关系如下：

| 三方库名称    | 三方库版本（npm地址）    | 发布信息     | 支持RN版本    | Autolink     | 编译API版本     | 社区基线版本    |  源码地址   |
| ------------ | ------------ | ------------------------------ | ------------- | ------------- |------------------------ | ------------- | ------------- | 
| @react-native-ohos/react-native-splash-screen | [~3.5.0](https://www.npmjs.com/package/@react-native-ohos/react-native-splash-screen)   | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-splash-screen/releases) | 0.82.* / 0.84.* | 是 | API12+ | 3.3.0 | [master](https://gitcode.com/CPF-RN/rntpc_react-native-splash-screen/tree/master) |
| @react-native-ohos/react-native-splash-screen | [~3.4.0](https://www.npmjs.com/package/@react-native-ohos/react-native-splash-screen)| [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-splash-screen/releases) | 0.77 | 否 | API12+ | 3.3.0 | [br_rnoh0.77](https://gitcode.com/CPF-RN/rntpc_react-native-splash-screen/tree/br_rnoh0.77)  |
| @react-native-ohos/react-native-splash-screen | [~3.3.2](https://www.npmjs.com/package/@react-native-ohos/react-native-splash-screen)    | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-splash-screen/releases) | 0.72 | 是 | API12+ | 3.3.0 | [br_rnoh0.72](https://gitcode.com/CPF-RN/rntpc_react-native-splash-screen/tree/br_rnoh0.72)  |
| @react-native-oh-tpl/react-native-splash-screen | [<= 3.3.0-0.0.2@deprecated](https://www.npmjs.com/package/@react-native-oh-tpl/react-native-splash-screen)  | [Github Releases(deprecated)](https://github.com/react-native-oh-library/react-native-splash-screen/releases) | 0.72      | 否 | API12+ | 3.3.0 | [sig](https://github.com/react-native-oh-library/react-native-splash-screen/tree/sig) |

## 简介

`react-native-splash-screen` 是一个用于 React Native 的启动页组件库。<br/>
它提供在应用加载完成前显示启动页、并在就绪后隐藏启动页的能力。

## 下载安装

进入到工程目录并输入以下命令：

<!-- tabs:start -->
**npm**

```bash
npm install @react-native-ohos/react-native-splash-screen
```

**yarn**

```bash
yarn add @react-native-ohos/react-native-splash-screen
```

<!-- tabs:end -->

下面的代码展示了这个库的基本使用场景：

> [!WARNING] 使用时 import 的库名不变。


##### **在 entry/src/main/ets/pages 目录下新建启动页SplashScreenPage.ets**

```ts
import { SplashScreenView } from '@react-native-ohos/react-native-splash-screen/src/main/ets/SplashScreenView'

@Entry
@Component
export struct SplashScreenPage {
  build() {
    Row() {
      SplashScreenView();
    }
  }
}
```

##### **在 entry/src/resources/base/profile/main_pages.json 内添加 `pages/SplashScreenPage`**
```json
{
  "src": [
    "pages/SplashScreenPage"
  ]
}
```

##### **在 entry/src/main/ets/entryability/EntryAbility.ets 内添加**
```ts
import { RNAbility } from '@rnoh/react-native-openharmony';
import window from '@ohos.window';
import { SplashScreen } from '@react-native-ohos/react-native-splash-screen/src/main/ets/SplashScreen'

export default class EntryAbility extends RNAbility {
  onWindowStageCreate(windowStage: window.WindowStage): void {
    let startWindowIcon = $r('app.media.splashIcon'); // 启动页图片（用户改成自己 App 的启动图片，一般在 entry/src/main/resources/base/media 文件夹中）
    let startWindowBackground = "#FFFFFF"; // 启动页背景色
    let startPageUrl = 'pages/SplashScreenPage'; // 启动页
    SplashScreen.show(this.context, windowStage, startWindowIcon, startWindowBackground, startPageUrl).then(() => {
      super.onWindowStageCreate(windowStage);
    })
  }
}
```

##### **在适当的时候关闭启动屏幕（如：启动初始化完成后）**
```tsx
import React, { Component } from 'react';
import SplashScreen from 'react-native-splash-screen';
export default class WelcomePage extends Component {
    componentDidMount() {
        SplashScreen.hide(); // 关闭启动屏幕
    }
}
```

## Link

|                                      | 是否支持autolink | RN框架版本 |
|--------------------------------------|-----------------|------------|
| ~3.5.0                              |  是              |  0.82/0.84 |

使用AutoLink的工程需要根据该文档配置，Autolink框架指导文档：https://gitcode.com/CPF-RN/ohos_react_native/blob/master/docs/zh-cn/Autolinking.md

如您使用的版本支持 Autolink，并且工程已接入 Autolink，可跳过ManualLink配置。
<details>
  <summary>ManualLink: 此步骤为手动配置原生依赖项的指导</summary>

首先需要使用 DevEco Studio 打开项目里的 HarmonyOS 工程 `harmony`。

### 1. Overrides RN SDK

为了让工程依赖同一个版本的 RN SDK，需要在工程根目录的 `oh-package.json5` 添加 overrides 字段，指向工程需要使用的 RN SDK 版本。替换的版本既可以是一个具体的版本号，也可以是一个模糊版本，还可以是本地存在的 HAR 包或源码目录。

```json
{
  "overrides": {
    "@rnoh/react-native-openharmony": "~0.84.2" // ohpm 在线版本
    // "@rnoh/react-native-openharmony" : "./react_native_openharmony.har" // 指向本地 har 包的路径
    // "@rnoh/react-native-openharmony" : "./react_native_openharmony" // 指向源码路径
  }
}
```

### 2. 引入原生端代码

目前有两种方法：

- 通过 har 包引入；
- 直接链接源码。

方法一：通过 har 包引入（推荐）

> [!TIP] har 包位于三方库安装路径的 `harmony` 文件夹下。

打开 `entry/oh-package.json5`，添加以下依赖

```json
"dependencies": {
    "@react-native-ohos/react-native-splash-screen": "file:../../node_modules/@react-native-ohos/react-native-splash-screen/harmony/splash_screen.har"
  }
```

点击右上角的 `sync` 按钮

或者在命令行终端执行：

```bash
cd entry
ohpm install
```

方法二：直接链接源码

> [!TIP] 如需使用直接链接源码，请参考[直接链接源码说明](https://gitcode.com/CPF-RN/usage-docs/blob/master/zh-cn/link-source-code.md)
> 
### 3. 配置 CMakeLists 和引入 SplashScreenPackage

> 若使用的是 <= 3.3.0-0.0.2 版本，请跳过本章。

打开 `entry/src/main/cpp/CMakeLists.txt`，添加：

```diff
...

project(rnapp)
cmake_minimum_required(VERSION 3.4.1)
set(RNOH_APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
+ set(OH_MODULES "${CMAKE_CURRENT_SOURCE_DIR}/../../../oh_modules")
set(RNOH_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../react-native-harmony/harmony/cpp")

add_subdirectory("${RNOH_CPP_DIR}" ./rn)

# RNOH_BEGIN: manual_package_linking_1
add_subdirectory("../../../../sample_package/src/main/cpp" ./sample-package)
+ add_subdirectory("${OH_MODULES}/@react-native-ohos/react-native-splash-screen/src/main/cpp" ./splash_screen)
# RNOH_END: manual_package_linking_1

add_library(rnoh_app SHARED
    "./PackageProvider.cpp"
    "${RNOH_CPP_DIR}/RNOHAppNapiBridge.cpp"
)

target_link_libraries(rnoh_app PUBLIC rnoh)

# RNOH_BEGIN: manual_package_linking_2
target_link_libraries(rnoh_app PUBLIC rnoh_sample_package)
+ target_link_libraries(rnoh_app PUBLIC rnoh_splash_screen)
# RNOH_END: manual_package_linking_2
```

打开 `entry/src/main/cpp/PackageProvider.cpp`，添加：

```diff
#include "RNOH/PackageProvider.h"
+ #include "SplashScreenPackage.h"

using namespace rnoh;

std::vector<std::shared_ptr<Package>> PackageProvider::getPackages(Package::Context ctx) {
    return {
+        std::make_shared<SplashScreenPackage>(ctx)
}
```

### 4. 在 ArkTs 侧引入 SplashScreenPackage

打开 `entry/src/main/ets/RNPackagesFactory.ts`，添加：

```diff
  ...
+ import {SplashScreenPackage} from '@react-native-ohos/react-native-splash-screen/ts';

export function createRNPackages(ctx: RNPackageContext): RNPackage[] {
  return [
    new SamplePackage(ctx),
+   new SplashScreenPackage(ctx)
  ];
}
```
</details>

### 运行

点击右上角的 `sync` 按钮

或者在命令行终端执行：

```bash
cd entry
ohpm install
```

然后编译、运行即可。

下面的代码展示了这个库的基本使用场景：

```jsx
import React from 'react';
import { Text, View } from 'react-native';
import SplashScreen from 'react-native-splash-screen';

class App extends React.Component {
  componentDidMount() {
    SplashScreen.hide();
  }

  render() {
    return (
      <View style={{
        flex: 1,
        justifyContent: 'center',
        alignItems: 'center'
      }}>
        <Text>Hello, world!</Text>
      </View>
    )
  }
}

export default App;

```

## 约束与限制

### 兼容性

要使用此库，需要使用正确的 React-Native 和 RNOH 版本。另外，还需要使用配套的 DevEco Studio 和 手机 ROM。

在以下版本验证通过：

1. RNOH: 0.82.7; SDK: HarmonyOS 6.0.1 Release SDK; IDE: DevEco Studio 6.0.1 Release; ROM:6.0.0.328 SP26;
2. RNOH: 0.84.1; SDK: HarmonyOS 6.0.1 Release SDK; IDE: DevEco Studio 6.0.1 Release; ROM:6.0.0.120 SP7;

## 接口说明

### API

> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "OpenHarmony Support"列为 yes 表示 OpenHarmony平台支持 该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

### 1. JS 侧 API

| 名称 | 类型 | 参数类型 | 返回值 | 必填 | 平台 | OpenHarmony平台支持 | 描述 |
| ---- | ---- | -------- | ------ | ---- | ---- | ------------------- | ---- |
| `show()` | function | / | / | No | iOS / Android | Yes | 显示启动屏 |
| `hide()` | function | / | / | Yes | iOS / Android | Yes | 隐藏启动屏 |

### 2. 鸿蒙 Native 侧 API

| 名称 | 类型 | 参数类型 | 返回值 | 必填 | 平台 | OpenHarmony平台支持 | 描述 |
| ---- | ---- | -------- | ------ | ---- | ---- | ------------------- | ---- |
| `SplashScreen.show(abilityContext, windowStage, iconResource, backgroundColor, pageUrl)` | function | `UIAbilityContext`、`window.WindowStage`、`Resource`、`string`、`string` | `Promise<void>` | Yes | HarmonyOS | Yes | 创建全屏子窗口并显示启动屏 |

**参数说明：**

| Name | Description | Type |
| ---- | ----------- | ---- |
| `abilityContext` | 应用上下文，一般传 `this.context` | `UIAbilityContext` |
| `windowStage` | 窗口舞台，仅在 `onWindowStageCreate` 中传入 | `window.WindowStage` |
| `iconResource` | 启动页图片，如 `$r('app.media.splashIcon')` | `Resource` |
| `backgroundColor` | 启动页背景色，如 `"#FFFFFF"` | `string` |
| `pageUrl` | 子窗口加载的 ArkUI 页面路由，如 `'pages/SplashScreenPage'` | `string` |

> [!WARNING] 若未在 Native 层调用 `SplashScreen.show(...)`，JS 侧 `show()` 将因缺少 `windowStage` 等配置而无法生效。推荐流程：**Native `show` → RN 加载 → JS `hide()`**；需要再次遮罩时再调用 JS `show()` / `hide()`。

## 遗留问题

无

## 其他

- 在 iOS 中，show() 方法的工作原理是： 在入口 application 方法中，首先加载 App 首页，然后使用 while 循环让界面停留在 App 启动屏，此时首页仍然会异步加载，加载完成后，停止 while 循环即可隐藏启动屏，显示首页。
  HarmonyOS 中，在入口 onWindowStageCreate 中调用 windowStage.loadContent 加载首页，如果此时使用 while 循环，首页无法异步加载。

## 目录结构

```
/rntpc_react-native-splash-screen  # 项目根目录
├── harmony              # 鸿蒙适配代码
│    ├─ splash_screen.har          # har 包
│    └─ splash_screen              # 鸿蒙适配核心代码
│          ├─ index.ets            # 鸿蒙适配代码入口
│          ├─ ts.ets               # Package 导出
│          └─ src/main
│              ├─ cpp/                   # C++ 原生模块
│              └─ ets/
│                  ├─ SplashScreen.ts        # 核心模块实现
│                  ├─ SplashScreenPackage.ets # Package 注册
│                  ├─ SplashScreenView.ets   # 启动页视图组件
│                  └─ Logger.ts              # 日志工具
├── src                  # RN 代码
│    └─ RNSplashScreenModule.ts  # TurboModule 模块实现
├── index.js                     # 入口文件
├── index.d.ts                   # 类型定义
├── README_en.md           # 英文文档
├── README.md              # 中文文档
```

## 贡献代码

使用过程中发现任何问题都可以提交 [Issue](https://gitcode.com/CPF-RN/rntpc_react-native-splash-screen/issues)，当然，也非常欢迎提交 [PR](https://gitcode.com/CPF-RN/rntpc_react-native-splash-screen/pulls)。

## 开源协议

本项目基于 [The MIT License (MIT)](https://github.com/crazycodeboy/react-native-splash-screen/blob/v3.3.0/LICENSE) ，请自由地享受和参与开源。
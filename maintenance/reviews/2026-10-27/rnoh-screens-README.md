> 文档模板：v0.4.2

<p align="center">
  <h1 align="center"> <code>@react-native-ohos/react-native-screens</code> </h1>
</p>

本项目基于 [react-native-screens](https://github.com/software-mansion/react-native-screens) 开发。

该第三方库的仓库已迁移至 Gitcode，且支持直接从 npm 下载，新的包名为：`@react-native-ohos/react-native-screens` 版本所属关系如下：

| 三方库名称    | 三方库版本（npm地址）    | 发布信息     | 支持RN版本    | Autolink     | 编译API版本     | 社区基线版本    |  源码地址   |
| ------------ | ------------ | ------------------------------ | ------------- | ------------- |------------------------ | ------------- | ------------- |
| @react-native-ohos/react-native-screens | [~ 4.10.0（开发中）](https://www.npmjs.com/package/@react-native-ohos/react-native-screens) | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-screens/releases) | 0.84.* | 是 | API12+ | 4.26.2 | [master](https://gitcode.com/CPF-RN/rntpc_react-native-screens/tree/master) |
| @react-native-ohos/react-native-screens | [~ 4.9.0](https://www.npmjs.com/package/@react-native-ohos/react-native-screens) | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-screens/releases) | 0.82.* | 否 | API12+ | 4.17.1 | [br_rnoh0.82](https://gitcode.com/CPF-RN/rntpc_react-native-screens/tree/br_rnoh0.82) |
| @react-native-ohos/react-native-screens | [~ 4.8.3](https://www.npmjs.com/package/@react-native-ohos/react-native-screens) | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-screens/releases) | 0.77.* | 否 | API12+ | 4.8.0 | [br_rnoh0.77](https://gitcode.com/CPF-RN/rntpc_react-native-screens/tree/br_rnoh0.77) |
| @react-native-ohos/react-native-screens | [~ 3.34.2](https://www.npmjs.com/package/@react-native-ohos/react-native-screens) | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-screens/releases) | 0.72.* | 是 | API12+ | 3.34.0 | [br_rnoh0.72](https://gitcode.com/CPF-RN/rntpc_react-native-screens/tree/br_rnoh0.72) |
| @react-native-oh-tpl/react-native-screens | [<= 3.34.0-0.0.2@deprecated](https://www.npmjs.com/package/@react-native-oh-tpl/react-native-screens) | [Github Releases(deprecated)](https://github.com/react-native-oh-library/react-native-harmony-screens/releases) | 0.72.* | 否 | API12+ | 3.34.0 | [sig](https://github.com/react-native-oh-library/react-native-harmony-screens) |

## 简介

react-native-screens 是 React Navigation 生态系统中的原生屏幕导航组件库，提供原生导航的屏幕堆栈管理能力，支持 iOS、Android、HarmonyOS 平台。

## 下载安装

进入到工程目录并输入以下命令：

以下为@react-native-ohos/react-native-screens鸿蒙化结合@react-native-ohos/native-stack、@react-native-ohos/stack使用：

**npm**

```bash
npm install @react-native-ohos/react-native-screens@4.10.0-beta.1
npm install @react-native-ohos/native-stack@7.4.0-beta.13
npm install @react-native-ohos/stack@7.2.12-beta.2
npm install react-native-screens@4.17.1
```

**yarn**

```bash
yarn add @react-native-ohos/react-native-screens@4.10.0-beta.1
yarn add @react-native-ohos/native-stack@7.4.0-beta.13
yarn add @react-native-ohos/stack@7.2.12-beta.2
yarn add react-native-screens@4.17.1
```

## 依赖关系说明

请根据应用工程使用的 RN 框架版本，选择对应版本的三方库进行引入，版本对应关系如下：

| RN 框架版本 | 鸿蒙化依赖库 | 支持版本范围 |
| --- | --- | --- |
| 0.84 | [@react-native-ohos/native-stack](https://gitcode.com/CPF-RN/rntpc_react-navigation/blob/master/packages/native-stack/README.md) | >=7.4.0-beta.13 |
| | [@react-native-ohos/stack](https://gitcode.com/CPF-RN/rntpc_react-navigation/blob/master/packages/stack/README.md) | >=7.2.12-beta.2 |
| | [@react-native-ohos/react-native-safe-area-context](https://gitcode.com/CPF-RN/rntpc_react-native-safe-area-context/blob/master/README.md) | >=5.6.3 |
| | [@react-native-ohos/react-native-gesture-handler](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/blob/master/README.md) | >=2.30.1 |
| | [@react-native-ohos/react-native-reanimated](https://gitcode.com/CPF-RN/rntpc_react-native-reanimated/blob/master/README.md) | >=4.0.1 |

本库鸿蒙侧实现依赖上述鸿蒙化三方库，如已在鸿蒙应用工程中引入，可跳过本章节步骤，直接使用。

## Link

|                                      | 是否支持autolink | RN框架版本 |
|--------------------------------------|-----------------|------------|
| ~4.10.0                               |  Yes              |  0.84     |

使用AutoLink的工程需要根据该文档配置，Autolink框架指导文档：https://gitcode.com/CPF-RN/ohos_react_native/blob/master/docs/zh-cn/Autolinking.md

如您使用的版本支持 Autolink，并且工程已接入 Autolink，可跳过ManualLink配置。
<details>
  <summary>ManualLink: 此步骤为手动配置原生依赖项的指导</summary>

首先需要使用 DevEco Studio 打开项目里的 HarmonyOS 工程 `harmony`。

### 1. Overrides RN SDK

为了让工程依赖同一个版本的 RN SDK，需要在工程根目录的 `oh-package.json5` 添加 overrides 字段，指向工程需要使用的 RN SDK 版本。替换的版本既可以是一个具体的版本号，也可以是一个模糊版本，还可以是本地存在的 HAR 包或源码目录。

关于该字段的作用请阅读[官方说明](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides-V5/ide-oh-package-json5-V5#zh-cn_topic_0000001792256137_overrides)

```json
{
  "overrides": {
    "@rnoh/react-native-openharmony": "^0.84.1" // ohpm 在线版本
    // "@rnoh/react-native-openharmony" : "./react_native_openharmony.har" // 指向本地 har 包的路径
    // "@rnoh/react-native-openharmony" : "./react_native_openharmony" // 指向源码路径
  }
}
```

### 2. 引入原生端代码

目前有两种方法：

1. 通过 har 包引入（在 IDE 完善相关功能后该方法会被遗弃，目前首选此方法）；
2. 直接链接源码。

方法一：通过 har 包引入（推荐）

> [!TIP] har 包位于三方库安装路径的 `harmony` 文件夹下。

打开 `entry/oh-package.json5`，添加以下依赖

```json
"dependencies": {
    "@rnoh/react-native-openharmony": "file:../../node_modules/@rnoh/react-native-harmony/harmony/react_native_openharmony.har",
    "@react-native-ohos/react-native-screens": "file:../../node_modules/@react-native-ohos/react-native-screens/harmony/screens.har"
  }
```

点击右上角的 `sync` 按钮

或者在终端执行：

```bash
cd entry
ohpm install
```

方法二：直接链接源码

> [!TIP] 如需使用直接链接源码，请参考[直接链接源码说明](https://gitcode.com/CPF-RN/usage-docs/blob/master/zh-cn/link-source-code.md)

### 3. 配置 CMakeLists 和引入 Package

打开 `entry/src/main/cpp/CMakeLists.txt`，添加：

```diff
project(rnapp)
cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_SKIP_BUILD_RPATH TRUE)
set(RNOH_APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(NODE_MODULES "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../node_modules")
set(RNOH_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../oh_modules/@rnoh/react-native-openharmony/src/main/cpp")
+ set(OH_MODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../oh_modules")
set(RNOH_GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated")
set(LOG_VERBOSITY_LEVEL 1)
set(CMAKE_ASM_FLAGS "-Wno-error=unused-command-line-argument -Qunused-arguments")
set(CMAKE_CXX_FLAGS "-fstack-protector-strong -Wl,-z,relro,-z,now,-z,noexecstack -s -fPIE -pie")
set(WITH_HITRACE_SYSTRACE 1) # for other CMakeLists.txt files to use
add_compile_definitions(WITH_HITRACE_SYSTRACE)

add_subdirectory("${RNOH_CPP_DIR}" ./rn)

# RNOH_BEGIN: manual_package_linking_1
add_subdirectory("../../../../sample_package/src/main/cpp" ./sample-package)
+ add_subdirectory("${OH_MODULES_DIR}/@react-native-ohos/react-native-screens/src/main/cpp" ./rnoh_screens)
# RNOH_END: manual_package_linking_1

file(GLOB GENERATED_CPP_FILES "./generated/*.cpp")

add_library(rnoh_app SHARED
    ${GENERATED_CPP_FILES}
    "./PackageProvider.cpp"
    "${RNOH_CPP_DIR}/RNOHAppNapiBridge.cpp"
)
target_link_libraries(rnoh_app PUBLIC rnoh)

# RNOH_BEGIN: manual_package_linking_2
target_link_libraries(rnoh_app PUBLIC rnoh_sample_package)
+ target_link_libraries(rnoh_app PUBLIC rnoh_screens)
# RNOH_END: manual_package_linking_2
```

打开 `entry/src/main/cpp/PackageProvider.cpp`，添加：

```diff
#include "RNOH/PackageProvider.h"
#include "generated/RNOHGeneratedPackage.h"
+ #include "RnohReactNativeHarmonyScreensPackage.h"

using namespace rnoh;

std::vector<std::shared_ptr<Package>> PackageProvider::getPackages(Package::Context ctx) {
    return {
        std::make_shared<RNOHGeneratedPackage>(ctx),
        std::make_shared<SamplePackage>(ctx),
+        std::make_shared<rnoh::RnohReactNativeHarmonyScreensPackage>(ctx),
    };
}
```

### 4. 在ArkTs侧导入组件

找到 function buildCustomRNComponent()，一般位于 entry/src/main/ets/pages/index.ets 或 entry/src/main/ets/rn/LoadBundle.ets，添加：

```diff
  ...
+ import { componentBuilder } from "@react-native-ohos/react-native-screens"

@Builder
export function buildCustomRNComponent(ctx: ComponentBuilderContext) {
  ...
  Stack() {
+    componentBuilder(ctx)
  }
  ...
}
...
```

> [!TIP] 本库使用了混合方案，需要添加组件名。

在entry/src/main/ets/pages/index.ets 或 entry/src/main/ets/rn/LoadBundle.ets 找到常量 arkTsComponentNames 在其数组里添加组件名

```diff
const arkTsComponentNames: Array<string> = [
+ "RNSScreen", 
+ "RNSFullWindowOverlay", 
+ "RNSModalScreen", 
+ "RNSScreenContainer", 
+ "RNSScreenNavigationContainer", 
+ "RNSScreenStack", 
+ "RNSScreenStackHeaderSubview", 
+ "RNSSearchBar", 
+ "RNSScreenStackHeaderConfig", 
];
```

### 5. 在ArkTs侧引入Package

打开 `src/main/ets/RNOHPackagesFactory.ets`，添加：

```diff
  ...
import type { RNPackageContext, RNPackage } from '@rnoh/react-native-openharmony';
+ import RnohReactNativeHarmonyScreensPackage from '@react-native-ohos/react-native-screens';

export function createRNOHPackages(ctx: RNPackageContext): RNPackage[] {
  return [
    new SamplePackage(ctx),
+    new RnohReactNativeHarmonyScreensPackage(ctx),
  ];
}

```
</details>

### 运行

点击右上角的 `sync` 按钮

或者在终端执行：

```bash
cd entry
ohpm install
```

然后编译、运行即可。

## 约束与限制

### 兼容性

要使用此库，需要使用正确的 React-Native 和 RNOH 版本。另外，还需要使用配套的 DevEco Studio 和 手机 ROM。

在以下版本验证通过：

1. RNOH: 0.84.2; SDK: HarmonyOS 6.0.1 Release SDK; IDE: DevEco Studio 6.0.1.260; ROM:6.0.0.130;

## 使用示例

下面的代码展示了这个库的基本使用场景：

> [!WARNING] 使用时 import 的库名不变。

```js
import * as React from 'react';
import { Button, View } from 'react-native';
import { NavigationContainer } from '@react-navigation/native';
import { createNativeStackNavigator } from '@react-navigation/native-stack';
import { enableScreens } from "react-native-screens";
enableScreens(true);

function HomeScreen({ navigation }) {
  return (
    <View style={{ flex: 1, alignItems: 'center', justifyContent: 'center' }}>
      <Button
        title="Go to Profile"
        onPress={() => navigation.navigate('Profile')}
      />
    </View>
  );
}

function ProfileScreen({ navigation }) {
  return (
    <View style={{ flex: 1, alignItems: 'center', justifyContent: 'center' }}>
      <Button
        title="Go to Notifications"
        onPress={() => navigation.navigate('Notifications')}
      />
      <Button title="Go back" onPress={() => navigation.goBack()} />
    </View>
  );
}

function NotificationsScreen({ navigation }) {
  return (
    <View style={{ flex: 1, alignItems: 'center', justifyContent: 'center' }}>
      <Button
        title="Go to Settings"
        onPress={() => navigation.navigate('Settings')}
      />
      <Button title="Go back" onPress={() => navigation.goBack()} />
    </View>
  );
}

function SettingsScreen({ navigation }) {
  return (
    <View style={{ flex: 1, alignItems: 'center', justifyContent: 'center' }}>
      <Button title="Go back" onPress={() => navigation.goBack()} />
    </View>
  );
}

const Stack = createNativeStackNavigator();

function MyStack() {
  return (
    <Stack.Navigator
      initialRouteName="Home"
      screenOptions={{
        headerTintColor: '#999999',
        headerStyle: { backgroundColor: '#ff00ff' },
      }}
    >
      <Stack.Screen name="Home" component={HomeScreen} />
      <Stack.Screen name="Notifications" component={NotificationsScreen} />
      <Stack.Screen name="Profile" component={ProfileScreen} />
      <Stack.Screen name="Settings" component={SettingsScreen} />
    </Stack.Navigator>
  );
}

export default function App() {
  return (
    <NavigationContainer>
      <MyStack />
    </NavigationContainer>
  );
}

```

## 接口说明

> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；No 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

以下为@react-native-ohos/react-native-screens鸿蒙化的基本属性，属性已验证，更多属性详情请查看 [react-native-screens 的文档介绍](https://reactnavigation.org/docs/screen)。

| Name                                                      | Description                                                  | Type     | Required | Platform    | HarmonyOS Support |
| --------------------------------------------------------- | ------------------------------------------------------------ | -------- | -------- | ----------- | ----------------- |
| enableScreens | 支持原生及其 React Native View | function | No | Android,iOS | Yes |
| enableFreeze | 对 react-freeze 的支持，使用 ReactSuspense 机制来防止 React 组件树的部分渲染 | function | No | Android,iOS | No |
| createNativeStackNavigator | 提供屏幕切换的能力 | function | No | Android,iOS | No |
| NativeStackNavigationProp | 切换页面属性的封装 | object | No | Android,iOS | Yes |
| NativeStackNavigationOptions | 导航栏属性设置封装 | object | No | Android,iOS | No |
| FullWindowOverlay | 一个组件，可以将其子组件放在其他组件之上 | object | No | Android,iOS | No |
| useTransitionProgress | 提供屏幕过渡的动画插值器 | function | No | Android,iOS | No |
| useReanimatedTransitionProgress ReanimatedScreenProvider | 屏幕切换期间调用的帧回调，用于 react-native-reanimated 2.0 及其以上的版本，并使用 ReanimatedScreenProvider 进行封装 | function | No | Android,iOS | No |
| screensEnabled | 查询原生 screens 是否已启用 | function | No | All | Yes |
| freezeEnabled | 查询 freeze 是否已启用 | function | No | All | No |
| SearchBar | 原生搜索栏组件 | component | No | All | Yes |
| ScreenContainer | 屏幕容器组件，用于包裹多个 Screen | component | No | All | Yes |
| ScreenStack | 原生屏幕堆栈容器组件 | component | No | All | Yes |
| ScreenStackItem | ScreenStack 内部使用的屏幕容器组件 | component | No | Android | Yes |
| ScreenFooter | formSheet 呈现时的页脚容器组件 | component | No | Android,iOS | No |
| ScreenContentWrapper | 内容包裹容器组件，用于 Screen 内可滚动内容 | component | No | All | No |
| isSearchBarAvailableForCurrentPlatform | 判断当前平台是否支持 SearchBar | function | No | Android | Yes |
| executeNativeBackPress | 执行原生返回按键操作 | function | No | Android,iOS | Yes |
| compatibilityFlags | 兼容性标志位对象 | object | No | All | Yes |
| featureFlags | 功能特性标志位对象 | object | No | All | Yes |

### ScreenProps 属性列表

| Name                                         | Description                                                  | Type                                                         | Required | Platform    | HarmonyOS Support |
| -------------------------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ | -------- | ----------- | ----------------- |
| customAnimationOnSwipe | 布尔值，表示滑动解除应触发“stackAnimation”提供的动画。默认为“false”。 | `boolean` | No | iOS | Yes |
| fullScreenSwipeEnabled | 全屏滑动 | `boolean` | No | iOS | Yes |
| fullScreenSwipeShadowEnabled | 布尔值，指示全屏解除手势在过渡期间是否在视图下有阴影。手势使用自定义过渡，因此默认情况下没有阴影。启用后，在转换过程中会添加一个自定义阴影视图，试图模仿默认iOS阴影。默认为“true”。 | `boolean` | No | iOS | Yes |
| gestureEnabled | 是否开启手势滑动 | `boolean` | No | iOS | Yes |
| gestureResponseDistance | 手势滑动的有效区域 | `GestureResponseDistanceType` | No | iOS | Yes |
| homeIndicatorHidden | 主页指示器是否应在此屏幕上隐藏。默认为“false”。 | `boolean` | No | iOS | No |
| hideKeyboardOnSwipe | 滑至上一屏幕时，键盘是否应隐藏。默认为”false”。 | `boolean` | No | iOS | No |
| scrollEdgeEffects | 配置内容 ScrollView 的滚动边缘效果 | { top/bottom/left/right: 'automatic' \| 'hard' \| 'soft' \| 'hidden' } | No | iOS | No |
| onAppear | 页面显示 | function | No | All | No |
| onDisappear | 页面消失 | function | No | All | No |
| onSheetDetentChanged | 当前屏幕处于 formSheet 呈现状态且其 detent 发生变化时调用的回调 | function | No | All | No |
| onWillAppear | 页面将显示 | function | No | All | No |
| onWillDisappear | 页面将消失 | function | No | All | No |
| preventNativeDismiss | 指示是否阻止当前屏幕被 dismiss，默认为 false | `boolean` | No | iOS | Yes |
| replaceAnimation | 进出栈类型 | `ScreenReplaceTypes` | No | All | Yes |
| screenId | 在原生代码中标识屏幕的方式 | `string \| undefined` | No | iOS | Yes |
| screenOrientation | 屏幕显示方向 | `ScreenOrientationTypes` | No | All | Yes |
| sheetAllowedDetents | 描述 sheet 可以停留的高度，仅在 presentation 设置为 formSheet 时有效 | `number[] \| 'fitToContents' \| 'medium' \| 'large' \| 'all'` | No | All | No |
| sheetElevation | 描述 sheet 的 elevation，影响 sheet 顶部边缘的阴影 | `number` | No | Android | No |
| sheetExpandsWhenScrolledToEdge | 指示 sheet 在滚动到边缘时是否应该扩展到更大的 detent，默认为 true | `boolean` | No | iOS | No |
| sheetCornerRadius | sheet 将尝试渲染的圆角半径 | `number` | No | iOS | No |
| sheetGrabberVisible | 指示 sheet 是否在顶部显示抓取器，默认为 false | `boolean` | No | iOS | No |
| stackAnimation | 屏幕在堆栈顶部被推送或弹出时的出现/消失动画 | `StackAnimationTypes` | No | All | Yes |
| stackPresentation | 屏幕应该如何呈现 | `StackPresentationTypes` | No | All | Yes |
| statusBarAnimation | 设置状态栏动画（类似于 StatusBar 组件） | `'none' \| 'fade' \| 'slide'` | No | All | No |
| statusBarColor | 设置状态栏颜色（已废弃） | `ColorValue` | No | All | No |
| statusBarHidden | 指示是否应该在此屏幕上隐藏状态栏，默认为 false | `boolean` | No | All | Yes |
| statusBarStyle | 设置状态栏样式（类似于 StatusBar 组件） | `'inverted' \| 'auto' \| 'light' \| 'dark'` | No | All | Yes |
| statusBarTranslucent | 设置状态栏是否半透明 | `boolean` | No | All | Yes |
| transitionDuration | 更改 iOS 上 slide_from_bottom、fade_from_bottom、fade 和 simple_push 过渡的持续时间（毫秒），默认为 500 | `number` | No | iOS | Yes |
| onGestureCancel | 滑动返回被取消后调用的回调 | function | No | All | Yes |
| activityState | 在容器内多页面时，控制页面的是否显示 | `number` | No | All | Yes |
| active | 控制屏幕激活状态的动画插值（供导航器过渡使用） | `0 \| 1 \| AnimatedInterpolation<number>` | No | All | No |
| shouldFreeze | 是否使用 react-freeze 冻结当前屏幕 | `boolean` | No | All | No |
| enabled | 是否启用原生屏幕实现，需与容器取值一致 | `boolean` | No | All | No |
| isNativeStack | 内部属性，标记是否用于 native-stack 导航器 | `boolean` | No | All | No |
| hasLargeHeader | 内部属性，标记当前 header 是否为 iOS 大标题 | `boolean` | No | iOS | No |
| freezeOnBlur | 失焦时是否挂起渲染，enableFreeze() 开启后默认 true | `boolean` | No | All | No |
| nativeBackButtonDismissalEnabled | Android 返回键点击时是否在原生侧执行 pop，默认 false | `boolean` | No | Android | No |
| navigationBarColor | 已废弃，设置无效果 | `ColorValue` | No | Android | No |
| navigationBarTranslucent | 已废弃，设置无效果 | `boolean` | No | Android | No |
| navigationBarHidden | 是否隐藏导航栏，默认 false | `boolean` | No | Android | Yes |
| onComponentRef | 内部回调，用于获取组件引用 | `function` | No | All | No |
| onDismissed | 屏幕被硬件返回键（Android）或关闭手势 dismiss 后的回调 | `function` | No | All | No |
| onHeaderHeightChange | 标题栏高度变化后的回调 | `function` | No | All | No |
| onHeaderBackButtonClicked | 内部回调，Android 原生标题返回键点击且未启用原生 dismiss 时调用 | `function` | No | Android | No |
| onNativeDismissCancelled | 内部回调，preventNativeDismiss 为 true 时 dismiss 被阻止后调用 | `function` | No | iOS | No |
| onTransitionProgress | 内部回调，native-stack 屏幕过渡期间每帧调用 | `function` | No | All | No |
| sheetLargestUndimmedDetentIndex | sheet 下方视图不变暗的最大 detent 索引，仅 formSheet 呈现时有效 | `number \| 'none' \| 'last' \| 'medium' \| 'large' \| 'all'` | No | All | No |
| sheetInitialDetentIndex | sheet 打开后初始展开到的 detent 索引，仅 formSheet 呈现时有效 | `number \| 'last'` | No | All | No |
| sheetShouldOverflowTopInset | sheet 内容是否延伸到状态栏/挖孔区域之后，仅 formSheet 呈现时有效 | `boolean` | No | Android | No |
| sheetDefaultResizeAnimationEnabled | fitToContents 内容尺寸变化时是否使用默认原生 resize 动画，仅 formSheet 呈现时有效 | `boolean` | No | Android | No |
| swipeDirection | 设置关闭屏幕的滑动方向（vertical / horizontal） | `SwipeDirectionTypes` | No | iOS | Yes |
| unstable_sheetFooter | formSheet 呈现时渲染页脚组件 | `function` | No | Android | No |

### ScreenStackHeaderSubviewProps 属性列表

| Name                  | Description                                                  | Type      | Required | Platform | HarmonyOS Support |
| --------------------- | ------------------------------------------------------------ | --------- | -------- | -------- | ----------------- |
| hidesSharedBackground | 指示此项目可能与栏中其他项目共享的背景是否应该隐藏。仅适用于 type="right" 和 type="left" 子视图。仅从 iOS 26.0 及更高版本可用 | `boolean` | No | All | No |

### ScreenStackHeaderConfigProps 属性列表

| Name                                             | Description                                                  | Type                    | Required | Platform | HarmonyOS Support |
| ------------------------------------------------ | ------------------------------------------------------------ | ----------------------- | -------- | -------- | ----------------- |
| backButtonInCustomView | 是否在自定义左侧标题时显示返回按钮 | `boolean` | No | All | No |
| backgroundColor | 控制导航标题的颜色 | `ColorValue` | No | All | Yes |
| backTitle | 返回按钮文本内容 | `string` | No | iOS | Yes |
| backTitleFontSize | 返回按钮文本字号大小 | `number` | No | iOS | Yes |
| backTitleVisible | 返回按钮文本是否显示 | `boolean` | No | iOS | Yes |
| blurEffect | 应用于标题的模糊效果，与 backgroundColor 的 alpha < 1 一起使用 | `BlurEffectTypes` | No | iOS | Yes |
| color | 控制标题上渲染的项目的颜色，包括返回图标、返回文本和标题文本 | `ColorValue` | No | All | Yes |
| direction | 堆栈是否应该采用 rtl 或 ltr 形式 | `'rtl' \| 'ltr'` | No | All | No |
| disableBackButtonMenu | 控制是否在 iOS >= 14 返回按钮的长按上显示菜单 | `boolean` | No | iOS | No |
| backButtonDisplayMode | 控制返回按钮的行为 | `BackButtonDisplayMode` | No | iOS | No |
| headerLeftBarButtonItems | 标题左侧的 UIBarButtomItems 数组 | `HeaderBarButtonItem[]` | No | iOS | No |
| headerRightBarButtonItems | 标题右侧的 UIBarButtomItems 数组 | `HeaderBarButtonItem[]` | No | iOS | No |
| hidden | 隐藏标题栏 | `boolean` | No | All | Yes |
| hideBackButton | 隐藏标题栏返回按钮 | `boolean` | No | All | Yes |
| hideShadow | 控制是否隐藏标题栏的阴影或底部边框 | `boolean` | No | All | Yes |
| largeTitle | 设置原生属性以首选大标题标题（如 iOS 设置中） | `boolean` | No | iOS | No |
| largeTitleBackgroundColor | 控制当任何可滚动内容的边缘到达导航栏的匹配边缘时导航标题的颜色 | `ColorValue` | No | All | No |
| largeTitleColor | 自定义用于大标题的颜色，默认使用 titleColor 属性 | `ColorValue` | No | iOS | No |
| largeTitleFontFamily | 自定义用于大标题的字体系列 | `string` | No | iOS | No |
| largeTitleFontSize | 自定义用于大标题的字体大小 | `number` | No | iOS | No |
| largeTitleFontWeight | 自定义用于大标题的字体权重 | `string` | No | iOS | No |
| largeTitleHideShadow | 允许在任何可滚动内容的边缘到达导航栏的匹配边缘时禁用导航标题下的下拉阴影 | `boolean` | No | All | No |
| title | 标题栏标题 | `string` | No | All | Yes |
| titleColor | 允许设置标题的文本颜色 | `ColorValue` | No | All | Yes |
| titleFontFamily | 自定义用于标题的字体系列 | `string` | No | All | Yes |
| titleFontSize | 自定义用于标题的字体大小 | `number` | No | All | Yes |
| titleFontWeight | 自定义用于标题的字体权重 | `string` | No | All | Yes |
| topInsetEnabled | 已废弃，设置此属性无效果 | `boolean` | No | All | No |
| translucent | 标题栏是否透明化 | `boolean` | No | All | Yes |
| backTitleFontFamily | 自定义返回按钮文本的字体系列 | `string` | No | iOS | No |
| onAttached | 标题栏挂载时执行的回调 | `function` | No | All | No |
| onDetached | 标题栏卸载时执行的回调 | `function` | No | All | No |
| disableTopInsetApplication | 最外层可见标题栏禁用顶部 inset 处理 | `boolean` | No | Android | No |
| disableLeftInsetApplication | 禁用左侧 inset 处理并对整个子树生效 | `boolean` | No | Android | No |
| disableRightInsetApplication | 禁用右侧 inset 处理并对整个子树生效 | `boolean` | No | Android | No |
| disableBottomInsetApplication | 禁用底部 inset 处理并对整个子树生效 | `boolean` | No | Android | No |
| experimental_userInterfaceStyle | 覆盖标题栏系统外观（实验性） | `'unspecified' \| 'light' \| 'dark'` | No | iOS | No |

### SearchBarProps 属性列表

| Name                                      | Description                                                  | Type                                                         | Required | Platform | HarmonyOS Support |
| ----------------------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ | -------- | -------- | ----------------- |
| autoCapitalize | 自动大写行为，默认为 systemDefault | `'systemDefault' \| 'none' \| 'words' \| 'sentences' \| 'characters'` | No | All | No |
| autoFocus | 挂载时自动聚焦搜索栏 | `boolean` | No | Android | Yes |
| barTintColor | 搜索字段背景颜色 | `ColorValue` | No | All | Yes |
| tintColor | 光标插入符号和取消按钮文本的颜色 | `ColorValue` | No | iOS | Yes |
| cancelButtonText | 用于替代默认 Cancel 按钮文本的文本 | `string` | No | iOS | Yes |
| disableBackButtonOverride | 指定返回按钮是否应该关闭搜索栏的文本输入 | `boolean` | No | Android | No |
| hideNavigationBar | 指示是否应该隐藏导航栏 | `boolean` | No | iOS | Yes |
| hideWhenScrolling | 指示滚动时是否应该隐藏搜索栏 | `boolean` | No | iOS | No |
| inputType | 设置输入类型，默认为 text | `'text' \| 'phone' \| 'number' \| 'email'` | No | Android | Yes |
| obscureBackground | 指示是否应该模糊底层内容 | `boolean` | No | iOS | No |
| placeholder | 当搜索字段为空时显示的文本 | `string` | No | All | Yes |
| placement | 搜索栏的位置 | `SearchBarPlacement` | No | iOS | No |
| allowToolbarIntegration | 指示系统是否可以在 iPhone 上将搜索栏放置在其他工具栏项目之间 | `boolean` | No | iOS | No |
| textColor | 搜索字段文本颜色 | `ColorValue` | No | All | Yes |
| hintTextColor | 搜索提示文本颜色 | `ColorValue` | No | Android | Yes |
| headerIconColor | 标题中显示的搜索和关闭图标颜色 | `ColorValue` | No | Android | Yes |
| shouldShowHintSearchIcon | 当搜索栏聚焦时显示搜索提示图标 | `boolean` | No | Android | Yes |
| ref | 命令式引用，支持 focus/blur/clearText/setText/cancelSearch/toggleCancelButton | `SearchBarCommands` | No | All | Yes |
| onClose | 关闭搜索 | function | No | Android,iOS | Yes |
| onOpen | 展示搜索 | function | No | Android,iOS | Yes |
| onSearchButtonPress | 搜索 | function | No | Android,iOS | Yes |
| onCancelButtonPress | 取消搜索 | function | No | Android,iOS | Yes |
| onChangeText | 搜索栏文本变更 | function | No | Android,iOS | Yes |
| onFocus | 搜索栏获得焦点时的回调 | `function` | No | All | Yes |
| onBlur | 搜索栏失去焦点时的回调 | `function` | No | All | Yes |

### StackAnimationTypes 类型值列表

| Name              | Description            | Type                  | Required | Platform | HarmonyOS Support |
| ----------------- | ---------------------- | --------------------- | -------- | -------- | ----------------- |
| default | 默认动画 | `'default'` | No | All | Yes |
| fade | 淡入淡出动画 | `'fade'` | No | All | Yes |
| fade_from_bottom | 从底部淡入淡出动画 | `'fade_from_bottom'` | No | All | Yes |
| flip | 从右侧滑入动画 | `'flip'` | No | All | Yes |
| none | 无动画 | `'none'` | No | All | Yes |
| simple_push | 简单推送动画 | `'simple_push'` | No | All | Yes |
| slide_from_bottom | 从底部滑入动画 | `'slide_from_bottom'` | No | All | Yes |
| slide_from_right | 从右侧滑入动画 | `'slide_from_right'` | No | All | Yes |
| slide_from_left | 从左侧滑入动画 | `'slide_from_left'` | No | All | Yes |
| ios_from_right | iOS 风格从右侧滑入动画 | `'ios_from_right'` | No | All | Yes |
| ios_from_left | iOS 风格从左侧滑入动画 | `'ios_from_left'` | No | All | Yes |

### StackPresentationTypes 类型值列表

| Name                      | Description        | Type                          | Required | Platform | HarmonyOS Support |
| ------------------------- | ------------------ | ----------------------------- | -------- | -------- | ----------------- |
| push | 标准推送呈现 | `'push'` | No | All | Yes |
| modal | 模态呈现 | `'modal'` | No | All | Yes |
| transparentModal | 透明模态呈现 | `'transparentModal'` | No | All | Yes |
| containedModal | 包含式模态呈现 | `'containedModal'` | No | All | Yes |
| containedTransparentModal | 包含式透明模态呈现 | `'containedTransparentModal'` | No | All | Yes |
| fullScreenModal | 全屏模态呈现 | `'fullScreenModal'` | No | All | Yes |
| formSheet | 表单表单呈现 | `'formSheet'` | No | All | No |
| pageSheet | 页面表单呈现 | `'pageSheet'` | No | All | Yes |

### ScreenReplaceTypes 类型值列表

| Name | Description  | Type     | Required | Platform | HarmonyOS Support |
| ---- | ------------ | -------- | -------- | -------- | ----------------- |
| push | 推送新屏幕 | `'push'` | No | All | Yes |
| pop | 弹出当前屏幕 | `'pop'` | No | All | Yes |

### ScreenOrientationTypes 类型值列表

| Name            | Description  | Type                | Required | Platform | HarmonyOS Support |
| --------------- | ------------ | ------------------- | -------- | -------- | ----------------- |
| default | 默认屏幕方向 | `'default'` | No | All | Yes |
| all | 所有屏幕方向 | `'all'` | No | All | Yes |
| portrait | 竖屏方向 | `'portrait'` | No | All | Yes |
| portrait_up | 向上竖屏方向 | `'portrait_up'` | No | All | Yes |
| portrait_down | 向下竖屏方向 | `'portrait_down'` | No | All | Yes |
| landscape | 横屏方向 | `'landscape'` | No | All | Yes |
| landscape_left | 向左横屏方向 | `'landscape_left'` | No | All | Yes |
| landscape_right | 向右横屏方向 | `'landscape_right'` | No | All | Yes |

### BlurEffectTypes 类型值列表

| Name                         | Description                    | Type                             | Required | Platform | HarmonyOS Support |
| ---------------------------- | ------------------------------ | -------------------------------- | -------- | -------- | ----------------- |
| none | 无模糊效果 | `'none'` | No | All | Yes |
| extraLight | 极轻量级模糊效果 | `'extraLight'` | No | All | Yes |
| light | 轻量级模糊效果 | `'light'` | No | All | Yes |
| dark | 深色模糊效果 | `'dark'` | No | All | Yes |
| regular | 常规模糊效果 | `'regular'` | No | All | Yes |
| prominent | 突出模糊效果 | `'prominent'` | No | All | No |
| systemUltraThinMaterial | 系统超薄材质模糊效果 | `'systemUltraThinMaterial'` | No | All | No |
| systemThinMaterial | 系统薄材质模糊效果 | `'systemThinMaterial'` | No | All | No |
| systemMaterial | 系统材质模糊效果 | `'systemMaterial'` | No | All | No |
| systemThickMaterial | 系统厚材质模糊效果 | `'systemThickMaterial'` | No | All | No |
| systemChromeMaterial | 系统Chrome材质模糊效果 | `'systemChromeMaterial'` | No | All | No |
| systemUltraThinMaterialLight | 系统超薄材质（浅色）模糊效果 | `'systemUltraThinMaterialLight'` | No | All | No |
| systemThinMaterialLight | 系统薄材质（浅色）模糊效果 | `'systemThinMaterialLight'` | No | All | No |
| systemMaterialLight | 系统材质（浅色）模糊效果 | `'systemMaterialLight'` | No | All | No |
| systemThickMaterialLight | 系统厚材质（浅色）模糊效果 | `'systemThickMaterialLight'` | No | All | No |
| systemChromeMaterialLight | 系统Chrome材质（浅色）模糊效果 | `'systemChromeMaterialLight'` | No | All | No |
| systemUltraThinMaterialDark | 系统超薄材质（深色）模糊效果 | `'systemUltraThinMaterialDark'` | No | All | No |
| systemThinMaterialDark | 系统薄材质（深色）模糊效果 | `'systemThinMaterialDark'` | No | All | No |
| systemMaterialDark | 系统材质（深色）模糊效果 | `'systemMaterialDark'` | No | All | No |
| systemThickMaterialDark | 系统厚材质（深色）模糊效果 | `'systemThickMaterialDark'` | No | All | No |
| systemChromeMaterialDark | 系统Chrome材质（深色）模糊效果 | `'systemChromeMaterialDark'` | No | All | No |

### GestureResponseDistanceType 属性列表

| Name   | Description        | Type     | Required | Platform | HarmonyOS Support |
| ------ | ------------------ | -------- | -------- | -------- | ----------------- |
| start | 手势响应的起始距离 | `number` | No | All | Yes |
| end | 手势响应的结束距离 | `number` | No | All | Yes |
| top | 手势响应的顶部距离 | `number` | No | All | Yes |
| bottom | 手势响应的底部距离 | `number` | No | All | Yes |

### SearchBarPlacement 类型值列表

| Name               | Description                            | Type                   | Required | Platform | HarmonyOS Support |
| ------------------ | -------------------------------------- | ---------------------- | -------- | -------- | ----------------- |
| automatic | 自动选择搜索栏位置 | `'automatic'` | No | All | No |
| inline | 内联搜索栏位置（从 iOS 26 开始已弃用） | `'inline'` | No | All | No |
| stacked | 堆叠搜索栏位置 | `'stacked'` | No | All | No |
| integrated | 集成搜索栏位置 | `'integrated'` | No | All | No |
| integratedButton | 集成按钮搜索栏位置 | `'integratedButton'` | No | All | No |
| integratedCentered | 集成居中搜索栏位置 | `'integratedCentered'` | No | All | No |

### SharedHeaderBarButtonItem 属性列表

| Name                  | Description                                            | Type                                                         | Required | Platform | HarmonyOS Support |
| --------------------- | ------------------------------------------------------ | ------------------------------------------------------------ | -------- | -------- | ----------------- |
| index | 项目在导航项目数组中的位置 | `number` | No | All | No |
| title | 项目的标签 | `string` | No | All | No |
| titleStyle | 项目标签的样式 | `{ fontFamily?: string; fontSize?: number; fontWeight?: string; color?: ColorValue; }` | No | All | No |
| icon | 项目的图标 | `PlatformIconIOS` | No | All | No |
| variant | 项目的变体 | `'plain' \| 'done' \| 'prominent'` | No | All | No |
| tintColor | 应用于项目的 tint 颜色 | `ColorValue` | No | All | No |
| disabled | 指示项目是否处于禁用状态的布尔值 | `boolean` | No | All | No |
| width | 项目的宽度 | `number` | No | All | No |
| hidesSharedBackground | 指示此项目可能与栏中其他项目共享的背景是否应该隐藏 | `boolean` | No | All | No |
| sharesBackground | 指示此项目是否可以与导航栏或工具栏中的其他项目共享背景 | `boolean` | No | All | No |
| identifier | 用于在导航栏或工具栏的过渡中匹配项目的标识符 | `string` | No | All | No |
| badge | 要在项目上渲染的徽章 | `{ value: string; style?: { color?: ColorValue; backgroundColor?: ColorValue; fontFamily?: string; fontSize?: number; fontWeight?: string; }; }` | No | All | No |
| accessibilityLabel | 无障碍标签 | `string` | No | All | No |
| accessibilityHint | 无障碍提示 | `string` | No | All | No |

### HeaderBarButtonItemWithAction 属性列表

| Name                            | Description                        | Type      | Required | Platform | HarmonyOS Support |
| ------------------------------- | ---------------------------------- | --------- | -------- | -------- | ----------------- |
| onPress | 项目被按下时的回调函数 | function | Yes | All | No |
| selected | 指示项目是否处于选中状态的布尔值 | `boolean` | No | All | No |
| changesSelectionAsPrimaryAction | 指示项目是否表示操作或选择的布尔值 【4.26.2 移至 HeaderBarButtonItemWithMenu】 | `boolean` | No | All | No |
| type | 项目类型，固定为 'button' | `'button'` | No | All | No |

### HeaderBarButtonItemMenuAction 属性列表

| Name                 | Description                     | Type                                                         | Required | Platform | HarmonyOS Support |
| -------------------- | ------------------------------- | ------------------------------------------------------------ | -------- | -------- | ----------------- |
| type | 菜单操作的类型，固定为 'action' | `'action'` | Yes | All | No |
| title | 菜单操作的标签 | `string` | No | All | No |
| onPress | 菜单操作被按下时的回调函数 | function | Yes | All | No |
| icon | 菜单操作的图标 | `PlatformIconIOSSfSymbol` | No | All | No |
| state | 菜单项的状态 | `'on' \| 'off' \| 'mixed'` | No | All | No |
| discoverabilityLabel | 菜单项的可发现性标签 | `string` | No | All | No |
| subtitle | 菜单操作的副标题 | `string` | No | All | No |
| disabled | 是否对菜单项应用禁用样式 | `boolean` | No | All | No |
| destructive | 是否对菜单项应用破坏性样式 | `boolean` | No | All | No |
| hidden | 是否对菜单项应用隐藏样式 | `boolean` | No | All | No |
| keepsMenuPresented | 触发菜单项动作后是否保持菜单展示 | `boolean` | No | All | No |

### HeaderBarButtonItemSubmenu 属性列表

| Name  | Description                    | Type                                           | Required | Platform | HarmonyOS Support |
| ----- | ------------------------------ | ---------------------------------------------- | -------- | -------- | ----------------- |
| type | 子菜单的类型，固定为 'submenu' | `'submenu'` | Yes | All | No |
| title | 子菜单的标签（原 label） | `string` | No | All | No |
| icon | 子菜单的图标 | `PlatformIconIOSSfSymbol` | No | All | No |
| items | 子菜单的项目列表 | `HeaderBarButtonItemWithMenu['menu']['items']` | Yes | All | No |
| displayInline | 子菜单是否内联展示 | `boolean` | No | All | No |
| destructive | 子菜单是否为破坏性操作 | `boolean` | No | All | No |
| singleSelection | 子菜单是否单选 | `boolean` | No | All | No |
| displayAsPalette | 子菜单是否以调色板形式展示 | `boolean` | No | All | No |

### HeaderBarButtonItemWithMenu 属性列表

| Name | Description    | Type                                                         | Required | Platform | HarmonyOS Support |
| ---- | -------------- | ------------------------------------------------------------ | -------- | -------- | ----------------- |
| menu | 按钮的菜单配置。`title` 为菜单标题（原 label）；`singleSelection` 是否单选；`displayAsPalette` 是否以调色板展示 | `{ title?: string; items: (HeaderBarButtonItemMenuAction \| HeaderBarButtonItemSubmenu)[]; singleSelection?: boolean; displayAsPalette?: boolean; }` | Yes | All | No |
| type | 项目类型，固定为 'menu' | `'menu'` | No | All | No |
| changesSelectionAsPrimaryAction | 按钮标题是否指示选中状态（自 WithAction 移入） | `boolean` | No | All | No |

### HeaderBarButtonItemSpacing 属性列表

| Name    | Description    | Type     | Required | Platform | HarmonyOS Support |
| ------- | -------------- | -------- | -------- | -------- | ----------------- |
| spacing | 按钮之间的间距 | `number` | Yes | All | No |
| type | 项目类型，固定为 'spacing' | `'spacing'` | No | All | No |

### HeaderBarButtonItem 类型值列表

| Name                          | Description              | Type                            | Required | Platform | HarmonyOS Support |
| ----------------------------- | ------------------------ | ------------------------------- | -------- | -------- | ----------------- |
| HeaderBarButtonItemWithAction | 带有点击动作的导航栏按钮 | `HeaderBarButtonItemWithAction` | No | All | No |
| HeaderBarButtonItemWithMenu | 带有菜单的导航栏按钮 | `HeaderBarButtonItemWithMenu` | No | All | No |
| HeaderBarButtonItemSpacing | 导航栏按钮之间的间距 | `HeaderBarButtonItemSpacing` | No | All | No |

### BottomTabs类型值列表

| Name                               | Description                | Type                                               | Required | Platform | HarmonyOS Support |
| ---------------------------------- | -------------------------- | -------------------------------------------------- | -------- | -------- | ----------------- |
| `normal` | 未选中、未聚焦状态下标签项外观（含 tabBarItemTitleFontColor / tabBarItemIconColor） | `object` | No | Android | No |
| `selected` | 选中状态下标签项外观 | `object` | No | Android | No |
| `focused` | 聚焦状态下标签项外观 | `object` | No | Android | No |
| `disabled` | 禁用状态下标签项外观 | `object` | No | Android | No |
| `tabBarItemTitleSmallLabelFontSize` | 未选中标签项标题字号（sp） | `number` | No | Android | No |
| `tabBarItemTitleLargeLabelFontSize` | 选中标签项标题字号（sp） | `number` | No | Android | No |
| `navStateRequest` | 传递给原生的导航状态请求（selectedScreenKey + baseProvenance，必填） | `object` | **Yes** | Android,iOS | No |
| `rejectStaleNavStateUpdates` | 是否拒绝基于过期状态的 JS 导航更新，默认 false | `boolean` | No | Android,iOS | No |
| `tabBarHidden` | 是否隐藏标签栏，默认 false | `boolean` | No | Android,iOS | No |
| `nativeContainerStyle` | 原生容器样式（backgroundColor） | `object` | No | Android,iOS | No |
| `direction` | 原生容器布局方向 | `'inherit' \| 'ltr' \| 'rtl'` | No | Android,iOS | No |
| `colorScheme` | 容器及子容器使用的配色方案 | `'inherit' \| 'light' \| 'dark'` | No | Android,iOS | No |
| `onTabSelected` | 标签选中变化时的回调 | `function` | No | Android,iOS | No |
| `onTabSelectionRejected` | 原生拒绝标签选择请求时的回调 | `function` | No | Android,iOS | No |
| `onTabSelectionPrevented` | 因 preventNativeSelection 阻止标签选择时的回调 | `function` | No | Android,iOS | No |
| `tabBarRespectsIMEInsets` | 标签栏是否适配键盘 inset（API 30+，默认 false） | `boolean` | No | Android | No |

### BottomTabsScreen类型值列表

| Name                             | Description                                      | Type                                        | Required | Platform     | HarmonyOS Support |
| -------------------------------- | ------------------------------------------------ | ------------------------------------------- | -------- | ------------ | ----------------- |
| `title` | 标签页标题 | `string` | No | Android,iOS | No |
| `badgeValue` | 标签页徽章文本 | `string` | No | Android,iOS | No |
| `onWillAppear` | 页面即将进入前台、即将可见时触发 | `BottomTabsScreenEventHandler<EmptyObject>` | No | Android,iOS | No |
| `onDidAppear` | 页面已经完全显示并进入前台后触发 | `BottomTabsScreenEventHandler<EmptyObject>` | No | Android,iOS | No |
| `onWillDisappear` | 页面即将离开前台、即将不可见时触发（但尚未消失） | `BottomTabsScreenEventHandler<EmptyObject>` | No | Android,iOS | No |
| `onDidDisappear` | 页面已经完全不可见（退出前台）后触发 | `BottomTabsScreenEventHandler<EmptyObject>` | No | Android,iOS | No |
| `screenKey` | 标签页唯一标识符 | `string` | **Yes** | Android,iOS | No |
| `preventNativeSelection` | 是否阻止该屏幕被原生选中，默认 false | `boolean` | No | Android,iOS | No |
| `specialEffects` | 重复选中标签页时的特效（popToRoot / scrollToTop） | `object` | No | Android,iOS | No |
| `testID` | TabsScreen 的 testID | `string` | No | Android,iOS | No |
| `accessibilityLabel` | TabsScreen 的无障碍标签 | `string` | No | Android,iOS | No |
| `tabBarItemTestID` | TabBarItem 的 testID | `string` | No | Android,iOS | No |
| `tabBarItemAccessibilityLabel` | TabBarItem 的无障碍标签 | `string` | No | Android,iOS | No |
| `scrollEdgeAppearance` | 滚动内容到达标签栏边缘时的标签栏外观 | `object` | No | iOS | No |
| `overrideScrollViewContentInsetAdjustmentBehavior` | 是否恢复首个 ScrollView 的 automatic inset 行为（仅 Fabric，默认 true） | `boolean` | No | iOS | No |
| `experimental_userInterfaceStyle` | 覆盖标签栏系统外观（实验性） | `'unspecified' \| 'light' \| 'dark'` | No | iOS | No |

## 遗留问题

- [ ] formSheet页面暂使用普通页面，该页面使用的Context.openBindSheet系统方法存在绑定的js页面绑定的位置错乱，造成按钮事件会无法响应。需要系统修正后补充。  [issue#4](https://github.com/react-native-oh-library/react-native-harmony-screens/issues/4)

- [ ] RNSScreenFooter依赖formSheet。需要实现formSheet功能后再补充该功能。  [issue#28](https://github.com/react-native-oh-library/react-native-harmony-screens/issues/28)

- [ ] GestureDetectorProvider按原库的引用方式在77环境上引用不到，需要系统框架定位处理。  [issue#29](https://github.com/react-native-oh-library/react-native-harmony-screens/issues/29)

- [ ] 在Screen和其子组件中通过Option有设置不同属性时，在77环境ets侧接收到的属性有丢失。需要系统框架定位处理后。  [issue#30](https://github.com/react-native-oh-library/react-native-harmony-screens/issues/30)

- [ ] sheetInitialDetent sheet 打开后应展开到的 detent 索引。formsheet功能的属性。 未实现
- [ ] sheetElevation 描述表单标高的整数值，影响表单顶部边缘的阴影。formsheet功能的属性。 未实现
- [ ] sheetAllowedDetents 描述表单可以放置的高度。formsheet功能的属性。 未实现
- [ ] sheetLargestUndimmedDetent 最大的表单棘爪，其下方视图不会变暗。formsheet功能的属性。 未实现
- [ ] sheetGrabberVisible 布尔值，指示工作表顶部是否显示抓取器。formsheet功能的属性。 未实现
- [ ] sheetCornerRadius 表单将尝试渲染的角半径。formsheet功能的属性。 未实现
- [ ] sheetExpandsWhenScrolledToEdge 滚动时，表单是否应扩展到更大的棘爪。formsheet功能的属性。 未实现
- [ ] onSheetDetentChanged 当当前屏幕处于“formSheet”演示状态并且其定位符已更改时调用的回调。formsheet功能的回调。 未实现
- [ ] scrollEdgeEffects，BottomTabs当前架构不支持。 未实现
- [ ] ScreenContentWrapper依赖formSheet。由于依赖的formSheet功能未实现，功能未实现。

## 其他

无

## 目录结构

````
/rntpc_react-native-screens  # 项目根目录
├── react-native-harmony-screens  # 鸿蒙适配 npm 包
│   ├── harmony                   # 鸿蒙原生代码
│   │   └── screens.har           # har 包
│   ├── src                       # RN 侧 TS 源码
│   │   ├── components             # RN 组件（Screen、ScreenContainer、ScreenStack 等）
│   │   ├── native-stack/views     # native-stack 相关视图
│   │   ├── specs                  # Codegen 生成的原生组件规格
│   │   ├── core.ts                # 核心逻辑
│   │   ├── index.ts               # 入口文件
│   │   └── utils.ts               # 工具函数
│   ├── scripts                   # 构建/发布脚本
│   ├── package.json
│   └── tsconfig.json
├── tester                        # 测试工程
│   ├── harmony                   # 鸿蒙测试工程
│   │   ├── entry                  # 应用入口模块
│   │   └── screens                # 鸿蒙适配源码（ArkTS + CPP）
│   │       └── src/main
│   │           ├── cpp            # C++ 侧（Package、JSIBinder、 TurboModule）
│   │           └── ets            # ArkTS 侧（组件、动画、Package）
│   ├── apps/src                  # 测试用例
│   ├── android                   # Android 测试工程
│   └── package.json
├── README.md                     # 中文安装使用方法
├── README_en.md                  # 英文安装使用方法
└── LICENSE                       # 开源协议
````

## 贡献代码

使用过程中发现任何问题都可以提交 [Issue](https://gitcode.com/CPF-RN/rntpc_react-native-screens/issues)，当然，也非常欢迎提交 [PR](https://gitcode.com/CPF-RN/rntpc_react-native-screens/pulls) 。

## 开源协议

本项目基于 [The MIT License (MIT)](https://github.com/software-mansion/react-native-screens/blob/main/LICENSE) ，请自由地享受和参与开源。

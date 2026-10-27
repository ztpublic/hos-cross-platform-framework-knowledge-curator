> 文档模板：v0.4.2

<p align="center">
  <h1 align="center"> <code>react-native-gesture-handler</code> </h1>
</p>

本项目基于 [react-native-gesture-handler](https://github.com/software-mansion/react-native-gesture-handler) 开发。

该第三方库的仓库已迁移至 Gitcode，且支持直接从 npm 下载，新的包名为：`@react-native-ohos/react-native-gesture-handler` 版本所属关系如下：

| 三方库名称    | 三方库版本（npm地址）    | 发布信息     | 支持RN版本    | Autolink     | 编译API版本     | 社区基线版本    |  源码地址   |
| ------------ | ------------ | ------------------------------ | ------------- | ------------- |------------------------ | ------------- | ------------- | 
| @react-native-ohos/react-native-gesture-handler | [~2.31.0(进行中)](https://www.npmjs.com/package/@react-native-ohos/react-native-gesture-handler)    | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/releases) | 0.84.* | 是 | API12+ | 2.33.0 | [master](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/tree/master) |
| @react-native-ohos/react-native-gesture-handler | [~2.30.1](https://www.npmjs.com/package/@react-native-ohos/react-native-gesture-handler)    | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/releases) | 0.82.* | 否 | API12+ | 2.30.0 | [br_rnoh0.82](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/tree/br_rnoh0.82) |
| @react-native-ohos/react-native-gesture-handler | [~2.23.2](https://www.npmjs.com/package/@react-native-ohos/react-native-gesture-handler)    | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/releases) | 0.77.* | 否 | API12+ | 2.25.0 | [br_rnoh0.77](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/tree/br_rnoh0.77) |
| @react-native-ohos/react-native-gesture-handler | [~2.14.18](https://www.npmjs.com/package/@react-native-ohos/react-native-gesture-handler)    | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/releases) | 0.72.* | 是 | API12+ | 2.14.1 | [br_rnoh0.72](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/tree/br_rnoh0.72) |
| @react-native-oh-tpl/react-native-gesture-handler  | [<= 2.14.17@deprecated](https://www.npmjs.com/package/@react-native-oh-tpl/react-native-gesture-handler) | [Github Releases(deprecated)](https://github.com/react-native-oh-library/react-native-gesture-handler/releases) | 0.72.* | 否 | API12+ | 2.12.1 | [sig](https://github.com/react-native-oh-library/react-native-gesture-handler) |

## 简介

React Native Gesture Handler 为 React Native 提供了原生手势处理能力，支持多种手势类型（如点击、拖拽、捏合、旋转等），并提供 Touchable 组件和手势检测器（GestureDetector），实现流畅的手势交互体验。

## 下载安装

请到三方库的 Releases 发布地址查看配套的版本信息：[@react-native-ohos/react-native-gesture-handler Releases](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/releases)。对于未发布到npm的旧版本，请参考[安装指南](https://gitcode.com/CPF-RN/usage-docs/blob/master/zh-cn/tgz-usage.md)安装tgz包。

进入到工程目录并输入以下命令：

**npm**

```bash
npm install @react-native-ohos/react-native-gesture-handler
```

**yarn**

```bash
yarn add @react-native-ohos/react-native-gesture-handler
```

## 使用示例

下面的代码展示了这个库的基本使用场景：

> [!WARNING] 使用时 import 的库名不变。

```js
import React, { Component } from "react";
import { Animated, Dimensions } from "react-native";
import {
  GestureHandlerRootView,
  PanGestureHandler,
} from "react-native-gesture-handler";

const { width } = Dimensions.get("screen");
const circleRadius = 30;

class Circle extends Component {
  _touchX = new Animated.Value(width / 2 - circleRadius);

  _onPanGestureEvent = Animated.event([{ nativeEvent: { x: this._touchX } }], {
    useNativeDriver: true,
  });

  render() {
    return (
      <GestureHandlerRootView>
        <PanGestureHandler onGestureEvent={this._onPanGestureEvent}>
          <Animated.View
            style={{
              height: 150,
              justifyContent: "center",
            }}
          >
            <Animated.View
              style={[
                {
                  backgroundColor: "#42a5f5",
                  borderRadius: circleRadius,
                  height: circleRadius * 2,
                  width: circleRadius * 2,
                },
                {
                  transform: [
                    {
                      translateX: Animated.add(
                        this._touchX,
                        new Animated.Value(-circleRadius)
                      ),
                    },
                  ],
                },
              ]}
            />
          </Animated.View>
        </PanGestureHandler>
      </GestureHandlerRootView>
    );
  }
}

export default function App() {
  return <Circle />;
}
```

### Gestures API（2.x）使用示例

下面的代码展示了 2.x Gestures API（`Gesture` 工厂方法 + `GestureDetector`）的使用场景。该 API 为当前推荐用法：手势通过链式调用完成配置与回调绑定，并通过 `Gesture.Simultaneous` 等方法组合使用。

```js
import React, { useState } from "react";
import { StyleSheet, View } from "react-native";
import {
  Gesture,
  GestureDetector,
  GestureHandlerRootView,
} from "react-native-gesture-handler";

export default function App() {
  const [position, setPosition] = useState({ x: 0, y: 0 });
  const [pressed, setPressed] = useState(false);

  // 创建平移手势：链式配置激活条件与回调
  const panGesture = Gesture.Pan()
    .minDistance(10)
    .onUpdate((event) => {
      setPosition({ x: event.translationX, y: event.translationY });
    })
    .onEnd(() => {
      setPosition({ x: 0, y: 0 });
    });

  // 创建长按手势
  const longPressGesture = Gesture.LongPress()
    .minDuration(600)
    .onStart(() => setPressed(true))
    .onFinalize(() => setPressed(false));

  // 通过 Simultaneous 组合两个手势，使其可同时识别
  const composedGesture = Gesture.Simultaneous(panGesture, longPressGesture);

  return (
    <GestureHandlerRootView style={styles.container}>
      <GestureDetector gesture={composedGesture}>
        <View
          style={[
            styles.box,
            {
              transform: [
                { translateX: position.x },
                { translateY: position.y },
                { scale: pressed ? 1.2 : 1 },
              ],
            },
          ]}
        />
      </GestureDetector>
    </GestureHandlerRootView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: "center",
    alignItems: "center",
  },
  box: {
    height: 120,
    width: 120,
    backgroundColor: "#42a5f5",
    borderRadius: 20,
  },
});
```

## Link

|                                      | 是否支持autolink | RN框架版本 |
|--------------------------------------|-----------------|------------|
| ~ 2.31.0（开发中）                       |  Yes             |  0.84     |

使用AutoLink的工程需要根据该文档配置，Autolink框架指导文档：https://gitcode.com/CPF-RN/ohos_react_native/blob/master/docs/zh-cn/Autolinking.md

如您使用的版本支持 Autolink，并且工程已接入 Autolink，可跳过ManualLink配置。

<details>
  <summary>ManualLink: 此步骤为手动配置原生依赖项的指导</summary>

首先需要使用 DevEco Studio 打开项目里的 HarmonyOS 工程 `harmony`

### 1. Overrides RN SDK

为了让工程依赖同一个版本的 RN SDK，需要在工程根目录的 `oh-package.json5` 添加 overrides 字段，指向工程需要使用的 RN SDK 版本。替换的版本既可以是一个具体的版本号，也可以是一个模糊版本，还可以是本地存在的 HAR 包或源码目录。

关于该字段的作用请阅读[官方说明](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides-V5/ide-oh-package-json5-V5#zh-cn_topic_0000001792256137_overrides)

```json
{
  ...
  "overrides": {
    "@rnoh/react-native-openharmony" : "./react_native_openharmony"
  }
}
```

### 2. 引入原生端代码

> [!TIP] 引入原生代码之前请确认IDE版本，5.0.3.810及其之后的版本需要在harmony工程中的hvigor-config.json5文件中新增如下配置以解决路径过长导致的编译报错问题

```json
"properties":{
  "ohos.nativeResolver":false
}
```
目前有两种方法：

1. 通过 har 包引入；
2. 直接链接源码。

方法一：通过 har 包引入（推荐）

> [!TIP] har 包位于三方库安装路径的 `harmony` 文件夹下。

打开 `entry/oh-package.json5`，添加以下依赖

```json
"dependencies": {
    "@rnoh/react-native-openharmony": "file:../react_native_openharmony",

    "@react-native-ohos/react-native-gesture-handler": "file:../../node_modules/@react-native-ohos/react-native-gesture-handler/harmony/gesture_handler.har"
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

### 3. 配置 CMakeLists 和引入 GestureHandlerPackage

打开 `entry/src/main/cpp/CMakeLists.txt`，添加：

```diff
project(rnapp)
cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_SKIP_BUILD_RPATH TRUE)
set(RNOH_APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
+ set(NODE_MODULES "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../node_modules")
+ set(OH_MODULES "${CMAKE_CURRENT_SOURCE_DIR}/../../../oh_modules")
set(RNOH_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../react-native-harmony/harmony/cpp")
set(LOG_VERBOSITY_LEVEL 1)
set(CMAKE_ASM_FLAGS "-Wno-error=unused-command-line-argument -Qunused-arguments")
set(CMAKE_CXX_FLAGS "-fstack-protector-strong -Wl,-z,relro,-z,now,-z,noexecstack -s -fPIE -pie")
set(WITH_HITRACE_SYSTRACE 1) # for other CMakeLists.txt files to use
add_compile_definitions(WITH_HITRACE_SYSTRACE)

add_subdirectory("${RNOH_CPP_DIR}" ./rn)

# RNOH_BEGIN: manual_package_linking_1
add_subdirectory("../../../../sample_package/src/main/cpp" ./sample-package)
+ add_subdirectory("${OH_MODULES}/@react-native-ohos/react-native-gesture-handler/src/main/cpp" ./gesture-handler)
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
+ target_link_libraries(rnoh_app PUBLIC rnoh_gesture_handler)
# RNOH_END: manual_package_linking_2
```

打开 `entry/src/main/cpp/PackageProvider.cpp`，添加：

```diff
#include "RNOH/PackageProvider.h"
+ #include "generated/RNOHGeneratedPackage.h"
+ #include "GestureHandlerPackage.h"

using namespace rnoh;

std::vector<std::shared_ptr<Package>> PackageProvider::getPackages(Package::Context ctx) {
    return {
+     std::make_shared<RNOHGeneratedPackage>(ctx),
+     std::make_shared<GestureHandlerPackage>(ctx)
    };
}
```

### 4. 在 ArkTs 侧引入 Gesture Handler Package

打开 `entry/src/main/ets/RNPackagesFactory.ts`，添加：

```diff
+ import GestureHandlerPackage from '@react-native-ohos/react-native-gesture-handler';

export function createRNPackages(ctx: RNPackageContext): RNPackage[] {
  return [
    new SamplePackage(ctx),
+   new GestureHandlerPackage(ctx),
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

本文档内容基于以下版本验证通过：

1. RNOH: 0.84.2; SDK: HarmonyOS 6.0.2 Release SDK; IDE: DevEco Studio 6.0.2.650; ROM: 6.1.0.135 SP8;

### 编译运行API要求（如有）

> [!TIP] 当前三方库所有版本均已实现版本隔离，支持在 `API12+` 工程编译，及 `API12+` ROM运行。

## 接口说明

> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

### Gestures

#### Gesture detector

GestureDetector 是 Gesture Handler 库 2.x 版本的一个主要组件。

##### Gesture detector 属性

> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| Name   | Description                                                                                  | Type                             | Required | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- | -------- |
| gesture    | 一个包含配置与回调的手势对象。                                     | base gestures or any ComposedGesture | yes      | All      | yes      |

##### Gesture 的方法
> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| method | Description                                                                                  | Type                             | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- |
| Gesture.Tap() | 使用默认配置且无回调创建一个 [TapGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/gestures/tap-gesture) 实例。 | function() | All      | yes    |
| Gesture.Pan() | 使用默认配置且无回调创建一个 [PanGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/gestures/pan-gesture) 实例。 | function() | All      | yes    |
| Gesture.Rotation() | 使用默认配置且无回调创建一个 [`RotationGesture`](https://docs.swmansion.com/react-native-gesture-handler/docs/gestures/rotation-gesture) 实例。 | function() | All      | yes    |
| Gesture.LongPress() | 使用默认配置且无回调创建一个 [`LongPressGesture`](https://docs.swmansion.com/react-native-gesture-handler/docs/gestures/long-press-gesture) 实例。 | function() | All      | yes    |
| Gesture.Manual() | 使用默认配置且无回调创建一个 [`ManualGesture`](https://docs.swmansion.com/react-native-gesture-handler/docs/gestures/manual-gesture) 实例。 | function() | All      | yes    |
| Gesture.Pinch() | 使用默认配置且无回调创建一个 [`PinchGesture`](https://docs.swmansion.com/react-native-gesture-handler/docs/gestures/pinch-gesture) 实例。 | function() | All      | yes    |
| Gesture.Fling() | 使用默认配置且无回调创建一个 [`FlingGesture`](https://docs.swmansion.com/react-native-gesture-handler/docs/gestures/fling-gesture) 实例。 | function() | All      | yes    |
| Gesture.Race() | 创建一个 [RaceGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/composed-gestures) 实例。将多个手势组合为竞速关系，第一个变为活跃状态的手势生效，其余手势将被阻止激活。 | function(...gestures) | All      | yes    |
| Gesture.Simultaneous() | 创建一个 [SimultaneousGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/composed-gestures) 实例。将多个手势组合为并行关系，允许这些手势同时识别并同时处于活跃状态。 | function(...gestures) | All      | yes    |
| Gesture.Exclusive() | 创建一个 [ExclusiveGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/composed-gestures) 实例。将多个手势组合为互斥关系，按传入顺序识别，仅当前面的手势识别失败后，后面的手势才可能被识别。 | function(...gestures) | All      | yes    |
| Gesture.Native() | 创建一个 [NativeGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/native-gesture) 实例，用于包装原生视图（如 ScrollView）自带的原生手势。 | function() | All      | yes    |
| Gesture.Hover() | 使用默认配置且无回调创建一个 [HoverGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/hover-gesture) 实例，用于响应指针在视图上的悬停移动（仅 Web 平台支持）。 | function() | Web      | no    |
| Gesture.ForceTouch() | 使用默认配置且无回调创建一个 [ForceTouchGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/force-touch-gesture) 实例。 | function() | All      | yes    |

##### 通用事件回调方法

以下事件回调方法为所有手势共有，可在创建手势后通过链式调用进行配置（如 `Gesture.Tap().onStart(...)`）。

> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| Name | Description                                                                                  | Type                             | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- |
| onBegin(callback) | 设置手势处理器开始接收触摸时触发的回调。此时处理器尚未进入 ACTIVE 状态，也无法确定手势能否被识别。 | function(callback) | All | yes |
| onStart(callback) | 设置手势被处理器识别并转为 ACTIVE 状态时触发的回调。 | function(callback) | All | yes |
| onEnd(callback) | 设置被识别的手势结束时触发的回调。仅当处理器之前处于 ACTIVE 状态时才会触发。 | function(callback) | All | yes |
| onFinalize(callback) | 设置处理器结束手势处理时触发的回调，无论手势识别成功还是失败均会触发。 | function(callback) | All | yes |
| onTouchesDown(callback) | 设置手指按下时触发的回调。 | function(callback) | All | yes |
| onTouchesMove(callback) | 设置手指移动时触发的回调。 | function(callback) | All | yes |
| onTouchesUp(callback) | 设置手指抬起时触发的回调。 | function(callback) | All | yes |
| onTouchesCancelled(callback) | 设置手指停止被跟踪时（例如手势结束）触发的回调。 | function(callback) | All | yes |

###### 通用方法

以下方法为所有手势共有，用于配置手势行为与手势间关系（如 `Gesture.Pan().enabled(false)`）。

| Name | Description                                                                                  | Type                             | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- |
| enabled(value) | 设置该手势是否分析触摸事件流。为 false 时手势状态永远不会变为 ACTIVE；若在手势识别过程中更新为 false，状态将立即变为 FAILED 或 CANCELLED（取决于当前状态）。默认为 true。 | function(value: boolean) | All | yes |
| shouldCancelWhenOutside(value) | 为 true 时，手指离开附加视图区域后手势将被取消或识别失败（取决于当前状态）。默认值因手势类型而异：多数手势默认为 false，LongPressGesture、TapGesture 与 NativeGesture（Android 和 Web 上）默认为 true。 | function(value: boolean) | All | yes |
| hitSlop(settings) | 控制可用来开始识别手势的视图区域范围。可传入负数（各边等量收缩），或包含 left、right、top、bottom、horizontal、vertical、width、height 字段的对象。 | function(settings) | All | yes |
| withRef(ref) | 为手势对象设置 ref，用于与旧 API 互操作。 | function(ref) | All | yes |
| withTestId(testID) | 为手势对象设置 testID，便于在测试中查询。 | function(testID) | All | yes |
| cancelsTouchesInView(value) | 为 true 时，手势变为 ACTIVE 后将取消其附加的原生 UI 组件（UIButton、UISwitch 等）的触摸。默认为 true。 | function(value: boolean) | IOS | no |
| runOnJS(value) | 安装 react-native-reanimated 后，传给手势的回调默认被 worklet 化并在 UI 线程执行；为 true 时回调改为在 JS 线程执行。默认为 false。 | function(value: boolean) | All | yes |
| simultaneousWithExternalGesture(...gestures) | 添加与本手势同时识别的外部手势。注意该方法仅标记手势间的关系，不会组合手势，外部手势仍需添加到其他 GestureDetector 才能被识别。 | function(...gestures) | All | yes |
| requireExternalGestureToFail(...gestures) | 添加关系：要求其他手势识别失败后，本手势才能激活。 | function(...gestures) | All | yes |
| blocksExternalGesture(...gestures) | 添加关系：使其他手势等待本手势失败（或未开始）后才允许激活。注意该方法同样仅标记关系，不会组合手势。 | function(...gestures) | All | yes |
| activeCursor(value) | 指定手势激活时使用的鼠标指针样式，支持所有 CSS cursor 值（如 "grab"、"zoom-in"）。默认为 "auto"。 | function(value) | Web | no |

###### 通用事件回调数据

以下事件回调数据为所有手势共有，会随各手势的回调参数一同提供。

| 名称 | 所属 API | 参数类型 | 默认值 | 必填 | 平台 | HarmonyOS平台支持 | 描述 |
| ---- | -------- | -------- | ------ | ---- | ---- | ----------------- | ---- |
| state | onBegin、onStart、onEnd、onFinalize | State | - | 是 | All | yes | 手势处理器的当前状态，取值为库导出的 State 对象中的常量。 |
| numberOfPointers | onBegin、onStart、onUpdate、onEnd、onFinalize | number | - | 是 | All | yes | 当前放在屏幕上的指针（手指）数量。 |
| pointerType | onBegin、onStart、onUpdate、onEnd、onFinalize | PointerType | - | 是 | All | no | 当前使用的指针设备类型，取值为 PointerType 枚举：TOUCH（手指）、STYLUS（触控笔）、MOUSE（鼠标）、KEY（键盘）、OTHER（未知设备类型）。 |

##### TapGesture

[TapGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/tap-gesture) 是识别一次或多次点击的离散手势，点击过程中的移动距离、次数与时间间隔均可配置。手势激活后会立即结束。

###### TapGesture 属性
> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| Name | Description                                                                                  | Type                             | Required | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- | -------- |
| minPointers(value) | 处理器激活前需要放置的最少指针（手指）数量，应为正整数。默认值为 1。 | function(value: number) | no | All | yes |
| maxDuration(value) | 手指触摸后必须在多快时间内抬起的最大时长（毫秒）。默认值为 500。 | function(value: number) | no | All | yes |
| maxDelay(value) | 需要多次点击时，两次点击之间允许经过的最大时长（毫秒）。默认值为 500。 | function(value: number) | no | All | yes |
| numberOfTaps(value) | 激活手势所需的点击次数。默认值为 1。 | function(value: number) | no | All | yes |
| maxDeltaX(value) | 点击手势期间手指沿 X 轴允许移动的最大距离（point）。若沿 X 轴移动超过该距离且手势尚未激活，将识别失败。 | function(value: number) | no | All | yes |
| maxDeltaY(value) | 点击手势期间手指沿 Y 轴允许移动的最大距离（point）。若沿 Y 轴移动超过该距离且手势尚未激活，将识别失败。 | function(value: number) | no | All | yes |
| maxDistance(value) | 点击手势期间手指允许移动的最大距离（point）。若移动超过该距离且手势尚未激活，将识别失败。 | function(value: number) | no | All | yes |
| mouseButton(value) | 指定手势响应的鼠标按键。可用 `\|` 运算符组合 MouseButton 枚举值（LEFT、RIGHT、MIDDLE、BUTTON_4、BUTTON_5、ALL），默认为 LEFT。 | function(value: MouseButton) | no | Web,Android | no |

###### TapGesture 事件回调数据

请查看[通用事件回调数据](#通用事件回调数据)。以下是 TapGesture 特有的事件回调数据。
> [!TIP] "Platform"列表示该字段在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该字段；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| 名称 | 所属 API | 参数类型 | 默认值 | 必填 | 平台 | HarmonyOS平台支持 | 描述 |
| ---- | -------- | -------- | ------ | ---- | ---- | ----------------- | ---- |
| x | onBegin、onStart、onEnd、onFinalize | number | - | 是 | All | yes | 指针（单指或多指的主指）相对于附加到 GestureDetector 的视图的当前 X 坐标，单位为 point。 |
| y | onBegin、onStart、onEnd、onFinalize | number | - | 是 | All | yes | 指针（单指或多指的主指）相对于附加到 GestureDetector 的视图的当前 Y 坐标，单位为 point。 |
| absoluteX | onBegin、onStart、onEnd、onFinalize | number | - | 是 | All | yes | 指针（单指或多指的主指）相对于窗口的当前 X 坐标，单位为 point。若手势会导致视图变换，推荐使用 absoluteX 而不是 x。 |
| absoluteY | onBegin、onStart、onEnd、onFinalize | number | - | 是 | All | yes | 指针（单指或多指的主指）相对于窗口的当前 Y 坐标，单位为 point。若手势会导致视图变换，推荐使用 absoluteY 而不是 y。 |

##### PanGesture

[PanGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/pan-gesture) 是识别平移（拖拽）并跟踪其移动的连续手势。手指按下并移动一定初始距离后激活，可配置最小初始距离、指定方向的平移检测以及激活所需的手指数（支持多指滑动）。

###### PanGesture 属性
> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| Name | Description                                                                                  | Type                             | Required | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- | -------- |
| minDistance(value) | 手势激活前手指（或多指）需要移动的最小距离，单位为 point。 | function(value: number) | no | All | yes |
| minVelocity(value) | 手势激活前指针需要达到的最小速度，单位为 point/s。 | function(value: number) | no | All | yes |
| minVelocityX(value) | 手势激活前指针沿 X 轴需要达到的最小速度，单位为 point/s。 | function(value: number) | no | All | yes |
| minVelocityY(value) | 手势激活前指针沿 Y 轴需要达到的最小速度，单位为 point/s。 | function(value: number) | no | All | yes |
| minPointers(value) | 手势激活前需要放置的最少手指数量，应为大于等于 0 的整数。 | function(value: number) | no | All | yes |
| maxPointers(value) | 当屏幕上放置到指定数量的手指且手势尚未激活时，将停止识别该手势。应为大于等于 0 的整数。 | function(value: number) | no | All | yes |
| activateAfterLongPress(duration) | 长按超过该时长（毫秒）后 Pan 手势才允许激活；若期间手指移动，手势将识别失败。默认值为 0，即不要求先长按。 | function(duration: number) | no | All | yes |
| activeOffsetX(value) | X 轴方向（point）内手指移动不会触发激活的区间，移出该区间意味着激活手势。可传入数组（第一个值须小于等于 0、第二个值须大于等于 0）或单个数字（p 大于等于 0 时表示 (-inf, p)，否则表示 (-p, inf)）。 | function(value: number \| number[]) | no | All | yes |
| activeOffsetY(value) | Y 轴方向（point）内手指移动不会触发激活的区间，规则同 activeOffsetX。 | function(value: number \| number[]) | no | All | yes |
| failOffsetX(value) | 当手指沿 X 轴移出该区间（point）且手势尚未激活时，手势将识别失败。区间规则同 activeOffsetX。 | function(value: number \| number[]) | no | All | yes |
| failOffsetY(value) | 当手指沿 Y 轴移出该区间（point）且手势尚未激活时，手势将识别失败。区间规则同 activeOffsetX。 | function(value: number \| number[]) | no | All | yes |
| averageTouches(value) | Android 默认以主指（最先按下的手指）位置计算 translation 值，该选项可改为使用所有活跃指针的平均位置（即 iOS 的默认行为）。 | function(value: boolean) | no | Android | no |
| enableTrackpadTwoFingerGesture(value) | 在支持的设备（例如带触摸板的 iPad）上启用双指手势；未启用时手势需要"点按 + 拖拽"触发。 | function(value: boolean) | no | IOS | no |
| mouseButton(value) | 指定手势响应的鼠标按键。可用 `\|` 运算符组合 MouseButton 枚举值，默认为 LEFT。 | function(value: MouseButton) | no | Web,Android | no |

###### PanGesture 事件回调数据

请查看[通用事件回调数据](#通用事件回调数据)。以下是 PanGesture 特有的事件回调数据。
> [!TIP] "Platform"列表示该字段在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该字段；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| 名称 | 所属 API | 参数类型 | 默认值 | 必填 | 平台 | HarmonyOS平台支持 | 描述 |
| ---- | -------- | -------- | ------ | ---- | ---- | ----------------- | ---- |
| translationX | onBegin、onStart、onUpdate、onEnd、onFinalize | number | - | 是 | All | yes | 手势过程中沿 X 轴累计的平移距离，单位为 point。 |
| translationY | onBegin、onStart、onUpdate、onEnd、onFinalize | number | - | 是 | All | yes | 手势过程中沿 Y 轴累计的平移距离，单位为 point。 |
| velocityX | onBegin、onStart、onUpdate、onEnd、onFinalize | number | - | 是 | All | yes | 当前沿 X 轴的平移速度，单位为 point/s。 |
| velocityY | onBegin、onStart、onUpdate、onEnd、onFinalize | number | - | 是 | All | yes | 当前沿 Y 轴的平移速度，单位为 point/s。 |
| x | onBegin、onStart、onUpdate、onEnd、onFinalize | number | - | 是 | All | yes | 指针（单指或多指的主指）相对于附加到 GestureDetector 的视图的当前 X 坐标，单位为 point。 |
| y | onBegin、onStart、onUpdate、onEnd、onFinalize | number | - | 是 | All | yes | 指针（单指或多指的主指）相对于附加到 GestureDetector 的视图的当前 Y 坐标，单位为 point。 |
| absoluteX | onBegin、onStart、onUpdate、onEnd、onFinalize | number | - | 是 | All | yes | 指针（单指或多指的主指）相对于窗口的当前 X 坐标，单位为 point。若手势会导致视图变换，推荐使用 absoluteX 而不是 x。 |
| absoluteY | onBegin、onStart、onUpdate、onEnd、onFinalize | number | - | 是 | All | yes | 指针（单指或多指的主指）相对于窗口的当前 Y 坐标，单位为 point。若手势会导致视图变换，推荐使用 absoluteY 而不是 y。 |
| stylusData | onUpdate、onEnd | Object | - | 否 | All | no | 包含触控笔附加信息的对象，字段包括 tiltX（Y-Z 平面与屏幕的夹角）、tiltY（X-Z 平面与屏幕的夹角）、altitudeAngle（笔轴与屏幕 X-Y 平面的夹角）、azimuthAngle（Y-Z 平面与包含笔轴和 Y 轴的平面的夹角）、pressure（归一化压力值）。仅触控笔输入时提供。 |

##### RotationGesture

[RotationGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/rotation-gesture) 是识别旋转并跟踪其移动的连续手势，两指按到屏幕上并按正确方式改变位置后激活。

###### RotationGesture 属性

RotationGesture 未提供特有属性，可配置项请查看[通用方法](#通用方法)。

###### RotationGesture 事件回调数据

请查看[通用事件回调数据](#通用事件回调数据)。以下是 RotationGesture 特有的事件回调数据。
> [!TIP] "Platform"列表示该字段在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该字段；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| 名称 | 所属 API | 参数类型 | 默认值 | 必填 | 平台 | HarmonyOS平台支持 | 描述 |
| ---- | -------- | -------- | ------ | ---- | ---- | ----------------- | ---- |
| rotation | onBegin、onStart、onUpdate、onEnd、onFinalize | number | - | 是 | All | yes | 相对于手势焦点（锚点）旋转的量，单位为弧度。 |
| velocity | onBegin、onStart、onUpdate、onEnd、onFinalize | number | - | 是 | All | yes | 手势的瞬时速度，单位为 point/s。 |
| anchorX | onBegin、onStart、onUpdate、onEnd、onFinalize | number | - | 是 | All | yes | 手势中心焦点（锚点）的 X 坐标，单位为 point。 |
| anchorY | onBegin、onStart、onUpdate、onEnd、onFinalize | number | - | 是 | All | yes | 手势中心焦点（锚点）的 Y 坐标，单位为 point。 |

##### LongPressGesture

[LongPressGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/long-press-gesture) 是识别长时间按住的离散手势，手指松开或移动超过允许距离前按住达到最短时长后激活，激活后立即结束。

###### LongPressGesture 属性
> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| Name | Description                                                                                  | Type                             | Required | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- | -------- |
| minDuration(value) | 手指在对应视图上按住的最短时长（毫秒），超过后手势才会激活。默认值为 500。 | function(value: number) | no | All | yes |
| maxDistance(value) | 长按手势期间手指允许移动的最大距离（point）。若移动超过该距离且手势尚未激活，将识别失败。默认值为 10。 | function(value: number) | no | All | yes |
| mouseButton(value) | 指定手势响应的鼠标按键。可用 `\|` 运算符组合 MouseButton 枚举值，默认为 LEFT。 | function(value: MouseButton) | no | Web,Android | no |

###### LongPressGesture 事件回调数据

请查看[通用事件回调数据](#通用事件回调数据)。以下是 LongPressGesture 特有的事件回调数据。
> [!TIP] "Platform"列表示该字段在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该字段；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| 名称 | 所属 API | 参数类型 | 默认值 | 必填 | 平台 | HarmonyOS平台支持 | 描述 |
| ---- | -------- | -------- | ------ | ---- | ---- | ----------------- | ---- |
| x | onBegin、onStart、onEnd、onFinalize | number | - | 是 | All | yes | 指针（单指或多指的主指）相对于附加到 GestureDetector 的视图的当前 X 坐标，单位为 point。 |
| y | onBegin、onStart、onEnd、onFinalize | number | - | 是 | All | yes | 指针（单指或多指的主指）相对于附加到 GestureDetector 的视图的当前 Y 坐标，单位为 point。 |
| absoluteX | onBegin、onStart、onEnd、onFinalize | number | - | 是 | All | yes | 指针（单指或多指的主指）相对于窗口的当前 X 坐标，单位为 point。若手势会导致视图变换，推荐使用 absoluteX 而不是 x。 |
| absoluteY | onBegin、onStart、onEnd、onFinalize | number | - | 是 | All | yes | 指针（单指或多指的主指）相对于窗口的当前 Y 坐标，单位为 point。若手势会导致视图变换，推荐使用 absoluteY 而不是 y。 |
| duration | onBegin、onStart、onEnd、onFinalize | number | - | 是 | All | yes | 长按的持续时长（自手势开始起计算），单位为毫秒。 |

##### ManualGesture

[ManualGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/manual-gesture) 不识别任何手势，仅用于通过 GestureStateManager 手动控制手势状态，需要配合 `onTouchesDown` 等触摸回调使用。

###### ManualGesture 属性

ManualGesture 未提供特有属性，可配置项请查看[通用方法](#通用方法)。

###### ManualGesture 事件回调数据

ManualGesture 未提供特有的事件回调数据，请查看[通用事件回调数据](#通用事件回调数据)。

##### PinchGesture

[PinchGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/pinch-gesture) 是识别捏合（双指缩放）的连续手势，通过跟踪两指间距离提供缩放因子，可用于缩放或缩放内容。

###### PinchGesture 属性

PinchGesture 未提供特有属性，可配置项请查看[通用方法](#通用方法)。

###### PinchGesture 事件回调数据

请查看[通用事件回调数据](#通用事件回调数据)。以下是 PinchGesture 特有的事件回调数据。
> [!TIP] "Platform"列表示该字段在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该字段；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| 名称 | 所属 API | 参数类型 | 默认值 | 必填 | 平台 | HarmonyOS平台支持 | 描述 |
| ---- | -------- | -------- | ------ | ---- | ---- | ----------------- | ---- |
| scale | onBegin、onStart、onUpdate、onEnd、onFinalize | number | - | 是 | All | yes | 缩放因子，相对于屏幕坐标系中两指的位置。手势开始时为 1.0，随两指间距离的增大而按比例增大，随距离的减小而减小。 |
| velocity | onBegin、onStart、onUpdate、onEnd、onFinalize | number | - | 是 | All | yes | 当前时刻捏合手势的速度，以"缩放因子/秒"表示。 |
| focalX | onBegin、onStart、onUpdate、onEnd、onFinalize | number | - | 是 | All | yes | 手势中心锚点（两指焦点）沿 X 轴的坐标，单位为 point。 |
| focalY | onBegin、onStart、onUpdate、onEnd、onFinalize | number | - | 是 | All | yes | 手势中心锚点（两指焦点）沿 Y 轴的坐标，单位为 point。 |

##### FlingGesture

[FlingGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/fling-gesture) 是识别快速滑动的离散手势，移动距离足够长且耗时足够短时激活，激活后转为 END 状态。

###### FlingGesture 属性
> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| Name | Description                                                                                  | Type                             | Required | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- | -------- |
| direction(value) | 允许的滑动方向，取值为 Directions 对象导出的常量，可用 `\|` 运算符组合多个方向（如 `Directions.RIGHT \| Directions.LEFT`）。 | function(value: Directions) | no | All | yes |
| numberOfPointers(value) | 触发 fling 手势所需的指针（手指）数量。 | function(value: number) | no | All | yes |
| mouseButton(value) | 指定手势响应的鼠标按键。可用 `\|` 运算符组合 MouseButton 枚举值，默认为 LEFT。 | function(value: MouseButton) | no | Web,Android | no |

###### FlingGesture 事件回调数据

请查看[通用事件回调数据](#通用事件回调数据)。以下是 FlingGesture 特有的事件回调数据。
> [!TIP] "Platform"列表示该字段在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该字段；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| 名称 | 所属 API | 参数类型 | 默认值 | 必填 | 平台 | HarmonyOS平台支持 | 描述 |
| ---- | -------- | -------- | ------ | ---- | ---- | ----------------- | ---- |
| x | onBegin、onStart、onEnd、onFinalize | number | - | 是 | All | yes | 指针（单指或多指的主指）相对于附加到 GestureDetector 的视图的当前 X 坐标，单位为 point。 |
| y | onBegin、onStart、onEnd、onFinalize | number | - | 是 | All | yes | 指针（单指或多指的主指）相对于附加到 GestureDetector 的视图的当前 Y 坐标，单位为 point。 |
| absoluteX | onBegin、onStart、onEnd、onFinalize | number | - | 是 | All | yes | 指针（单指或多指的主指）相对于窗口的当前 X 坐标，单位为 point。若手势会导致视图变换，推荐使用 absoluteX 而不是 x。 |
| absoluteY | onBegin、onStart、onEnd、onFinalize | number | - | 是 | All | yes | 指针（单指或多指的主指）相对于窗口的当前 Y 坐标，单位为 point。若手势会导致视图变换，推荐使用 absoluteY 而不是 y。 |

##### NativeGesture

[NativeGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/native-gesture) 用于包装原生视图（如 ScrollView）自带的原生手势，使其能参与 Gesture Handler 的手势编排，通常配合 `createNativeWrapper` 或包装组件使用。

###### NativeGesture 属性
> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| Name | Description                                                                                  | Type                             | Required | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- | -------- |
| shouldActivateOnStart(value) | 为 true 时，底层 handler 在 BEGAN 或 UNDETERMINED 状态下收到任何触摸即无条件激活。 | function(value: boolean) | no | Android | yes |
| disallowInterruption(value) | 为 true 时，该手势进入 ACTIVE 状态后将取消其他所有手势处理器。 | function(value: boolean) | no | All | yes |

###### NativeGesture 事件回调数据

请查看[通用事件回调数据](#通用事件回调数据)。以下是 NativeGesture 特有的事件回调数据。
> [!TIP] "Platform"列表示该字段在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该字段；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| 名称 | 所属 API | 参数类型 | 默认值 | 必填 | 平台 | HarmonyOS平台支持 | 描述 |
| ---- | -------- | -------- | ------ | ---- | ---- | ----------------- | ---- |
| pointerInside | onBegin、onStart、onEnd、onFinalize | boolean | - | 是 | All | yes | 手势是否在所附加的视图范围内执行，在范围内为 true，否则为 false。 |

##### ForceTouchGesture

[ForceTouchGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/force-touch-gesture) 是识别按压（压力触控）的连续手势，压力达到最小值后激活，压力超过最大值则识别失败。

###### ForceTouchGesture 属性
> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| Name | Description                                                                                  | Type                             | Required | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- | -------- |
| minForce(value) | 手势激活所需的最小压力，取值范围为 [0.0, 1.0]。默认值为 0.2。 | function(value: number) | no | All | yes |
| maxForce(value) | 允许施加的最大压力，压力超过该值手势将识别失败。取值范围为 [0.0, 1.0]。 | function(value: number) | no | All | yes |
| feedbackOnActivation(value) | 手势激活时是否执行触觉反馈（震动）。 | function(value: boolean) | no | All | yes |

###### ForceTouchGesture 事件回调数据

请查看[通用事件回调数据](#通用事件回调数据)。以下是 ForceTouchGesture 特有的事件回调数据。
> [!TIP] "Platform"列表示该字段在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该字段；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| 名称 | 所属 API | 参数类型 | 默认值 | 必填 | 平台 | HarmonyOS平台支持 | 描述 |
| ---- | -------- | -------- | ------ | ---- | ---- | ----------------- | ---- |
| force | onBegin、onStart、onUpdate、onEnd、onFinalize | number | - | 是 | All | yes | 触碰的压力。 |

### Components

#### Touchables

Gesture Handler 库提供了一种基于原生按钮的 React Native touchable 组件的实现，它不依赖于 React Native 的 JS responder system。这些组件的实现遵循相同的 API，旨在替代 React Native 中的 touchable 组件,HarmonyOS 已全面支持。

- [Touchable Opacity](https://reactnative.dev/docs/touchableopacity)
- [Touchable Without Feedback](https://reactnative.dev/docs/touchablewithoutfeedback)
- [Touchable Highlight](https://reactnative.dev/docs/touchablehighlight)
- [Touchable Native Feedback](https://reactnative.dev/docs/touchablenativefeedback)

#### Buttons

BaseButton 属性
| Name | Description                                                                                  | Type                             | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- |
| onActiveStateChange | 当按钮从非激活切换到激活或反向切换时触发。 | function | All   | yes   |
| onPress | 按钮被按下时触发。 | function | All   | yes   |
| onLongPress | 按钮按住至少 `delayLongPress` 毫秒后触发。 | function | All   | yes   |
| exclusive | 是否允许多个按钮同时被按下，默认值为 `true`。 | boolean | All   | yes   |
| delayLongPress | 触发 `onLongPress` 前的延迟（毫秒），默认 600。 | number | All   | yes   |

RectButton 属性
| Name | Description                                                                                  | Type                             | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- |
| underlayColor | 按钮处于激活态时会被调暗的背景色。 | Color | All   | yes   |
| activeOpacity | 按钮处于激活状态时，底色的不透明度被应用。 | number | IOS   | no   |



RawButton 属性

| Name          | Description                        | Type  | Platform | HarmonyOS Support |
| ------------- | ---------------------------------- | ----- | -------- | ----------------- |
| exclusive | 定义是否可以同时按下多个按钮。默认情况下设置为“真”。 | boolean | All      | yes               |
| style | 额外样式对象。 | ViewStyle | All      | yes               |
| onLayout | 布局更改时被调用。 | function | All      | yes               |
| rippleColor | 水波纹颜色。 | number | All      | NO             |
| rippleRadius | 水波纹半径。 | number | All      | NO             |
| borderless | 设置水波纹扩散效果是否超出组件本身边界。 | boolean | All      | NO             |
| touchSoundDisabled | 是否点击按钮声音反馈。 | boolean | All      | NO             |
| foreground | 设置水波纹是否为前景扩散。 | boolean | All      | NO             |



Pressable 属性

| Name                            | Description                                          | Type      | Platform | HarmonyOS Support |
| ------------------------------- | ---------------------------------------------------- | --------- | -------- | ----------------- |
| children                        | 设置子组件 | ReactNode | All      | yes               |
| cancelable                      | 设置按压事件是否被外部滚动事件所打断，默认为打断。               | boolean | All      | yes               |
| delayLongPress                  | 设置触发长按事件的时间限制。                     | number | All      | yes               |
| disabled                        | 是否禁用按压操作。                                | boolean | All      | yes             |
| hitSlop                         | 在此视野之外的其他区域中，若检测到有压力存在，则会触发事件。           | number\|object    | All      | yes             |
| pressRetentionOffset            | 在按下后允许手指移动的偏移量。 | number\|object | All      | yes             |
| style                           | 在按下状态时设置样式。                     | ViewStyle | All      | yes             |
| simultaneousWithExternalGesture | 设置手势与之同时响应。                 | RelationPropType | All      | yes             |
| requireExternalGestureToFail | 等待外部手势响应失败后响应按压事件。          | RelationPropType | All      | yes             |
| blocksExternalGesture | 阻止外部手势响应。                   | RelationPropType | All      | yes             |
| onPress | 当检测到单次点击手势时会触发此事件。         | function | All      | yes             |
| onPressIn | 当触摸操作在“onPress”事件之前被触发时会调用此方法。 | function | All      | yes             |
| onPressOut | 当触摸操作在“onPress”事件之前被释放时触发此事件。 | function | All      | yes             |
| onLongPress | 当检测到长按手势时会调用此函数。           | function | All      | yes             |

Pressable 事件类型（PressableEvent）

`onPress` / `onPressIn` / `onPressOut` / `onLongPress` 回调均接收 `PressableEvent` 类型的事件对象。

| Name | Description                                                                                  | Type                             | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- |
| identifier | 触摸点的唯一标识。 | number | All   | yes   |
| locationX | 触摸点相对于组件左上角的 X 坐标。 | number | All   | yes   |
| locationY | 触摸点相对于组件左上角的 Y 坐标。 | number | All   | yes   |
| pageX | 触摸点相对于页面左上角的 X 坐标。 | number | All   | yes   |
| pageY | 触摸点相对于页面左上角的 Y 坐标。 | number | All   | yes   |
| target | 接收事件的目标节点 ID（恒为 0）。 | number | no   | no   |
| timestamp | 事件的时间戳（毫秒）。 | number | All   | yes   |
| touches | 当前屏幕上所有触摸点的列表(恒为空数组)。 | InnerPressableEvent[] | no   | no   |
| changedTouches | 本次事件中发生变化的触摸点(恒为空数组)。 | InnerPressableEvent[] | no   | no   |
| force | 按压力度（0~1），可选字段（恒 undefined）。 | number | no   | no   |

#### Drawer Layout

属性
| Name | Description                                                                                  | Type                             | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- |
| drawerType | 抽屉的展示类型。                                     | 'front'\|'back'\|'slide' | All | yes |
| edgewidth | 定义手势距离内容视图边缘多远时才会被激活。 | number | All   | yes   |
| hideStatusBar | 设为 `true` 时，抽屉被拉出或处于打开状态会通过 [StatusBar](https://reactnative.dev/docs/statusbar.html) API 隐藏系统状态栏。 | boolean | All   | yes   |
| statusbaranimation | 在 `hideStatusBar` 为 `true` 时，选择隐藏/显示状态栏使用的动画。 | 'fade'\|'slide'\|'none' | All   | no   |
| overlaycolor | 抽屉打开时覆盖在内容视图之上的半透明遮罩颜色。 | color | All   | yes   |
| rendernavigationview | 标准实现中已包含的必填参数，用于渲染导航视图。 | function | All   | yes   |
| ondrawerclose | 抽屉关闭时调用。 | function | All   | yes   |
| ondraweropen | 抽屉打开时调用。 | function | All   | yes   |
| ondrawerslide | 抽屉因触摸事件滑动打开时调用。 | function | All   | yes   |
| ondrawerstatechanged | 抽屉状态变化时调用。 | function | All   | yes   |
| children | 默认渲染并被抽屉包裹的子组件。 | component\| function | All   | yes   |


#### ReanimatedDrawerLayout

属性
| Name | Description                                                                                  | Type                             | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- |
| renderNavigationView | 用于渲染抽屉内容                             | function | All | yes |
| drawerPosition | 确定抽屉将从哪个方向开启。 | DrawerPosition {<br/>  LEFT,<br/>  RIGHT,<br/>} | All   | yes   |
| drawerWidth | 抽屉的宽度。 | number | All   | yes   |
| drawerBackgroundColor | 抽屉的背景颜色。 | string | All   | yes   |
| drawerLockMode | 指定抽屉的锁定模式。 | enum DrawerLockMode {<br/>  UNLOCKED,<br/>  LOCKED_CLOSED,<br/>  LOCKED_OPEN,<br/>} | All   | yes   |
| keyboardDismissMode | 设置在拖动抽屉时是否应关闭系统键盘。 | enum DrawerKeyboardDismissMode {<br/>  NONE,<br/>  ON_DRAG,<br/>} | All   | yes   |
| onDrawerClose | 当抽屉关闭时触发此事件。 | function | All   | yes   |
| onDrawerOpen | 当抽屉打开时触发此事件。 | function | All   | yes   |
| onDrawerStateChanged | 当抽屉的状态发生变化时触发此事件。 | function | All   | yes   |
| drawerType | 当打开抽屉时将播放的动画类型。 | enum DrawerType {<br/>  FRONT,<br/>  BACK,<br/>  SLIDE,<br/>} | All   | yes   |
| animationSpeed | 当拖拽松开或关闭抽屉时所播放的动画速度。 | number | All   | yes   |
| edgeWidth | 设置手指在距离内容视图边缘多远的位置时才会被激活。 | number | All   | yes   |
| minSwipeDistance | 在抽屉开始被拖动之前，最小滑动距离为多少。 | number | All   | yes   |
| hideStatusBar | 设置当抽屉在处于打开状态时是否隐藏状态栏。 | boolean | All   | yes   |
| statusBarAnimation | 在 hideStatusBar 为 true 时，设置隐藏状态栏时的动画效果。 | 'none' \| 'fade' \|'slide' | All   | no |
| overlayColor | 滑动抽屉时，主体内容的遮罩颜色。 | string | All   | yes   |
| contentContainerStyle | 主体内容区域的样式。 | ViewStyle | All   | yes   |
| drawerContainerStyle | 抽屉内容区域的样式。 | ViewStyle | All   | yes   |
| enableTrackpadTwoFingerGesture | 在支持的设备（例如带有触摸板的 iPad）上支持双指操作。 | boolean | IOS | no |
| onDrawerSlide | 当手动滑动抽屉在滑动过程中触发此事件。 | function | All   | yes   |
| children | 设置子组件。 | ReactNode | All   | yes   |
| userSelect | 设置是否允许对抽屉内部及上下文窗口内的文本进行选择。 | 'none' \| 'auto' \| 'text' | Web | no |
| activeCursor | 当抽屉被拖动时，设置显示的光标图标。 | string | Web | no |
| mouseButton | 设置哪个鼠标按键可以触发抽屉滑动。 | number | Web | no |
| enableContextMenu | 控制按钮是否允许触发右键菜单（上下文菜单）。 | boolean | Web | no |
| openDrawer | 打开抽屉。 | 接口 | All   | yes   |
| closeDrawer | 关闭抽屉。 | 接口 | All   | yes   |

#### Swipeable

属性
| Name | Description                                                                                  | Type                             | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- |
| friction | 视觉交互相对手势距离的延滞程度。 | number | All | yes |
| leftThreshold | 释放面板后，从左侧到达该距离就会动画到打开状态。 | number | All | yes |
| rightThreshold | 释放面板后，从右侧到达该距离就会动画到打开状态。 | number | All | yes |
| dragOffsetFromLeftEdge | 从左边缘开始拖动到被视为滑动所需的距离。 | number                   | All | yes |
| dragOffsetFromRightEdge | 从右边缘开始拖动到被视为滑动所需的距离。 | number | All | yes |
| overshootLeft | 是否允许可滑动面板被拉出超过左侧动作面板宽度。 | boolean | All | yes |
| overshootRight | 是否允许可滑动面板被拉出超过右侧动作面板宽度。 | boolean | All | yes |
| overshootFriction | 超出范围时视觉交互相对手势距离的延滞程度。 | number                   | All | yes |
| onSwipeableOpen | 动作面板打开时调用。 | function(direction: 'left' \| 'right', swipeable: Swipeable) | All | yes |
| onSwipeableClose | 动作面板关闭时调用。 | function(direction: 'left' \| 'right', swipeable: Swipeable) | All | yes |
| onSwipeableWillOpen | 动作面板开始动画打开（左/右）时调用。 | function((direction: 'left' \| 'right')) | All | yes |
| onSwipeableWillClose | 动作面板开始动画关闭时调用。 | function((direction: 'left' \| 'right')) | All | yes |
| renderLeftActions | 用于返回左侧动作面板的渲染方法，提供的插值输入区间可用于额外动画。 | function(progressAnimatedValue: AnimatedInterpolation,<br/>    dragAnimatedValue: AnimatedInterpolation,<br/>    swipeable: Swipeable) | All | yes |
| renderRightActions | 用于返回右侧动作面板的渲染方法（向左滑动时显露）。 | function(progressAnimatedValue: AnimatedInterpolation,<br/>    dragAnimatedValue: AnimatedInterpolation,<br/>    swipeable: Swipeable) | All | yes |
| containerStyle | 容器（Animated.View）的样式对象，例如覆盖 `overflow: 'hidden'`。 | StyleProp<ViewStyle> | All | yes |
| childrenContainerStyle | 子容器（Animated.View）的样式对象，例如应用 `flex: 1`。 | StyleProp<ViewStyle> | All | yes |

#### ReanimatedSwipeable

属性
| Name | Description                                                                                  | Type                             | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- |
| children | 设置子组件。 | ReactNode | All | yes |
| enabled | 设置操作面板是否可滑动。 | boolean | All | yes |
| hitSlop | 在此视野之外的其他区域中，若检测到有拖拽手势，也会拖动滑块。 | number\|object | All | yes |
| enableTrackpadTwoFingerGesture | 在支持的设备（例如带有触摸板的 iPad）上支持双指操作。 | boolean            | IOS | no |
| friction | 指定了视觉交互相对于手势距离的延迟程度。 | number | All | yes |
| leftThreshold | 设置滑动面板向右侧滑动最小值才能打开面板 | number | All | yes |
| rightThreshold | 设置滑动面板向左侧滑动最小值才能打开面板 | number | All | yes |
| dragOffsetFromLeftEdge | 滑动面板从左侧边缘拖动的距离。 | number                   | All | yes |
| dragOffsetFromRightEdge | 滑动面板从右侧边缘拖动的距离。 | number | All | yes |
| overshootLeft | 可滑动面板是否能够超出左侧操作面板的宽度，然后松手回弹。 | boolean | All | yes |
| overshootRight | 可滑动面板是否能够超出右侧操作面板的宽度，然后松手回弹。 | boolean | All | yes |
| overshootFriction | 滑动超出操作面板后阻力大小设置，当值越大时，滑动超出操作面板后滑动阻力就越大 | number | All | yes |
| onSwipeableOpen | 当操作面板打开后触发此事件。 | function | All | yes |
| onSwipeableClose | 当操作面板关闭后触发此事件。 | function | All | yes |
| onSwipeableWillOpen | 当操作面板在打开（无论是向右还是向左）开始进行动画效果时触发此事件。 | function | All | yes |
| onSwipeableWillClose | 当操作面板在关闭开始动画时触发此事件。 | function   | All | yes |
| onSwipeableOpenStartDrag | 当操作面板开始被拖动打开时触发此事件。 | function | All | yes |
| onSwipeableCloseStartDrag | 当操作面板开始被拖动关闭时触发此事件。 | function | All | yes |
| renderLeftActions | 左侧按钮视图。 | function | All | yes |
| renderRightActions | 右侧按钮视图。 | function | All | yes |
| animationOptions | 动画配置参数。 | Record | All | yes |
| containerStyle | 滑块区域的样式设置。 | ViewStyle | All | yes |
| childrenContainerStyle | 用于子容器的样式对象。 | ViewStyle | All | yes |
| simultaneousWithExternalGesture | 设置手势与之同时响应。 | RelationPropType | All | yes |
| requireExternalGestureToFail | 等待外部手势响应失败后响应按压事件。 | RelationPropType | All | yes |
| blocksExternalGesture | 阻止外部手势响应。 | RelationPropType | All | yes |
| close | 关闭滑块。 | 接口 | All | yes |
| openLeft | 滑块从左侧打开。 | 接口 | All | yes |
| openRight | 滑块从右侧打开。 | 接口 | All | yes |
| reset | 重置滑块状态到关闭状态。 | 接口 | All | yes |

#### 包装组件（Wrapped Components）

以下组件通过 `createNativeWrapper` 包装了 React Native 的原生组件（即在原生组件外包一层 NativeViewGestureHandler），使其注册进 Gesture Handler 的手势编排系统，从而可被其他手势通过 `waitFor`、`simultaneousWithExternalGesture` 等关系引用。这些组件的属性与被包装的 RN 原生组件一致（另支持 [NativeViewGestureHandler 属性](#nativeviewgesturehandler-属性)），此处不再重复罗列。

> [!TIP] "Platform"列表示该组件在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该组件；no 则表示不支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| Name | Description                                                                                  | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | -------- | -------- |
| ScrollView | 包装 RN ScrollView，默认手势配置为 `disallowInterruption: true`、`shouldCancelWhenOutside: false`；当传入 `refreshControl` 时，会自动将其手势加入 `waitFor`，避免下拉刷新与其他手势冲突。 | All | yes |
| FlatList | 包装 RN FlatList，内部使用上述 RNGH ScrollView 作为滚动容器，属性与 RN FlatList 一致。 | All | yes |
| RefreshControl | 包装 RN RefreshControl，默认手势配置为 `disallowInterruption: true`，使下拉刷新不被其他手势打断。通常无需直接使用，配合 RNGH ScrollView 时已自动处理。 | All | yes |
| Switch | 包装 RN Switch，默认手势配置为 `shouldActivateOnStart: true`、`disallowInterruption: true`。 | All | yes |
| TextInput | 包装 RN TextInput，无特殊默认手势配置，仅将其注册进手势系统。 | All | yes |
| DrawerLayoutAndroid | 包装 RN DrawerLayoutAndroid。该 RN 组件仅 Android 平台提供，HarmonyOS 上不可用，请使用 [Drawer Layout](#drawer-layout) 或 [ReanimatedDrawerLayout](#reanimateddrawerlayout) 替代。 | Android | no |
| Text | 包装 RN Text。当设置 `onPress`/`onLongPress` 时，按压通过 `Gesture.Native().runOnJS(true)` 走 Gesture Handler 系统处理，而非 RN 的 responder 系统。 | All | yes |


### Gesture handlers(legacy)

> [!WARNING] 建议使用新的 [gestures API](https://docs.swmansion.com/react-native-gesture-handler/docs/gestures/gesture)。旧的 API 已不再积极维护，也不会接收新功能。查看 [Introduction 中的 RNGH 2.0 章节](https://docs.swmansion.com/react-native-gesture-handler/docs/#rngh-20) 了解更多信息。

#### Gesture handlers 通用属性
> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| NAME                | Description                                                                                                                                   | TYPE                     | Required | Platform | HarmonyOS Support |
| ----------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------- | -------- | -------- | -------- |
| enabled                 | 指定该手势处理器是否应当分析触摸事件流。                                                            | boolean                      | no       | All      | yes      |
| hitSlop                 | 控制可用来开始识别手势的视图区域范围。                            | object                       | no       | All      | yes      |
| onGestureEvent          | 当处理器处于 ACTIVE 状态时，对后续触摸事件逐个触发的回调。                           | callback                     | no       | All      | yes      |
| onHandlerStateChange    | 当处理器状态变化时触发的回调。                                                           | callback                     | no       | All      | yes      |

#### Gesture handlers 通用事件数据

以下是提供给 `onGestureEvent` 和 `onHandlerStateChange` 回调的通用事件数据:
> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| NAME         | Description                                                             | TYPE | Platform | HarmonyOS Support |
| ---------------- | --------------------------------------------------------------------------- | ------ | -------- | -------- |
| state            | 处理器的当前状态。                                               | State  | All      | yes      |
| numberOfPointers | 当前放在屏幕上的指针（手指）数量。 | number | All      | yes      |

#### PanGestureHandler

##### PanGestureHandler 属性
> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| NAME                       | Description                                                                                                                                                                             | TYPE | Required | Platform | HarmonyOS Support |
| ------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------- | -------- | -------- | -------- |
| minDist                        | 手指（或多指）需要移动的最小距离，超过后处理器才会激活。                                                                                               | number  | no       | All      | yes      |
| minPointers                    | 处理器激活前需要放置的最少手指数量。                                                                                                              | number  | no       | All      | yes      |
| maxPointers                    | 当屏幕上放置到指定数量的手指且尚未激活时，将停止识别该手势。                                                             | number  | no       | All      | yes      |
| activeOffsetX                  | X 轴方向（点单位）内，不会触发处理器激活的移动区间。                                                                                                         | number  | no       | All      | yes      |
| activeOffsetY                  | Y 轴方向（点单位）内，不会触发处理器激活的移动区间。                                                                                                         | number  | no       | All      | yes      |

##### PanGestureHandler 事件数据

请查看[基础 handler 类的通用事件数据](#gesture-handlers-通用事件数据)。以下是 PanGestureHandler 特有的事件数据。
> [!TIP] "Platform"列表示该字段在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该字段；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| 名称         | 所属 API                        | 参数类型 | 默认值 | 必填 | 平台 | HarmonyOS平台支持 | 描述                                                                   |
| ------------ | ------------------------------- | -------- | ------ | ---- | ---- | ----------------- | ---------------------------------------------------------------------- |
| translationX | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 手势过程中沿 X 轴累计的平移距离，单位为 point。                         |
| translationY | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 手势过程中沿 Y 轴累计的平移距离，单位为 point。                         |
| velocityX    | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 当前沿 X 轴的平移速度，单位为 point/s。                                 |
| velocityY    | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 当前沿 Y 轴的平移速度，单位为 point/s。                                 |
| x            | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 指针（单指或多指的主指）相对于附加到处理器的视图的当前 X 坐标，单位为 point。 |
| y            | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 指针（单指或多指的主指）相对于附加到处理器的视图的当前 Y 坐标，单位为 point。 |

#### TapGestureHandler

##### TapGestureHandler 属性
> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| NAME      | Description                                                                                                                    | TYPE | Required | Platform | HarmonyOS Support |
| ------------- | ---------------------------------------------------------------------------------------------------------------------------------- | ------ | -------- | -------- | -------- |
| minPointers   | 处理器激活前需要放置的最少手指数量。                                           | number | no       | All      | yes      |
| maxDurationMs | 手指触摸后必须在多快时间内抬起的最大时长（毫秒）。                            | number | no       | All      | yes      |
| maxDelayMs    | 在需要多次点击时，两次点击之间允许经过的最大时间（毫秒）。                            | number | no       | All      | yes      |
| maxDeltaX     | 点击手势期间手指在 X 轴允许移动的最大距离（point）。 | number | no       | All      | yes      |
| maxDeltaY     | 点击手势期间手指在 Y 轴允许移动的最大距离（point）。 | number | no       | All      | yes      |
| maxDist       | 点击手势期间手指允许移动的最大距离（point）。                  | number | no       | All      | yes      |

##### TapGestureHandler 事件数据

请查看[基础 handler 类的通用事件数据](#gesture-handlers-通用事件数据)。以下是 TapGestureHandler 特有的事件数据。
> [!TIP] "Platform"列表示该字段在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该字段；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| 名称      | 所属 API                             | 参数类型 | 默认值 | 必填 | 平台 | HarmonyOS平台支持 | 描述                                                                          |
| --------- | ------------------------------------ | -------- | ------ | ---- | ---- | ----------------- | ----------------------------------------------------------------------------- |
| x         | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 指针（单指或多指的主指）相对于附加到处理器的视图的当前 X 坐标，单位为 point。      |
| y         | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 指针（单指或多指的主指）相对于附加到处理器的视图的当前 Y 坐标，单位为 point。      |
| absoluteX | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 指针（单指或多指的主指）相对于窗口的当前 X 坐标，单位为 point。若手势会导致视图变换，推荐使用 absoluteX 而不是 x。 |
| absoluteY | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 指针（单指或多指的主指）相对于窗口的当前 Y 坐标，单位为 point。若手势会导致视图变换，推荐使用 absoluteY 而不是 y。 |

#### ForceTouchGestureHandler

##### ForceTouchGestureHandler属性
> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| NAME      | Description                                                                                                                    | TYPE | Required | Platform | HarmonyOS Support |
| ------------- | ---------------------------------------------------------------------------------------------------------------------------------- | ------ | -------- | -------- | -------- |
| minForce | 最小压力                                       | number | no       | All      | yes      |
| maxForce | 最大压力。                        | number | no       | All      | yes      |
| feedbackOnActivation | 手势激活时是否需要触觉反馈（震动）。           | boolean | no       | All      | yes      |

##### ForceTouchGestureHandler事件数据

以下是 ForceTouchGestureHandler 特有的事件数据。
> [!TIP] "Platform"列表示该字段在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该字段；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| 名称  | 所属 API                             | 参数类型 | 默认值 | 必填 | 平台 | HarmonyOS平台支持 | 描述             |
| ----- | ------------------------------------ | -------- | ------ | ---- | ---- | ----------------- | ---------------- |
| force | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 触碰带来的压力。 |

#### LongPressGestureHandler

##### LongPressGestureHandler 属性
> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| NAME          | Description                                                                                                                   | TYPE   | Required | Platform | HarmonyOS Support |
| ------------- | ----------------------------------------------------------------------------------------------------------------------------- | ------ | -------- | -------- | -------- |
| minDurationMs | 手指在对应视图上按住的最短时长（毫秒），超过后处理器才会激活。默认值为 500。                                                  | number | no       | All      | yes      |
| maxDist       | 长按手势期间手指允许移动的最大距离（point）。若手指移动超过该距离且处理器尚未激活，手势识别将失败。默认值为 10。              | number | no       | All      | yes      |

##### LongPressGestureHandler 事件数据

请查看[基础 handler 类的通用事件数据](#gesture-handlers-通用事件数据)。以下是 LongPressGestureHandler 特有的事件数据。
> [!TIP] "Platform"列表示该字段在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该字段；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| 名称      | 所属 API                             | 参数类型 | 默认值 | 必填 | 平台 | HarmonyOS平台支持 | 描述                                                                          |
| --------- | ------------------------------------ | -------- | ------ | ---- | ---- | ----------------- | ----------------------------------------------------------------------------- |
| x         | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 指针（单指或多指的主指）相对于附加到处理器的视图的当前 X 坐标，单位为 point。      |
| y         | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 指针（单指或多指的主指）相对于附加到处理器的视图的当前 Y 坐标，单位为 point。      |
| absoluteX | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 指针（单指或多指的主指）相对于窗口的当前 X 坐标，单位为 point。若手势会导致视图变换，推荐使用 absoluteX 而不是 x。 |
| absoluteY | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 指针（单指或多指的主指）相对于窗口的当前 Y 坐标，单位为 point。若手势会导致视图变换，推荐使用 absoluteY 而不是 y。 |
| duration  | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 长按的持续时长（自事件开始起计算），单位为毫秒。                               |

#### PinchGestureHandler

##### PinchGestureHandler 属性

PinchGestureHandler 未在基础 handler 类的通用属性之外提供特有属性。

##### PinchGestureHandler 事件数据

请查看[基础 handler 类的通用事件数据](#gesture-handlers-通用事件数据)。以下是 PinchGestureHandler 特有的事件数据。
> [!TIP] "Platform"列表示该字段在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该字段；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| 名称     | 所属 API                             | 参数类型 | 默认值 | 必填 | 平台 | HarmonyOS平台支持 | 描述                                                                                          |
| -------- | ------------------------------------ | -------- | ------ | ---- | ---- | ----------------- | --------------------------------------------------------------------------------------------- |
| scale    | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 缩放因子，相对于屏幕坐标系中两指的位置。手势开始时为 1.0，随两指间距离的增大而按比例增大，随距离的减小而减小。 |
| velocity | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 当前时刻捏合手势的速度，以“缩放因子/秒”表示。                                                |
| focalX   | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 手势中心锚点（两指焦点）沿 X 轴的坐标，单位为 point。                                        |
| focalY   | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 手势中心锚点（两指焦点）沿 Y 轴的坐标，单位为 point。                                        |

#### RotationGestureHandler

##### RotationGestureHandler 属性

RotationGestureHandler 未在基础 handler 类的通用属性之外提供特有属性。

##### RotationGestureHandler 事件数据

请查看[基础 handler 类的通用事件数据](#gesture-handlers-通用事件数据)。以下是 RotationGestureHandler 特有的事件数据。
> [!TIP] "Platform"列表示该字段在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该字段；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| 名称     | 所属 API                             | 参数类型 | 默认值 | 必填 | 平台 | HarmonyOS平台支持 | 描述                                                 |
| -------- | ------------------------------------ | -------- | ------ | ---- | ---- | ----------------- | ---------------------------------------------------- |
| rotation | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 相对于手势焦点（锚点）旋转的量，单位为弧度。         |
| velocity | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 手势的瞬时速度，单位为 point/s。                    |
| anchorX  | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 手势中心焦点（锚点）的 X 坐标，单位为 point。       |
| anchorY  | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 手势中心焦点（锚点）的 Y 坐标，单位为 point。       |

#### FlingGestureHandler

##### FlingGestureHandler 属性
> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| NAME             | Description                                                                              | TYPE       | Required | Platform | HarmonyOS Support |
| ---------------- | ---------------------------------------------------------------------------------------- | ---------- | -------- | -------- | -------- |
| direction        | 允许的滑动方向，可通过 `\|` 运算符组合多个方向（如 `Directions.RIGHT \| Directions.LEFT`）。 | Directions | no       | All      | yes      |
| numberOfPointers | 触发 fling 手势所需的指针（手指）数量。                                                  | number     | no       | All      | yes      |

##### FlingGestureHandler 事件数据

请查看[基础 handler 类的通用事件数据](#gesture-handlers-通用事件数据)。以下是 FlingGestureHandler 特有的事件数据。
> [!TIP] "Platform"列表示该字段在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该字段；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| 名称      | 所属 API                             | 参数类型 | 默认值 | 必填 | 平台 | HarmonyOS平台支持 | 描述                                                                          |
| --------- | ------------------------------------ | -------- | ------ | ---- | ---- | ----------------- | ----------------------------------------------------------------------------- |
| x         | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 指针（单指或多指的主指）相对于附加到处理器的视图的当前 X 坐标，单位为 point。      |
| y         | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 指针（单指或多指的主指）相对于附加到处理器的视图的当前 Y 坐标，单位为 point。      |
| absoluteX | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 指针（单指或多指的主指）相对于窗口的当前 X 坐标，单位为 point。若手势会导致视图变换，推荐使用 absoluteX 而不是 x。 |
| absoluteY | onGestureEvent、onHandlerStateChange | number   | -      | 是   | All  | yes               | 指针（单指或多指的主指）相对于窗口的当前 Y 坐标，单位为 point。若手势会导致视图变换，推荐使用 absoluteY 而不是 y。 |

#### NativeViewGestureHandler

##### NativeViewGestureHandler 属性
> [!TIP] "Platform"列表示该属性在原三方库上支持的平台。

> [!TIP] "HarmonyOS Support"列为 yes 表示 HarmonyOS 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| NAME                 | Description                                                                                       | TYPE    | Required | Platform | HarmonyOS Support |
| -------------------- | ------------------------------------------------------------------------------------------------- | ------- | -------- | -------- | -------- |
| shouldActivateOnStart | 为 true 时，底层 handler 在 BEGAN 或 UNDETERMINED 状态下将无条件激活。                            | boolean | no       | Android  | yes      |
| disallowInterruption | 为 true 时，当该 NativeViewGestureHandler 进入 ACTIVE 状态后将取消其他所有手势处理器。            | boolean | no       | All      | yes      |

##### NativeViewGestureHandler 事件数据

NativeViewGestureHandler 未提供特有的事件数据，请查看[基础 handler 类的通用事件数据](#gesture-handlers-通用事件数据)。

## 遗留问题

- [ ] statusbaranimation属性由于ArkTs bar 只有淡出淡入动画和RN框架状态栏动画不支持，该功能暂不支持，问题: [issue#55](https://github.com/react-native-oh-library/react-native-harmony-gesture-handler/issues/55)
- [ ] 0.82上游社区升级遗留问题，问题: [issue#39](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/issues/39)

## 其他

- 当使用框架 release 包时（react_native_openharmony_release.har），因为手势库用到了 deviceinfo 接口，需要在工程的 cmake 配置导入libdeviceinfo_ndk.z.so
```diff
# CMakeLists.txt
target_link_libraries(cpplib PUBLIC libace_napi.z.so 
+ libdeviceinfo_ndk.z.so)
```
## 目录结构

````
/rntpc_react-native-gesture-handler         # 项目根目录
├── react-native-harmony-gesture-handler    # 三方库源码（npm 包 @react-native-ohos/react-native-gesture-handler）
│   ├── harmony                             # 鸿蒙适配 har 包产物目录
│   │   └── gesture_handler.har             # 鸿蒙适配 har 包
│   ├── src                                 # RN/TS 源码
│   │   ├── components                      # 组件实现（Pressable、ReanimatedSwipeable 等）
│   │   ├── handlers                        # 手势 Handler 定义
│   │   ├── specs                           # TurboModule/Codegen 类型定义
│   │   └── index.ts                        # 库入口文件
│   ├── DrawerLayout                        # DrawerLayout 子路径入口
│   ├── Swipeable                           # Swipeable 子路径入口
│   ├── ReanimatedDrawerLayout              # ReanimatedDrawerLayout 子路径入口
│   ├── ReanimatedSwipeable                 # ReanimatedSwipeable 子路径入口
│   ├── scripts                             # 构建、发布脚本
│   ├── img                                 # 文档图片资源
│   ├── CHANGELOG.md                        # 版本变更记录
│   ├── COMMITTERS.md                       # 贡献者名单
│   └── package.json                        # 包配置文件
├── tester                                  # 测试工程（Demo）
│   ├── harmony                             # DevEco Studio 鸿蒙工程
│   │   ├── entry                           # 应用入口模块
│   ├── android                             # Android Demo 工程
│   ├── rnoh_package                        # 本地依赖包（rnoh-testerino）
│   ├── src                                 # Demo 页面源码
│   └── scripts                             # 工程辅助脚本
├── jest                                    # 单元测试
├── buildEnv.sh                             # 构建环境脚本
├── README.md                               # 中文安装使用方法
├── README_en.md                            # 英文安装使用方法
└── LICENSE                                 # 开源协议
````

## 贡献代码

使用过程中发现任何问题都可以提交 [Issue](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/issues)，当然，也非常欢迎提交 [PR](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/pulls) 。

## 开源协议

本项目基于 [The MIT License (MIT)](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/blob/master/LICENSE) ，请自由地享受和参与开源。

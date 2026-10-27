> Document Template：v0.4.2

<p align="center">
  <h1 align="center"> <code>react-native-gesture-handler</code> </h1>
</p>

This project is based on [react-native-gesture-handler](https://github.com/software-mansion/react-native-gesture-handler).

This third-party library has been migrated to Gitcode and is now available for direct download from npm, the new package name is: `@react-native-ohos/react-native-gesture-handler`, The version correspondence details are as follows:

| Name | Version(Npm Address) | Release Information | Supported RN Version | Supported Autolink | Compile API Version | Community Baseline Version | Source code address |
| --------------| -------------- | ------------------------------ | ------------- | ------------- |------------------------ | ------------ | ------------- |
| @react-native-ohos/react-native-gesture-handler | [~2.31.0(in progress)](https://www.npmjs.com/package/@react-native-ohos/react-native-gesture-handler)    | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/releases) | 0.84.* | Yes | API12+ | 2.33.0 | [master](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/tree/master) |
| @react-native-ohos/react-native-gesture-handler | [~2.30.1](https://www.npmjs.com/package/@react-native-ohos/react-native-gesture-handler)    | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/releases) | 0.82.* | No | API12+ | 2.30.0 | [br_rnoh0.82](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/tree/br_rnoh0.82) |
| @react-native-ohos/react-native-gesture-handler | [~2.23.2](https://www.npmjs.com/package/@react-native-ohos/react-native-gesture-handler)    | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/releases) | 0.77.* | No | API12+ | 2.25.0 | [br_rnoh0.77](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/tree/br_rnoh0.77) |
| @react-native-ohos/react-native-gesture-handler | [~2.14.18](https://www.npmjs.com/package/@react-native-ohos/react-native-gesture-handler)    | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/releases) | 0.72.* | Yes | API12+ | 2.14.1 | [br_rnoh0.72](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/tree/br_rnoh0.72) |
| @react-native-oh-tpl/react-native-gesture-handler  | [<= 2.14.17@deprecated](https://www.npmjs.com/package/@react-native-oh-tpl/react-native-gesture-handler) | [Github Releases(deprecated)](https://github.com/react-native-oh-library/react-native-gesture-handler/releases) | 0.72.* | No | API12+ | 2.12.1 | [sig](https://github.com/react-native-oh-library/react-native-gesture-handler) |

## Introduction

React Native Gesture Handler provides native gesture handling capabilities for React Native, supporting multiple gesture types (tap, drag, pinch, rotation, etc.), and provides Touchable components and GestureDetector for smooth gesture interaction.

## Installation

Find the matching version information in the release address of a third-party library: [@react-native-ohos/react-native-gesture-handler Releases](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/releases). For older versions that are not published to npm, please refer to the [installation guide](https://gitcode.com/CPF-RN/usage-docs/blob/master/en/tgz-usage-en.md) to install the tgz package.

Go to the project directory and execute the following instruction:

**npm**

```bash
npm install @react-native-ohos/react-native-gesture-handler
```

**yarn**

```bash
yarn add @react-native-ohos/react-native-gesture-handler
```

## Example

The following code shows the basic use scenario of the repository:

> [!WARNING] The name of the imported repository remains unchanged.

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

### Gestures API (2.x) Example

The following code shows the use scenario of the 2.x Gestures API (`Gesture` factory methods + `GestureDetector`). This is the currently recommended usage: gestures are configured and given callbacks through chained calls, and can be combined with methods such as `Gesture.Simultaneous`.

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

  // Create a pan gesture: configure activation conditions and callbacks by chaining
  const panGesture = Gesture.Pan()
    .minDistance(10)
    .onUpdate((event) => {
      setPosition({ x: event.translationX, y: event.translationY });
    })
    .onEnd(() => {
      setPosition({ x: 0, y: 0 });
    });

  // Create a long press gesture
  const longPressGesture = Gesture.LongPress()
    .minDuration(600)
    .onStart(() => setPressed(true))
    .onFinalize(() => setPressed(false));

  // Combine the two gestures with Simultaneous so that they are recognized simultaneously
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

|                                      | Autolink Support | RN Framework Version |
|--------------------------------------|-----------------|------------|
| ~ 2.31.0(in progress)                       |  Yes             |  0.84     |

Projects using AutoLink should follow this guide:
https://gitcode.com/CPF-RN/ohos_react_native/blob/master/docs/zh-cn/Autolinking.md

If your selected version supports AutoLink and your project has integrated AutoLink, you can skip the ManualLink configuration.
<details>
  <summary>ManualLink: This step provides guidance for manually configuring native dependencies.</summary>

Open the `harmony` directory of the HarmonyOS project in DevEco Studio.

### 1. Overrides RN SDK

To ensure the project relies on the same version of the RN SDK, you need to add an `overrides` field in the project's root `oh-package.json5` file, specifying the RN SDK version to be used. The replacement version can be a specific version number, a semver range, or a locally available HAR package or source directory.

For more information about the purpose of this field, please refer to the [official documentation](https://developer.huawei.com/consumer/en/doc/harmonyos-guides-V5/ide-oh-package-json5-V5#en-us_topic_0000001792256137_overrides).

```json
{
  ...
  "overrides": {
    "@rnoh/react-native-openharmony" : "./react_native_openharmony"
  }
}
```

### 2. Introducing Native Code

> [!TIP] Before introducing native code, please check the IDE version. For version 5.0.3.810 and later, you need to add the following configuration to the hvigor-config.json5 file in the harmony project to solve the compilation error caused by the path being too long.

```json
"properties":{
      "ohos.nativeResolver":false
}
```
Currently, two methods are available:

1. Use the HAR file.
2. Directly link to the source code.

Method 1 (recommended): Use the HAR file.

> [!TIP]  The HAR file is stored in the `harmony` directory in the installation path of the third-party library.

Open `entry/oh-package.json5` file and add the following dependencies:

```json
"dependencies": {
    "@rnoh/react-native-openharmony": "file:../react_native_openharmony",

    "@react-native-ohos/react-native-gesture-handler": "file:../../node_modules/@react-native-ohos/react-native-gesture-handler/harmony/gesture_handler.har"
  }
```

Click the `sync` button in the upper right corner.

Alternatively, run the following instruction on the terminal:

```bash
cd entry
ohpm install
```

Method 2: Directly link to the source code.

> [!TIP] For details, see [Directly Linking Source Code](https://gitcode.com/CPF-RN/usage-docs/blob/master/en/link-source-code.md)

### 3. Configuring CMakeLists and Introducing GestureHandlerPackage

Open `entry/src/main/cpp/CMakeLists.txt` and add the following code:

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

Open `entry/src/main/cpp/PackageProvider.cpp` and add the following code:

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

### 4. Introducing Gesture Handler Package to ArkTS

Open the`entry/src/main/ets/RNPackagesFactory.ts ` file and add the following code:

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

### Running

Click the `sync` button in the upper right corner.

Alternatively, run the following instruction on the terminal:

```bash
cd entry
ohpm install
```

Then build and run the code.

## Constraints

### Compatibility

This document is verified based on the following versions:

1. RNOH: 0.84.2; SDK: HarmonyOS 6.0.2 Release SDK; IDE: DevEco Studio 6.0.2.650; ROM: 6.1.0.135 SP8;

### API requirements (if any)

> [!TIP] All versions of the current third-party libraries have implemented version isolation, supporting compilation in `API12+` projects and execution on `API12+` ROMs.

## API

> [!TIP] The **Platform** column indicates the platform where the property is supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same across platforms and the effect is aligned with iOS or Android.

### Gestures

#### Gesture detector

GestureDetector is a major component of the Gesture Handler library version 2.x.

##### Gesture detector Properties

> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name   | Description                                                                                  | Type                             | Required | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- | -------- |
| gesture    | A gesture object containing the configuration and callbacks.                                     | base gestures or any ComposedGesture | yes      | All      | yes      |

##### Gesture Method
> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| method | Description                                                                                  | Type                             | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- |
| Gesture.Tap() | Creates a new instance of [TapGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/gestures/tap-gesture) with its default config and no callbacks. | function | All      | yes    |
| Gesture.Pan() | Creates a new instance of [PanGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/gestures/pan-gesture) with its default config and no callbacks. | function | All      | yes    |
| Gesture.Rotation() | Creates a new instance of [`RotationGesture`](https://docs.swmansion.com/react-native-gesture-handler/docs/gestures/rotation-gesture) with its default config and no callbacks. | function | All      | yes    |
| Gesture.LongPress() | Creates a new instance of [`LongPressGesture`](https://docs.swmansion.com/react-native-gesture-handler/docs/gestures/long-press-gesture) with its default config and no callbacks. | function | All      | yes    |
| Gesture.Manual() | Creates a new instance of [`ManualGesture`](https://docs.swmansion.com/react-native-gesture-handler/docs/gestures/manual-gesture) with its default config and no callbacks. | function | All      | yes    |
| Gesture.Pinch() | Creates a new instance of [`PinchGesture`](https://docs.swmansion.com/react-native-gesture-handler/docs/gestures/pinch-gesture) with its default config and no callbacks. | function | All      | yes    |
| Gesture.Fling() | Creates a new instance of [`FlingGesture`](https://docs.swmansion.com/react-native-gesture-handler/docs/gestures/fling-gesture) with its default config and no callbacks. | function | All      | yes    |
| Gesture.Race() | Creates a new instance of [RaceGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/composed-gestures). Combines multiple gestures into a race: the first gesture to become active wins, and the remaining gestures are prevented from activating. | function(...gestures) | All      | yes    |
| Gesture.Simultaneous() | Creates a new instance of [SimultaneousGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/composed-gestures). Combines multiple gestures so that they can be recognized simultaneously and stay active at the same time. | function(...gestures) | All      | yes    |
| Gesture.Exclusive() | Creates a new instance of [ExclusiveGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/composed-gestures). Combines multiple gestures into an exclusive relation: gestures are recognized in the given order, and a gesture can be recognized only when all the preceding ones fail. | function(...gestures) | All      | yes    |
| Gesture.Native() | Creates a new instance of [NativeGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/native-gesture) used to wrap gestures of native views (e.g. ScrollView). | function | All      | yes    |
| Gesture.Hover() | Creates a new instance of [HoverGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/hover-gesture) with its default config and no callbacks. Responds to the pointer hovering over the view (supported on Web only). | function | Web      | no    |
| Gesture.ForceTouch() | Creates a new instance of [ForceTouchGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/force-touch-gesture) with its default config and no callbacks. | function | All      | yes    |

##### Common Event Callbacks

The following event callbacks are common to all gestures. They can be configured by chaining after the gesture is created (e.g. `Gesture.Tap().onStart(...)`).

> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name | Description                                                                                  | Type                             | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- |
| onBegin(callback) | Sets the callback that is being called when the given gesture handler starts receiving touches. At the moment of this callback the handler is not yet in an active state and we don't know yet if it will recognize the gesture at all. | function(callback) | All | yes |
| onStart(callback) | Sets the callback that is being called when the gesture is recognized by the handler and it transitions to the ACTIVE state. | function(callback) | All | yes |
| onEnd(callback) | Sets the callback that is being called when the gesture that was recognized by the handler finishes. It will be called only if the handler was previously in the ACTIVE state. | function(callback) | All | yes |
| onFinalize(callback) | Sets the callback that is being called when the handler finalizes handling the gesture, regardless of whether it was recognized or failed. | function(callback) | All | yes |
| onTouchesDown(callback) | Sets the callback that is called every time a finger is placed on the screen. | function(callback) | All | yes |
| onTouchesMove(callback) | Sets the callback that is called every time a finger is moved on the screen. | function(callback) | All | yes |
| onTouchesUp(callback) | Sets the callback that is called every time a finger is lifted from the screen. | function(callback) | All | yes |
| onTouchesCancelled(callback) | Sets the callback that is called every time a finger stops being tracked, for example when the gesture finishes. | function(callback) | All | yes |

###### Common Methods

The following methods are common to all gestures. They configure gesture behavior and relations between gestures (e.g. `Gesture.Pan().enabled(false)`).

| Name | Description                                                                                  | Type                             | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- |
| enabled(value) | Indicates whether the given handler should be analyzing the stream of touch events or not. When set to false, the handler's state will never become ACTIVE. If the value gets updated while the handler has already started recognizing a gesture, the state will immediately change to FAILED or CANCELLED (depending on its current state). Default value is true. | function(value: boolean) | All | yes |
| shouldCancelWhenOutside(value) | When true, the handler will cancel or fail recognition (depending on its current state) whenever the finger leaves the area of the connected view. The default value differs by gesture type: most gestures default to false, while LongPressGesture, TapGesture and NativeGesture (on Android and web) default to true. | function(value: boolean) | All | yes |
| hitSlop(settings) | Enables control over what part of the connected view area can be used to begin recognizing the gesture. Accepts a negative number (reducing the bounds evenly on each side) or an object with left, right, top, bottom, horizontal, vertical, width and height attributes. | function(settings) | All | yes |
| withRef(ref) | Sets a ref to the gesture object, allowing for interoperability with the old API. | function(ref) | All | yes |
| withTestId(testID) | Sets a testID property for the gesture object, allowing for querying for it in tests. | function(testID) | All | yes |
| cancelsTouchesInView(value) | When true, the gesture will cancel touches for native UI components (UIButton, UISwitch, etc.) it's attached to when it becomes ACTIVE. Default value is true. | function(value: boolean) | IOS | no |
| runOnJS(value) | When react-native-reanimated is installed, the callbacks passed to the gestures are automatically workletized and run on the UI thread when called. When true, all the callbacks will be run on the JS thread instead. Defaults to false. | function(value: boolean) | All | yes |
| simultaneousWithExternalGesture(...gestures) | Adds a gesture that should be recognized simultaneously with this one. Note that this method only marks the relation between gestures without composing them; the other gestures still need to be added to a GestureDetector to be recognized. | function(...gestures) | All | yes |
| requireExternalGestureToFail(...gestures) | Adds a relation requiring other gestures to fail before this one can activate. | function(...gestures) | All | yes |
| blocksExternalGesture(...gestures) | Adds a relation that makes other gestures wait with activation until this gesture fails (or doesn't start at all). Note that this method only marks the relation between gestures without composing them. | function(...gestures) | All | yes |
| activeCursor(value) | Allows specifying which cursor should be used when the gesture activates. Supports all CSS cursor values (e.g. "grab", "zoom-in"). Default value is set to "auto". | function(value) | Web | no |

###### Common Event Data

The following event data is common to all gestures and is provided along with each gesture's callback payloads.

| Name | Related API | Type | Default | Required | Platform | HarmonyOS Support | Description |
| ---- | -------- | -------- | ------ | ---- | ---- | ----------------- | ---- |
| state | onBegin, onStart, onEnd, onFinalize | State | - | yes | All | yes | Current state of the handler. Expressed as one of the constants exported under the State object by the library. |
| numberOfPointers | onBegin, onStart, onUpdate, onEnd, onFinalize | number | - | yes | All | yes | Represents the number of pointers (fingers) currently placed on the screen. |
| pointerType | onBegin, onStart, onUpdate, onEnd, onFinalize | PointerType | - | yes | All | no | Indicates the type of pointer device in use, represented by the PointerType enum: TOUCH (finger), STYLUS (stylus or digital pen), MOUSE (computer mouse), KEY (keyboard), OTHER (unknown device type). |

##### TapGesture

[TapGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/tap-gesture) is a discrete gesture that recognizes one or many taps. The allowed distance from the initial position, the number of taps and timing can be configured. Immediately after the gesture activates, it will end.

###### TapGesture Properties
> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name | Description                                                                                  | Type                             | Required | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- | -------- |
| minPointers(value) | Minimum number of pointers (fingers) required to be placed before the gesture activates. Should be a positive integer. The default value is 1. | function(value: number) | no | All | yes |
| maxDuration(value) | Maximum time, expressed in milliseconds, that defines how fast a finger must be released after a touch. The default value is 500. | function(value: number) | no | All | yes |
| maxDelay(value) | Maximum time, expressed in milliseconds, that can pass before the next tap, if many taps are required. The default value is 500. | function(value: number) | no | All | yes |
| numberOfTaps(value) | Number of tap gestures required to activate the gesture. The default value is 1. | function(value: number) | no | All | yes |
| maxDeltaX(value) | Maximum distance, expressed in points, that defines how far the finger is allowed to travel along the X axis during a tap gesture. If the finger travels further than the defined distance along the X axis and the gesture hasn't yet activated, it will fail to recognize the gesture. | function(value: number) | no | All | yes |
| maxDeltaY(value) | Maximum distance, expressed in points, that defines how far the finger is allowed to travel along the Y axis during a tap gesture. If the finger travels further than the defined distance along the Y axis and the gesture hasn't yet activated, it will fail to recognize the gesture. | function(value: number) | no | All | yes |
| maxDistance(value) | Maximum distance, expressed in points, that defines how far the finger is allowed to travel during a tap gesture. If the finger travels further than the defined distance and the gesture hasn't yet activated, it will fail to recognize the gesture. | function(value: number) | no | All | yes |
| mouseButton(value) | Allows users to choose which mouse button the handler should respond to. Arguments can be combined using the `\|` operator with values of the MouseButton enum (LEFT, RIGHT, MIDDLE, BUTTON_4, BUTTON_5, ALL). Default value is set to LEFT. | function(value: MouseButton) | no | Web,Android | no |

###### TapGesture event data

Please refer to the [common event data](#common-event-data). The following is the event data specific to the TapGesture.
> [!TIP] The **Platform** column indicates the platform where the fields are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this field; **no** means the opposite; **partially** means some capabilities of this field are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name | Related API | Type | Default | Required | Platform | HarmonyOS Support | Description |
| ---- | -------- | -------- | ------ | ---- | ---- | ----------------- | ---- |
| x | onBegin, onStart, onEnd, onFinalize | number | - | yes | All | yes | X coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the view attached to the GestureDetector. |
| y | onBegin, onStart, onEnd, onFinalize | number | - | yes | All | yes | Y coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the view attached to the GestureDetector. |
| absoluteX | onBegin, onStart, onEnd, onFinalize | number | - | yes | All | yes | X coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the window. It is recommended to use absoluteX instead of x in cases when the view attached to the GestureDetector can be transformed as an effect of the gesture. |
| absoluteY | onBegin, onStart, onEnd, onFinalize | number | - | yes | All | yes | Y coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the window. It is recommended to use absoluteY instead of y in cases when the view attached to the GestureDetector can be transformed as an effect of the gesture. |

##### PanGesture

[PanGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/pan-gesture) is a continuous gesture that can recognize a panning (dragging) gesture and track its movement. It activates when a finger is placed on the screen and moved some initial distance. A minimum initial distance, specific vertical or horizontal pan detection and the number of fingers required for activation may be configured.

###### PanGesture Properties
> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name | Description                                                                                  | Type                             | Required | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- | -------- |
| minDistance(value) | Minimum distance the finger (or multiple fingers) need to travel before the gesture activates. Expressed in points. | function(value: number) | no | All | yes |
| minVelocity(value) | Minimum speed the pointer has to reach in order for the gesture to activate. Expressed in points per second. | function(value: number) | no | All | yes |
| minVelocityX(value) | Minimum speed along the X axis the pointer has to reach in order for the gesture to activate. Expressed in points per second. | function(value: number) | no | All | yes |
| minVelocityY(value) | Minimum speed along the Y axis the pointer has to reach in order for the gesture to activate. Expressed in points per second. | function(value: number) | no | All | yes |
| minPointers(value) | A number of fingers that is required to be placed before the gesture can activate. Should be an integer greater than or equal to 0. | function(value: number) | no | All | yes |
| maxPointers(value) | When the given number of fingers is placed on the screen and the gesture hasn't yet activated, it will fail recognizing the gesture. Should be an integer greater than or equal to 0. | function(value: number) | no | All | yes |
| activateAfterLongPress(duration) | Duration in milliseconds of the LongPress gesture before Pan is allowed to activate. If the finger is moved during that period, the gesture will fail. Default value is 0, meaning no LongPress is required to activate the Pan. | function(duration: number) | no | All | yes |
| activeOffsetX(value) | Range along the X axis (in points) where fingers travel without activation of the gesture. Moving outside of this range implies activation. Range can be given as an array (the first value must be lower than or equal to 0 and the second one higher than or equal to 0) or a single number (if p is higher than or equal to 0, the range (-inf, p) will be used, otherwise (-p, inf)). | function(value: number \| number[]) | no | All | yes |
| activeOffsetY(value) | Range along the Y axis (in points) where fingers travel without activation of the gesture. Moving outside of this range implies activation. The range rules are the same as for activeOffsetX. | function(value: number \| number[]) | no | All | yes |
| failOffsetX(value) | When the finger moves outside this range (in points) along the X axis and the gesture hasn't yet activated, it will fail recognizing the gesture. The range rules are the same as for activeOffsetX. | function(value: number \| number[]) | no | All | yes |
| failOffsetY(value) | When the finger moves outside this range (in points) along the Y axis and the gesture hasn't yet activated, it will fail recognizing the gesture. The range rules are the same as for activeOffsetX. | function(value: number \| number[]) | no | All | yes |
| averageTouches(value) | Android, by default, will calculate translation values based on the position of the leading pointer (the first one that was placed on the screen). This modifier allows that behavior to be changed to the one that is default on iOS - the averaged position of all active pointers will be used to calculate the translation values. | function(value: boolean) | no | Android | no |
| enableTrackpadTwoFingerGesture(value) | Enables two-finger gestures on supported devices, for example iPads with trackpads. If not enabled the gesture will require click + drag. | function(value: boolean) | no | IOS | no |
| mouseButton(value) | Allows users to choose which mouse button the handler should respond to. Arguments can be combined using the `\|` operator. Default value is set to LEFT. | function(value: MouseButton) | no | Web,Android | no |

###### PanGesture event data

Please refer to the [common event data](#common-event-data). The following is the event data specific to the PanGesture.
> [!TIP] The **Platform** column indicates the platform where the fields are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this field; **no** means the opposite; **partially** means some capabilities of this field are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name | Related API | Type | Default | Required | Platform | HarmonyOS Support | Description |
| ---- | -------- | -------- | ------ | ---- | ---- | ----------------- | ---- |
| translationX | onBegin, onStart, onUpdate, onEnd, onFinalize | number | - | yes | All | yes | Translation of the pan gesture along the X axis accumulated over the time of the gesture. The value is expressed in point units. |
| translationY | onBegin, onStart, onUpdate, onEnd, onFinalize | number | - | yes | All | yes | Translation of the pan gesture along the Y axis accumulated over the time of the gesture. The value is expressed in point units. |
| velocityX | onBegin, onStart, onUpdate, onEnd, onFinalize | number | - | yes | All | yes | Velocity of the pan gesture along the X axis in the current moment. The value is expressed in point units per second. |
| velocityY | onBegin, onStart, onUpdate, onEnd, onFinalize | number | - | yes | All | yes | Velocity of the pan gesture along the Y axis in the current moment. The value is expressed in point units per second. |
| x | onBegin, onStart, onUpdate, onEnd, onFinalize | number | - | yes | All | yes | X coordinate of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the view attached to the GestureDetector. Expressed in point units. |
| y | onBegin, onStart, onUpdate, onEnd, onFinalize | number | - | yes | All | yes | Y coordinate of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the view attached to the GestureDetector. Expressed in point units. |
| absoluteX | onBegin, onStart, onUpdate, onEnd, onFinalize | number | - | yes | All | yes | X coordinate of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the window. The value is expressed in point units. It is recommended to use it instead of x in cases when the original view can be transformed as an effect of the gesture. |
| absoluteY | onBegin, onStart, onUpdate, onEnd, onFinalize | number | - | yes | All | yes | Y coordinate of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the window. The value is expressed in point units. It is recommended to use it instead of y in cases when the original view can be transformed as an effect of the gesture. |
| stylusData | onUpdate, onEnd | Object | - | no | All | no | Object that contains additional information about the stylus: tiltX (angle in degrees between the Y-Z plane of the stylus and the screen), tiltY (angle between the X-Z plane and the screen), altitudeAngle (angle between stylus axis and the X-Y plane of the screen), azimuthAngle (angle between the Y-Z plane and the plane containing both the stylus axis and the Y axis), pressure (normalized pressure of the stylus). Provided only for stylus input. |

##### RotationGesture

[RotationGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/rotation-gesture) is a continuous gesture that can recognize a rotation gesture and track its movement. It activates when fingers are placed on the screen and change position in a proper way.

###### RotationGesture Properties

RotationGesture does not provide any specific properties. For the available configuration see [Common Methods](#common-methods).

###### RotationGesture event data

Please refer to the [common event data](#common-event-data). The following is the event data specific to the RotationGesture.
> [!TIP] The **Platform** column indicates the platform where the fields are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this field; **no** means the opposite; **partially** means some capabilities of this field are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name | Related API | Type | Default | Required | Platform | HarmonyOS Support | Description |
| ---- | -------- | -------- | ------ | ---- | ---- | ----------------- | ---- |
| rotation | onBegin, onStart, onUpdate, onEnd, onFinalize | number | - | yes | All | yes | Amount rotated, expressed in radians, from the gesture's focal point (anchor). |
| velocity | onBegin, onStart, onUpdate, onEnd, onFinalize | number | - | yes | All | yes | Instantaneous velocity, expressed in point units per second, of the gesture. |
| anchorX | onBegin, onStart, onUpdate, onEnd, onFinalize | number | - | yes | All | yes | X coordinate, expressed in points, of the gesture's central focal point (anchor). |
| anchorY | onBegin, onStart, onUpdate, onEnd, onFinalize | number | - | yes | All | yes | Y coordinate, expressed in points, of the gesture's central focal point (anchor). |

##### LongPressGesture

[LongPressGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/long-press-gesture) is a discrete gesture that activates when the corresponding view is pressed for a sufficiently long time. The state will turn into END immediately after the finger is released.

###### LongPressGesture Properties
> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name | Description                                                                                  | Type                             | Required | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- | -------- |
| minDuration(value) | Minimum time, expressed in milliseconds, that a finger must remain pressed on the corresponding view. The default value is 500. | function(value: number) | no | All | yes |
| maxDistance(value) | Maximum distance, expressed in points, that defines how far the finger is allowed to travel during a long press gesture. If the finger travels further than the defined distance and the gesture hasn't yet activated, it will fail to recognize the gesture. The default value is 10. | function(value: number) | no | All | yes |
| mouseButton(value) | Allows users to choose which mouse button the handler should respond to. Arguments can be combined using the `\|` operator. Default value is set to LEFT. | function(value: MouseButton) | no | Web,Android | no |

###### LongPressGesture event data

Please refer to the [common event data](#common-event-data). The following is the event data specific to the LongPressGesture.
> [!TIP] The **Platform** column indicates the platform where the fields are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this field; **no** means the opposite; **partially** means some capabilities of this field are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name | Related API | Type | Default | Required | Platform | HarmonyOS Support | Description |
| ---- | -------- | -------- | ------ | ---- | ---- | ----------------- | ---- |
| x | onBegin, onStart, onEnd, onFinalize | number | - | yes | All | yes | X coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the view attached to the GestureDetector. |
| y | onBegin, onStart, onEnd, onFinalize | number | - | yes | All | yes | Y coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the view attached to the GestureDetector. |
| absoluteX | onBegin, onStart, onEnd, onFinalize | number | - | yes | All | yes | X coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the window. It is recommended to use absoluteX instead of x in cases when the view attached to the GestureDetector can be transformed as an effect of the gesture. |
| absoluteY | onBegin, onStart, onEnd, onFinalize | number | - | yes | All | yes | Y coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the window. It is recommended to use absoluteY instead of y in cases when the view attached to the GestureDetector can be transformed as an effect of the gesture. |
| duration | onBegin, onStart, onEnd, onFinalize | number | - | yes | All | yes | Duration of the long press (time since the start of the gesture), expressed in milliseconds. |

##### ManualGesture

[ManualGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/manual-gesture) does not recognize any gesture by itself. It is used to control the gesture state manually via the GestureStateManager, typically together with the onTouchesDown and other touch callbacks.

###### ManualGesture Properties

ManualGesture does not provide any specific properties. For the available configuration see [Common Methods](#common-methods).

###### ManualGesture event data

ManualGesture does not provide any event data specific to it. Please refer to the [common event data](#common-event-data).

##### PinchGesture

[PinchGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/pinch-gesture) is a continuous gesture that recognizes a pinch gesture. It allows for tracking the distance between two fingers and using that information to scale or zoom content.

###### PinchGesture Properties

PinchGesture does not provide any specific properties. For the available configuration see [Common Methods](#common-methods).

###### PinchGesture event data

Please refer to the [common event data](#common-event-data). The following is the event data specific to the PinchGesture.
> [!TIP] The **Platform** column indicates the platform where the fields are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this field; **no** means the opposite; **partially** means some capabilities of this field are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name | Related API | Type | Default | Required | Platform | HarmonyOS Support | Description |
| ---- | -------- | -------- | ------ | ---- | ---- | ----------------- | ---- |
| scale | onBegin, onStart, onUpdate, onEnd, onFinalize | number | - | yes | All | yes | The scale factor relative to the points of the two touches in screen coordinates. At the beginning of the gesture the scale factor is 1.0; it increases proportionally as the distance between the fingers increases and decreases as the distance decreases. |
| velocity | onBegin, onStart, onUpdate, onEnd, onFinalize | number | - | yes | All | yes | Velocity of the pinch gesture at the current moment. The value is expressed in scale factor per second. |
| focalX | onBegin, onStart, onUpdate, onEnd, onFinalize | number | - | yes | All | yes | Position expressed in points along the X axis of the center anchor point of the gesture. |
| focalY | onBegin, onStart, onUpdate, onEnd, onFinalize | number | - | yes | All | yes | Position expressed in points along the Y axis of the center anchor point of the gesture. |

##### FlingGesture

[FlingGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/fling-gesture) is a discrete gesture that activates when the movement is sufficiently long and fast. When the gesture gets activated it will turn into the END state when the finger is released.

###### FlingGesture Properties
> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name | Description                                                                                  | Type                             | Required | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- | -------- |
| direction(value) | Expresses the allowed direction of movement. Expected values are exported as constants in the Directions object. It's possible to pass one or many directions in one parameter (e.g. `Directions.RIGHT \| Directions.LEFT`). | function(value: Directions) | no | All | yes |
| numberOfPointers(value) | Determines the exact number of pointers required to handle the fling gesture. | function(value: number) | no | All | yes |
| mouseButton(value) | Allows users to choose which mouse button the handler should respond to. Arguments can be combined using the `\|` operator. Default value is set to LEFT. | function(value: MouseButton) | no | Web,Android | no |

###### FlingGesture event data

Please refer to the [common event data](#common-event-data). The following is the event data specific to the FlingGesture.
> [!TIP] The **Platform** column indicates the platform where the fields are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this field; **no** means the opposite; **partially** means some capabilities of this field are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name | Related API | Type | Default | Required | Platform | HarmonyOS Support | Description |
| ---- | -------- | -------- | ------ | ---- | ---- | ----------------- | ---- |
| x | onBegin, onStart, onEnd, onFinalize | number | - | yes | All | yes | X coordinate of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the view attached to the GestureDetector. Expressed in point units. |
| y | onBegin, onStart, onEnd, onFinalize | number | - | yes | All | yes | Y coordinate of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the view attached to the GestureDetector. Expressed in point units. |
| absoluteX | onBegin, onStart, onEnd, onFinalize | number | - | yes | All | yes | X coordinate of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the window. The value is expressed in point units. It is recommended to use it instead of x in cases when the original view can be transformed as an effect of the gesture. |
| absoluteY | onBegin, onStart, onEnd, onFinalize | number | - | yes | All | yes | Y coordinate of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the window. The value is expressed in point units. It is recommended to use it instead of y in cases when the original view can be transformed as an effect of the gesture. |

##### NativeGesture

[NativeGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/native-gesture) is used to wrap gestures of native views (e.g. ScrollView) so that they can participate in the Gesture Handler's gesture orchestration. It is usually used together with `createNativeWrapper` or the wrapped components.

###### NativeGesture Properties
> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name | Description                                                                                  | Type                             | Required | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- | -------- |
| shouldActivateOnStart(value) | When true, underlying handler will activate unconditionally when it receives any touches in BEGAN or UNDETERMINED state. | function(value: boolean) | no | Android | yes |
| disallowInterruption(value) | When true, cancels all other gesture handlers when this gesture changes its state to ACTIVE. | function(value: boolean) | no | All | yes |

###### NativeGesture event data

Please refer to the [common event data](#common-event-data). The following is the event data specific to the NativeGesture.
> [!TIP] The **Platform** column indicates the platform where the fields are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this field; **no** means the opposite; **partially** means some capabilities of this field are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name | Related API | Type | Default | Required | Platform | HarmonyOS Support | Description |
| ---- | -------- | -------- | ------ | ---- | ---- | ----------------- | ---- |
| pointerInside | onBegin, onStart, onEnd, onFinalize | boolean | - | yes | All | yes | True if the gesture was performed inside of the containing view, false otherwise. |

##### ForceTouchGesture

[ForceTouchGesture](https://docs.swmansion.com/react-native-gesture-handler/docs/2.x/gestures/force-touch-gesture) is a continuous gesture that recognizes force touches. It activates when the pressure reaches the minimal value and fails when the pressure exceeds the maximal value.

###### ForceTouchGesture Properties
> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name | Description                                                                                  | Type                             | Required | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- | -------- |
| minForce(value) | A minimal pressure that is required before the gesture can activate. Should be a value from the range [0.0, 1.0]. Default is 0.2. | function(value: number) | no | All | yes |
| maxForce(value) | A maximal pressure that could be applied for the gesture. If the pressure is greater, the gesture fails. Should be a value from the range [0.0, 1.0]. | function(value: number) | no | All | yes |
| feedbackOnActivation(value) | Value defining if haptic feedback has to be performed on activation. | function(value: boolean) | no | All | yes |

###### ForceTouchGesture event data

Please refer to the [common event data](#common-event-data). The following is the event data specific to the ForceTouchGesture.
> [!TIP] The **Platform** column indicates the platform where the fields are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this field; **no** means the opposite; **partially** means some capabilities of this field are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name | Related API | Type | Default | Required | Platform | HarmonyOS Support | Description |
| ---- | -------- | -------- | ------ | ---- | ---- | ----------------- | ---- |
| force | onBegin, onStart, onUpdate, onEnd, onFinalize | number | - | yes | All | yes | The pressure of a touch. |

### Components

#### Touchables

The Gesture Handler library provides an implementation of a React Native touchable component based on native buttons, which does not rely on React Native's JS responder system. The implementation of these components follows the same API and is designed to replace the touchable components in React Native. HarmonyOS has fully supported it.

- [Touchable Opacity](https://reactnative.dev/docs/touchableopacity)
- [Touchable Without Feedback](https://reactnative.dev/docs/touchablewithoutfeedback)
- [Touchable Highlight](https://reactnative.dev/docs/touchablehighlight)
- [Touchable Native Feedback](https://reactnative.dev/docs/touchablenativefeedback)

#### Buttons

BaseButton Properties
| Name | Description                                                                                  | Type                             | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- |
| onActiveStateChange | Triggered when button changes from inactive to active and vice versa. | function | All   | yes   |
| onPress | Triggered when the button gets pressed. | function | All   | yes   |
| onLongPress | Triggered when the button gets pressed for at least `delayLongPress` milliseconds. | function | All   | yes   |
| exclusive | Defines whether multiple buttons can be pressed simultaneously. Default is `true`. | boolean | All   | yes   |
| delayLongPress | Defines the delay, in milliseconds, after which the `onLongPress` callback gets called. Default is 600. | number | All   | yes   |

RectButton Properties
| Name | Description                                                                                  | Type                             | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- |
| underlayColor | The background color that will be dimmed when button is in active state. | Color | All   | yes   |
| activeOpacity | opacity applied to the button when it is in an active state. | number | IOS   | no   |

RawButton Properties

| Name | Description | Type | Platform | HarmonyOS Support |
| --- | --- | --- | --- | --- |
| exclusive | Defines whether multiple buttons can be pressed simultaneously. Default is `true`. | boolean | All | yes |
| style | Additional style object. | ViewStyle | All | yes |
| onLayout | Called when the layout changes. | function | All | yes |
| rippleColor | Ripple color. | number | All | NO |
| rippleRadius | Ripple radius. | number | All | NO |
| borderless | Whether the ripple can spread beyond the component bounds. | boolean | All | NO |
| touchSoundDisabled | Whether touch sound feedback is disabled. | boolean | All | NO |
| foreground | Whether ripple is shown as foreground effect. | boolean | All | NO |

Pressable Properties

| Name | Description | Type | Platform | HarmonyOS Support |
| --- | --- | --- | --- | --- |
| children | Child component(s). | ReactNode | All | yes |
| cancelable | Whether press can be interrupted by an external scroll gesture. | boolean | All | yes |
| delayLongPress | Time threshold for triggering `onLongPress`. | number | All | yes |
| disabled | Whether press interaction is disabled. | boolean | All | yes |
| hitSlop | Press can trigger when touches are detected in the expanded hit area. | number\|object | All | yes |
| pressRetentionOffset | Offset that allows finger movement after press starts. | number\|object | All | yes |
| style | Style in pressed state. | ViewStyle | All | yes |
| simultaneousWithExternalGesture | Allows simultaneous recognition with an external gesture. | RelationPropType | All | yes |
| requireExternalGestureToFail | Waits for an external gesture to fail before handling press. | RelationPropType | All | yes |
| blocksExternalGesture | Blocks an external gesture from activating. | RelationPropType | All | yes |
| onPress | Called when a single tap gesture is detected. | function | All | yes |
| onPressIn | Called when touch starts before `onPress` fires. | function | All | yes |
| onPressOut | Called when touch ends before `onPress` fires. | function | All | yes |
| onLongPress | Called when a long-press gesture is detected. | function | All | yes |

Pressable Event Type (PressableEvent)

The `onPress` / `onPressIn` / `onPressOut` / `onLongPress` callbacks all receive an event object of type `PressableEvent`.

| Name | Description | Type | Platform | HarmonyOS Support |
| --- | --- | --- | --- | --- |
| identifier | Unique identifier of the touch point. | number | All | yes |
| locationX | X coordinate of the touch point relative to the top-left corner of the component. | number | All | yes |
| locationY | Y coordinate of the touch point relative to the top-left corner of the component. | number | All | yes |
| pageX | X coordinate of the touch point relative to the top-left corner of the page. | number | All | yes |
| pageY | Y coordinate of the touch point relative to the top-left corner of the page. | number | All | yes |
| target | ID of the target node receiving the event (always 0). | number | no | no |
| timestamp | Timestamp of the event (in milliseconds). | number | All | yes |
| touches | List of all touch points currently on the screen (always an empty array). | InnerPressableEvent[] | no | no |
| changedTouches | Touch points that changed in this event (always an empty array). | InnerPressableEvent[] | no | no |
| force | Press force (0-1), optional field (always undefined). | number | no | no |

#### Drawer Layout

Properties
| Name | Description                                                                                  | Type                             | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- |
| drawerType | Drawer display type.                                     | 'front'\|'back'\|'slide' | All | yes |
| edgeWidth | Defines how far from the edge of the content view the gesture should activate. | number | All   | yes   |
| hideStatusBar | When set to `true`, Drawer component will use [StatusBar](https://reactnative.dev/docs/statusbar.html) API to hide the OS status bar whenever the drawer is pulled or when its in an "open" state. | boolean | All   | yes   |
| statusbaranimation | Can be used when `hideStatusBar` is set to `true` and will select the animation used for hiding/showing the status bar. | 'fade'\|'slide'\|'none' | All   | no   |
| overlaycolor | Semi-transparent overlay color displayed on top of the content view when drawer gets open. | color | All   | yes   |
| rendernavigationview | Required parameter in the standard implementation, used to render the navigation view. | function | All   | yes   |
| ondrawerclose | Called when the drawer is closed. | function | All   | yes   |
| ondraweropen | Called when the drawer is opened. | function | All   | yes   |
| ondrawerslide | Called as a drawer sliding open from touch events. | function | All   | yes   |
| ondrawerstatechanged | Called when the status of the drawer changes. | function | All   | yes   |
| children | Child component rendered by default and wrapped by drawer. | component\| function | All   | yes   |

#### ReanimatedDrawerLayout

Properties
| Name | Description | Type | Platform | HarmonyOS Support |
| --- | --- | --- | --- | --- |
| renderNavigationView | Used to render drawer content. | function | All | yes |
| drawerPosition | Determines from which side the drawer opens. | DrawerPosition {<br/> LEFT,<br/> RIGHT,<br/>} | All | yes |
| drawerWidth | Width of the drawer. | number | All | yes |
| drawerBackgroundColor | Background color of the drawer. | string | All | yes |
| drawerLockMode | Specifies drawer lock mode. | enum DrawerLockMode {<br/> UNLOCKED,<br/> LOCKED_CLOSED,<br/> LOCKED_OPEN,<br/>} | All | yes |
| keyboardDismissMode | Whether keyboard should dismiss while dragging drawer. | enum DrawerKeyboardDismissMode {<br/> NONE,<br/> ON_DRAG,<br/>} | All | yes |
| onDrawerClose | Triggered when drawer closes. | function | All | yes |
| onDrawerOpen | Triggered when drawer opens. | function | All | yes |
| onDrawerStateChanged | Triggered when drawer state changes. | function | All | yes |
| drawerType | Animation type when opening drawer. | enum DrawerType {<br/> FRONT,<br/> BACK,<br/> SLIDE,<br/>} | All | yes |
| animationSpeed | Animation speed when releasing drag or closing drawer. | number | All | yes |
| edgeWidth | Distance from content edge where gesture can activate. | number | All | yes |
| minSwipeDistance | Minimum swipe distance before drawer starts dragging. | number | All | yes |
| hideStatusBar | Whether to hide status bar when drawer is open. | boolean | All | yes |
| statusBarAnimation | Animation used when hiding status bar if `hideStatusBar` is `true`. | 'none' \| 'fade' \| 'slide' | All | no |
| overlayColor | Overlay color on main content while sliding. | string | All | yes |
| contentContainerStyle | Style of main content container. | ViewStyle | All | yes |
| drawerContainerStyle | Style of drawer container. | ViewStyle | All | yes |
| enableTrackpadTwoFingerGesture | Enables two-finger gesture on supported trackpad devices (for example iPad). | boolean | IOS | no |
| onDrawerSlide | Triggered continuously while manually sliding drawer. | function | All | yes |
| children | Child component(s). | ReactNode | All | yes |
| userSelect | Whether text selection is enabled in drawer and context window. | 'none' \| 'auto' \| 'text' | Web | no |
| activeCursor | Cursor shown while drawer is being dragged. | string | Web | no |
| mouseButton | Mouse button used to trigger drawer swipe. | number | Web | no |
| enableContextMenu | Whether right-click context menu is enabled. | boolean | Web | no |
| openDrawer | Opens the drawer. | interface | All | yes |
| closeDrawer | Closes the drawer. | interface | All | yes |

#### Swipeable

Properties
| Name | Description                                                                                  | Type                             | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | ------------------------------------ | -------- | -------- |
| friction | Specifies how much the visual interaction will be delayed compared to the gesture distance. | number | All | yes |
| leftThreshold | Distance from the left edge at which released panel will animate to the open state. | number | All | yes |
| rightThreshold | Distance from the right edge at which released panel will animate to the open state. | number | All | yes |
| dragOffsetFromLeftEdge | Distance that the panel must be dragged from the left edge to be considered a swipe. | number                   | All | yes |
| dragOffsetFromRightEdge | Distance that the panel must be dragged from the right edge to be considered a swipe. | number | All | yes |
| overshootLeft | Whether the swipeable panel can be pulled further than the left actions panel's width. | boolean | All | yes |
| overshootRight | Whether the swipeable panel can be pulled further than the right actions panel's width. | boolean | All | yes |
| overshootFriction | Specifies how much the visual interaction will be delayed compared to the gesture distance at overshoot. | number                   | All | yes |
| onSwipeableOpen | Called when action panel gets open. | function(direction: 'left' \| 'right', swipeable: Swipeable) | All | yes |
| onSwipeableClose | Called when action panel is closed. | function(direction: 'left' \| 'right', swipeable: Swipeable) | All | yes |
| onSwipeableWillOpen | Called when action panel starts animating on open (either right or left). | function(direction: 'left' \| 'right') | All | yes |
| onSwipeableWillClose | Called when action panel starts animating on close. | function(direction: 'left' \| 'right') | All | yes |
| renderLeftActions | Method that returns the left action panel renderer. | function(progressAnimatedValue: AnimatedInterpolation,<br/>    dragAnimatedValue: AnimatedInterpolation,<br/>    swipeable: Swipeable) | All | yes |
| renderRightActions | Method that returns the right action panel renderer. | function(progressAnimatedValue: AnimatedInterpolation,<br/>    dragAnimatedValue: AnimatedInterpolation,<br/>    swipeable: Swipeable) | All | yes |
| containerStyle | style object for the container (Animated.View), for example to override `overflow: 'hidden'`. | StyleProp<ViewStyle> | All | yes |
| childrenContainerStyle | style object for the children container (Animated.View), for example to apply `flex: 1`. | StyleProp<ViewStyle> | All | yes |

#### ReanimatedSwipeable

Properties
| Name | Description | Type | Platform | HarmonyOS Support |
| --- | --- | --- | --- | --- |
| children | Child component(s). | ReactNode | All | yes |
| enabled | Whether the action panel is swipeable. | boolean | All | yes |
| hitSlop | Allows drag gesture activation in the extended hit area. | number\|object | All | yes |
| enableTrackpadTwoFingerGesture | Enables two-finger gesture on supported trackpad devices (for example iPad). | boolean | IOS | no |
| friction | Specifies lag between gesture distance and visual interaction. | number | All | yes |
| leftThreshold | Minimum right-swipe distance to open panel. | number | All | yes |
| rightThreshold | Minimum left-swipe distance to open panel. | number | All | yes |
| dragOffsetFromLeftEdge | Drag distance from left edge. | number | All | yes |
| dragOffsetFromRightEdge | Drag distance from right edge. | number | All | yes |
| overshootLeft | Whether panel can overshoot left action width and bounce back on release. | boolean | All | yes |
| overshootRight | Whether panel can overshoot right action width and bounce back on release. | boolean | All | yes |
| overshootFriction | Resistance when swiping beyond action panel. Higher values mean stronger resistance. | number | All | yes |
| onSwipeableOpen | Triggered when action panel is open. | function | All | yes |
| onSwipeableClose | Triggered when action panel is closed. | function | All | yes |
| onSwipeableWillOpen | Triggered when open animation starts. | function | All | yes |
| onSwipeableWillClose | Triggered when close animation starts. | function | All | yes |
| onSwipeableOpenStartDrag | Triggered when opening drag starts. | function | All | yes |
| onSwipeableCloseStartDrag | Triggered when closing drag starts. | function | All | yes |
| renderLeftActions | Left action view renderer. | function | All | yes |
| renderRightActions | Right action view renderer. | function | All | yes |
| animationOptions | Animation configuration object. | Record | All | yes |
| containerStyle | Style for swipeable container. | ViewStyle | All | yes |
| childrenContainerStyle | Style object for children container. | ViewStyle | All | yes |
| simultaneousWithExternalGesture | Allows simultaneous recognition with an external gesture. | RelationPropType | All | yes |
| requireExternalGestureToFail | Waits for an external gesture to fail before handling. | RelationPropType | All | yes |
| blocksExternalGesture | Blocks an external gesture from activating. | RelationPropType | All | yes |
| close | Closes the swipeable. | interface | All | yes |
| openLeft | Opens swipeable from left side. | interface | All | yes |
| openRight | Opens swipeable from right side. | interface | All | yes |
| reset | Resets swipeable state to closed. | interface | All | yes |

#### Wrapped Components

The following components wrap React Native's native components via `createNativeWrapper` (i.e. they wrap the native component in a NativeViewGestureHandler), registering them in the Gesture Handler's gesture orchestration system so that other gestures can refer to them through relations such as `waitFor` or `simultaneousWithExternalGesture`. Their props are the same as the wrapped RN components (plus the [NativeViewGestureHandler properties](#nativeviewgesturehandler-properties)), so they are not listed again here.

> [!TIP] The **Platform** column indicates the platform where the components are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this component; **no** means the opposite. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name | Description                                                                                  | Platform | HarmonyOS Support |
| ---------- | ------------------------------------------------------------------------------------------------ | -------- | -------- |
| ScrollView | Wraps RN ScrollView with default gesture config `disallowInterruption: true` and `shouldCancelWhenOutside: false`; when `refreshControl` is provided, its gesture is automatically added to `waitFor` to avoid conflicts between pull-to-refresh and other gestures. | All | yes |
| FlatList | Wraps RN FlatList, using the RNGH ScrollView above as its scroll component. Props are the same as RN FlatList. | All | yes |
| RefreshControl | Wraps RN RefreshControl with default gesture config `disallowInterruption: true`, so that pull-to-refresh is not interrupted by other gestures. Usually there is no need to use it directly — it is handled automatically together with the RNGH ScrollView. | All | yes |
| Switch | Wraps RN Switch with default gesture config `shouldActivateOnStart: true` and `disallowInterruption: true`. | All | yes |
| TextInput | Wraps RN TextInput with no extra default gesture config, only registering it in the gesture system. | All | yes |
| DrawerLayoutAndroid | Wraps RN DrawerLayoutAndroid. This RN component is available on Android only and is not available on HarmonyOS; use [Drawer Layout](#drawer-layout) or [ReanimatedDrawerLayout](#reanimateddrawerlayout) instead. | Android | no |
| Text | Wraps RN Text. When `onPress`/`onLongPress` is set, presses are handled through the Gesture Handler system (`Gesture.Native().runOnJS(true)`) instead of RN's responder system. | All | yes |

### Gesture handlers(legacy)

> [!WARNING] Consider using the new [gestures API](https://docs.swmansion.com/react-native-gesture-handler/docs/gestures/gesture) instead. The old API is not actively supported and is not receiving the new features. Check out [RNGH 2.0 section in Introduction](https://docs.swmansion.com/react-native-gesture-handler/docs/#rngh-20) for more information.

#### Gesture handlers General Properties
> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| NAME                | Description                                                                                                                                   | TYPE                     | Required | Platform | HarmonyOS Support |
| ----------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------- | -------- | -------- | -------- |
| enabled                 | Indicates whether the given handler should be analyzing stream of touch events or not.                                                            | boolean                      | no       | All      | yes      |
| hitSlop                 | This parameter enables control over what part of the connected view area can be used to begin recognizing the gesture.                            | object                       | no       | All      | yes      |
| onGestureEvent          | Takes a callback that is going to be triggered for each subsequent touch event while the handler is in an ACTIVE state.                           | callback                     | no       | All      | yes      |
| onHandlerStateChange    | Takes a callback that is going to be triggered when state of the given handler changes.                                                           | callback                     | no       | All      | yes      |

#### Gesture handlers common event data

The following is the common event data for the `onGestureEvent` and `onHandlerStateChange` callbacks.
> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| NAME         | Description                                                             | TYPE | Platform | HarmonyOS Support |
| ---------------- | --------------------------------------------------------------------------- | ------ | -------- | -------- |
| state            | Current state of the handler.                                               | State  | All      | yes      |
| numberOfPointers | Represents the number of pointers (fingers) currently placed on the screen. | number | All      | yes      |

#### PanGestureHandler

##### PanGestureHandler Properties
> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| NAME                       | Description                                                                                                                                                                             | TYPE | Required | Platform | HarmonyOS Support |
| ------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------- | -------- | -------- | -------- |
| minDist                        | Minimum distance the finger (or multiple finger) need to travel before the handler activates.                                                                                               | number  | no       | All      | yes      |
| minPointers                    | A number of fingers that is required to be placed before handler can activate.                                                                                                              | number  | no       | All      | yes      |
| maxPointers                    | When the given number of fingers is placed on the screen and handler hasn't yet activated it will fail recognizing the gesture.                                                             | number  | no       | All      | yes      |
| activeOffsetX                  | Range along X axis (in points) where fingers travels without activation of handler.                                                                                                         | number  | no       | All      | yes      |
| activeOffsetY                  | Range along Y axis (in points) where fingers travels without activation of handler.                                                                                                         | number  | no       | All      | yes      |

##### PanGestureHandler event data

Please refer to the [common event data for the basic handler class](#gesture-handlers-common-event-data). The following is the event data specific to the  PanGestureHandler.
> [!TIP] The **Platform** column indicates the platform where the fields are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this field; **no** means the opposite; **partially** means some capabilities of this field are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name         | Related API                       | Type   | Default | Required | Platform | HarmonyOS Support | Description                                                                                                                              |
| ------------ | --------------------------------- | ------ | ------- | -------- | -------- | ----------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| translationX | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Translation of the pan gesture along X axis accumulated over the time of the gesture. The value is expressed in the point units.         |
| translationY | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Translation of the pan gesture along Y axis accumulated over the time of the gesture. The value is expressed in the point units.         |
| velocityX    | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Velocity of the pan gesture along the X axis in the current moment. The value is expressed in point units per second.                    |
| velocityY    | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Velocity of the pan gesture along the Y axis in the current moment. The value is expressed in point units per second.                    |
| x            | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | X coordinate of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the view attached to the handler. Expressed in point units. |
| y            | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Y coordinate of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the view attached to the handler. Expressed in point units. |

#### TapGestureHandler

##### TapGestureHandler Properties
> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| NAME      | Description                                                                                                                    | TYPE | Required | Platform | HarmonyOS Support |
| ------------- | ---------------------------------------------------------------------------------------------------------------------------------- | ------ | -------- | -------- | -------- |
| minPointers   | Minimum number of pointers (fingers) required to be placed before the handler activates.                                           | number | no       | All      | yes      |
| maxDurationMs | Maximum time, expressed in milliseconds, that defines how fast a finger must be released after a touch.                            | number | no       | All      | yes      |
| maxDelayMs    | Maximum time, expressed in milliseconds, that can pass before the next tap — if many taps are required.                            | number | no       | All      | yes      |
| maxDeltaX     | Maximum distance, expressed in points, that defines how far the finger is allowed to travel along the X axis during a tap gesture. | number | no       | All      | yes      |
| maxDeltaY     | Maximum distance, expressed in points, that defines how far the finger is allowed to travel along the Y axis during a tap gesture. | number | no       | All      | yes      |
| maxDist       | Maximum distance, expressed in points, that defines how far the finger is allowed to travel during a tap gesture.                  | number | no       | All      | yes      |

##### TapGestureHandler event data

Please refer to the [common event data for the basic handler class](#gesture-handlers-common-event-data). The following is the event data specific to the TapGestureHandler.
> [!TIP] The **Platform** column indicates the platform where the fields are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this field; **no** means the opposite; **partially** means some capabilities of this field are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name      | Related API                       | Type   | Default | Required | Platform | HarmonyOS Support | Description                                                                                                                                                                |
| --------- | ---------------------------------- | ------ | ------- | -------- | -------- | ----------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| x         | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | X coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the view attached to the handler.                                                |
| y         | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Y coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the view attached to the handler.                                                |
| absoluteX | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | X coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the window. It is recommended to use absoluteX instead of x in cases when the view attached to the handler can be transformed as an effect of the gesture. |
| absoluteY | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Y coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the window. It is recommended to use absoluteY instead of y in cases when the view attached to the handler can be transformed as an effect of the gesture. |

#### ForceTouchGestureHandler

##### ForceTouchGestureHandler Properties
> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| NAME      | Description                                                                                                                    | TYPE | Required | Platform | HarmonyOS Support |
| ------------- | ---------------------------------------------------------------------------------------------------------------------------------- | ------ | -------- | -------- | -------- |
| minForce | Minimum pressure                                       | number | no       | All      | yes      |
| maxForce | Maximum pressure                        | number | no       | All      | yes      |
| feedbackOnActivation | Is tactile feedback (vibration) required when the gesture is activated           | boolean | no       | All      | yes      |

##### ForceTouchGestureHandler event data

The following is event data specific to ForceTouchGestureHandler.
> [!TIP] The **Platform** column indicates the platform where the fields are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this field; **no** means the opposite; **partially** means some capabilities of this field are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name  | Related API                       | Type   | Default | Required | Platform | HarmonyOS Support | Description                  |
| ----- | ---------------------------------- | ------ | ------- | -------- | -------- | ----------------- | ---------------------------- |
| force | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Pressure produced by the touch. |

#### LongPressGestureHandler

##### LongPressGestureHandler Properties
> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| NAME          | Description                                                                                                                       | TYPE   | Required | Platform | HarmonyOS Support |
| ------------- | --------------------------------------------------------------------------------------------------------------------------------- | ------ | -------- | -------- | -------- |
| minDurationMs | Minimum time, expressed in milliseconds, that a finger must remain pressed on the corresponding view. The default value is 500.   | number | no       | All      | yes      |
| maxDist       | Maximum distance, expressed in points, that defines how far the finger is allowed to travel during a long press gesture. If the finger travels further than the defined distance and the handler hasn't yet activated, it will fail to recognize the gesture. The default value is 10. | number | no       | All      | yes      |

##### LongPressGestureHandler event data

Please refer to the [common event data for the basic handler class](#gesture-handlers-common-event-data). The following is the event data specific to the LongPressGestureHandler.
> [!TIP] The **Platform** column indicates the platform where the fields are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this field; **no** means the opposite; **partially** means some capabilities of this field are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name      | Related API                       | Type   | Default | Required | Platform | HarmonyOS Support | Description                                                                                                                                                                |
| --------- | ---------------------------------- | ------ | ------- | -------- | -------- | ----------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| x         | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | X coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the view attached to the handler.                                                |
| y         | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Y coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the view attached to the handler.                                                |
| absoluteX | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | X coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the window. It is recommended to use absoluteX instead of x in cases when the view attached to the handler can be transformed as an effect of the gesture. |
| absoluteY | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Y coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the window. It is recommended to use absoluteY instead of y in cases when the view attached to the handler can be transformed as an effect of the gesture. |
| duration  | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Duration of the long press (time since the start of the event), expressed in milliseconds.                                                                                 |

#### PinchGestureHandler

##### PinchGestureHandler Properties

PinchGestureHandler does not extend the common set of properties from the base handler class with any specific properties.

##### PinchGestureHandler event data

Please refer to the [common event data for the basic handler class](#gesture-handlers-common-event-data). The following is the event data specific to the PinchGestureHandler.
> [!TIP] The **Platform** column indicates the platform where the fields are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this field; **no** means the opposite; **partially** means some capabilities of this field are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name     | Related API                       | Type   | Default | Required | Platform | HarmonyOS Support | Description                                                                                                                     |
| -------- | ---------------------------------- | ------ | ------- | -------- | -------- | ----------------- | ------------------------------------------------------------------------------------------------------------------------------- |
| scale    | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | The scale factor relative to the points of the two touches in screen coordinates.                                                |
| velocity | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Velocity of the pinch gesture at the current moment. The value is expressed in scale factor per second.                          |
| focalX   | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Position expressed in points along X axis of center anchor point of the gesture.                                                |
| focalY   | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Position expressed in points along Y axis of center anchor point of the gesture.                                                |

#### RotationGestureHandler

##### RotationGestureHandler Properties

RotationGestureHandler does not extend the common set of properties from the base handler class with any specific properties.

##### RotationGestureHandler event data

Please refer to the [common event data for the basic handler class](#gesture-handlers-common-event-data). The following is the event data specific to the RotationGestureHandler.
> [!TIP] The **Platform** column indicates the platform where the fields are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this field; **no** means the opposite; **partially** means some capabilities of this field are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name     | Related API                       | Type   | Default | Required | Platform | HarmonyOS Support | Description                                                                                          |
| -------- | ---------------------------------- | ------ | ------- | -------- | -------- | ----------------- | ---------------------------------------------------------------------------------------------------- |
| rotation | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Amount rotated, expressed in radians, from the gesture's focal point (anchor).                       |
| velocity | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Instantaneous velocity, expressed in point units per second, of the gesture.                         |
| anchorX  | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | X coordinate, expressed in points, of the gesture's central focal point (anchor).                    |
| anchorY  | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Y coordinate, expressed in points, of the gesture's central focal point (anchor).                    |

#### FlingGestureHandler

##### FlingGestureHandler Properties
> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| NAME             | Description                                                                                                                 | TYPE       | Required | Platform | HarmonyOS Support |
| ---------------- | --------------------------------------------------------------------------------------------------------------------------- | ---------- | -------- | -------- | -------- |
| direction        | Expresses the allowed direction of movement. It's possible to pass one or many directions in one parameter (e.g. `Directions.RIGHT \| Directions.LEFT`). | Directions | no       | All      | yes      |
| numberOfPointers | Determines the exact number of pointers required to handle the fling gesture.                                               | number     | no       | All      | yes      |

##### FlingGestureHandler event data

Please refer to the [common event data for the basic handler class](#gesture-handlers-common-event-data). The following is the event data specific to the FlingGestureHandler.
> [!TIP] The **Platform** column indicates the platform where the fields are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this field; **no** means the opposite; **partially** means some capabilities of this field are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| Name      | Related API                       | Type   | Default | Required | Platform | HarmonyOS Support | Description                                                                                                                                                                |
| --------- | ---------------------------------- | ------ | ------- | -------- | -------- | ----------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| x         | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | X coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the view attached to the handler.                                                |
| y         | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Y coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the view attached to the handler.                                                |
| absoluteX | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | X coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the window. It is recommended to use absoluteX instead of x in cases when the view attached to the handler can be transformed as an effect of the gesture. |
| absoluteY | onGestureEvent, onHandlerStateChange | number | -       | yes      | All      | yes               | Y coordinate, expressed in points, of the current position of the pointer (finger or a leading pointer when there are multiple fingers placed) relative to the window. It is recommended to use absoluteY instead of y in cases when the view attached to the handler can be transformed as an effect of the gesture. |

#### NativeViewGestureHandler

##### NativeViewGestureHandler Properties
> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

| NAME                  | Description                                                                                                                                             | TYPE    | Required | Platform | HarmonyOS Support |
| --------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------- | ------- | -------- | -------- | -------- |
| shouldActivateOnStart | When true, underlying handler will activate unconditionally when in BEGAN or UNDETERMINED state.                                                        | boolean | no       | Android  | yes      |
| disallowInterruption  | When true, cancels all other gesture handlers when this NativeViewGestureHandler receives an ACTIVE state event.                                        | boolean | no       | All      | yes      |

##### NativeViewGestureHandler event data

NativeViewGestureHandler does not provide event data specific to it. Please refer to the [common event data for the basic handler class](#gesture-handlers-common-event-data).

## Known Issues

- [ ] statusbaranimation property is not supported due to the fact that Arkts bar only supports fade-in and fade-out animations, and the RN framework status bar animation is not supported. Issue: [issue#55](https://github.com/react-native-oh-library/react-native-harmony-gesture-handler/issues/55)
- [ ] Legacy issue from 0.82 upstream community upgrade. Issue: [issue#39](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/issues/39)

## Other

- When using the framework release package (`react_native_openharmony_release.har`), because Gesture Handler uses the deviceinfo interface, import `libdeviceinfo_ndk.z.so` in your project CMake config.

```diff
# CMakeLists.txt
target_link_libraries(cpplib PUBLIC libace_napi.z.so 
+ libdeviceinfo_ndk.z.so)
```

## Directory Structure

````
/rntpc_react-native-gesture-handler         # Project root directory
├── react-native-harmony-gesture-handler    # Library source (npm package @react-native-ohos/react-native-gesture-handler)
│   ├── harmony                             # HarmonyOS adaptation artifacts
│   │   └── gesture_handler.har             # HarmonyOS adaptation har package
│   ├── src                                 # React Native / TypeScript source
│   │   ├── components                      # Components (Pressable, ReanimatedSwipeable, etc.)
│   │   ├── handlers                        # Gesture handler definitions
│   │   ├── specs                           # TurboModule/Codegen type definitions
│   │   └── index.ts                        # Library entry file
│   ├── DrawerLayout                        # DrawerLayout subpath entry
│   ├── Swipeable                           # Swipeable subpath entry
│   ├── ReanimatedDrawerLayout              # ReanimatedDrawerLayout subpath entry
│   ├── ReanimatedSwipeable                 # ReanimatedSwipeable subpath entry
│   ├── scripts                             # Build and publish scripts
│   ├── img                                 # Documentation images
│   ├── CHANGELOG.md                        # Changelog
│   ├── COMMITTERS.md                       # Contributors list
│   └── package.json                        # Package configuration
├── tester                                  # Test project (Demo)
│   ├── harmony                             # DevEco Studio HarmonyOS project
│   │   ├── entry                           # Application entry module
│   ├── android                             # Android demo project
│   ├── rnoh_package                        # Local dependency package (rnoh-testerino)
│   ├── src                                 # Demo page source
│   └── scripts                             # Project helper scripts
├── jest                                    # Unit tests
├── buildEnv.sh                             # Build environment script
├── README.md                               # Chinese document
├── README_en.md                            # English document
└── LICENSE                                 # License
````

## How to Contribute

If you find any problem when using react-native-gesture-handler, submit an [issue](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/issues) or a [PR](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/pulls).

## License

This project is based on [The MIT License (MIT)](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/blob/master/LICENSE).

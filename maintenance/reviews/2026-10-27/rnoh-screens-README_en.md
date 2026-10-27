> Document template: v0.4.2

<p align="center">
  <h1 align="center"> <code>@react-native-ohos/react-native-screens</code> </h1>
</p>

This project is based on [react-native-screens](https://github.com/software-mansion/react-native-screens).

The repository of this third-party library has been migrated to Gitcode, and it can be downloaded directly from npm. The new package name is: `@react-native-ohos/react-native-screens`. The version mapping is as follows:

| Name | Version (npm address) | Release Information | Supported RN Version | Autolink | Compile API Version | Community Baseline Version | Source Address |
| --------------| ---------- | ------------------------------------------------------------ | ---------- | ------------- |------------------------ | ------------- | ------------- |
| @react-native-ohos/react-native-screens | [~ 4.10.0 (In development)](https://www.npmjs.com/package/@react-native-ohos/react-native-screens) | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-screens/releases) | 0.84.* | Yes | API12+ | 4.26.2 | [master](https://gitcode.com/CPF-RN/rntpc_react-native-screens/tree/master) |
| @react-native-ohos/react-native-screens | [~ 4.9.0](https://www.npmjs.com/package/@react-native-ohos/react-native-screens) | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-screens/releases) | 0.82.* | No | API12+ | 4.17.1 | [br_rnoh0.82](https://gitcode.com/CPF-RN/rntpc_react-native-screens/tree/br_rnoh0.82) |
| @react-native-ohos/react-native-screens | [~ 4.8.3](https://www.npmjs.com/package/@react-native-ohos/react-native-screens) | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-screens/releases) | 0.77.* | No | API12+ | 4.8.0 | [br_rnoh0.77](https://gitcode.com/CPF-RN/rntpc_react-native-screens/tree/br_rnoh0.77) |
| @react-native-ohos/react-native-screens | [~ 3.34.2](https://www.npmjs.com/package/@react-native-ohos/react-native-screens) | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-screens/releases) | 0.72.* | Yes | API12+ | 3.34.0 | [br_rnoh0.72](https://gitcode.com/CPF-RN/rntpc_react-native-screens/tree/br_rnoh0.72) |
| @react-native-oh-tpl/react-native-screens | [<= 3.34.0-0.0.2@deprecated](https://www.npmjs.com/package/@react-native-oh-tpl/react-native-screens) | [Github Releases(deprecated)](https://github.com/react-native-oh-library/react-native-harmony-screens/releases) | 0.72.* | No | API12+ | 3.34.0 | [sig](https://github.com/react-native-oh-library/react-native-harmony-screens) |

## Introduction

react-native-screens is a native screen navigation component library in the React Navigation ecosystem, providing native navigation screen stack management capabilities, supporting iOS, Android, and HarmonyOS platforms.

## Download and Install

Go to the project directory and execute the following instruction:

The following covers the usage of the HarmonyOS ported @react-native-ohos/react-native-screens with @react-native-ohos/native-stack and @react-native-ohos/stack.

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

## Dependency Notes

Choose the appropriate third-party library version based on the React Native framework version used in your application. The version mapping is as follows:

| RN Framework Version | Harmonized Dependency Library | Supported Version Range |
| --- | --- | --- |
| 0.84 | [@react-native-ohos/native-stack](https://gitcode.com/CPF-RN/rntpc_react-navigation/blob/master/packages/native-stack/README_en.md) | >=7.4.0-beta.13 |
| | [@react-native-ohos/stack](https://gitcode.com/CPF-RN/rntpc_react-navigation/blob/master/packages/stack/README_en.md) | >=7.2.12-beta.2 |
| | [@react-native-ohos/react-native-safe-area-context](https://gitcode.com/CPF-RN/rntpc_react-native-safe-area-context/blob/master/README_EN.md) | >=5.6.3 |
| | [@react-native-ohos/react-native-gesture-handler](https://gitcode.com/CPF-RN/rntpc_react-native-gesture-handler/blob/master/README_en.md) | >=2.30.1 |
| | [@react-native-ohos/react-native-reanimated](https://gitcode.com/CPF-RN/rntpc_react-native-reanimated/blob/master/README_en.md) | >=4.0.1 |

The HarmonyOS-side implementation of this library depends on the above third-party libraries. If they have already been introduced into your HarmonyOS application project, you may skip this section and use the library directly.

## Link

|                                      | Supported Autolink | Supported RN Version |
|--------------------------------------|------------------|------------|
| ~4.10.0                               |  Yes              |  0.84     |

Using AutoLink needs to be configured according to this document, Autolink Framework Guide Documentation: https://gitcode.com/CPF-RN/ohos_react_native/blob/master/docs/en/Autolinking.md

If the version you use supports Autolink and the project has been connected to Autolink, skip the ManualLink configuration.
<details>
  <summary>ManualLink: this step is a guide to manually configure native dependencies.</summary>

First, use DevEco Studio to open the HarmonyOS project `harmony` in the project directory.

### 1. Overrides RN SDK

To ensure the project relies on the same version of the RN SDK, you need to add an `overrides` field in the project's root `oh-package.json5` file, specifying the RN SDK version to be used. The replacement version can be a specific version number, a semver range, or a locally available HAR package or source directory.

For more information about the purpose of this field, please refer to the [official documentation](https://developer.huawei.com/consumer/en/doc/harmonyos-guides-V5/ide-oh-package-json5-V5#en-us_topic_0000001792256137_overrides).

```json
{
  "overrides": {
    "@rnoh/react-native-openharmony": "^0.84.1" // ohpm online version
    // "@rnoh/react-native-openharmony" : "./react_native_openharmony.har" // path to local har package
    // "@rnoh/react-native-openharmony" : "./react_native_openharmony" // path to source code
  }
}
```
### 2. Introducing Native Code

Currently, two methods are available:

1. Use the HAR file (preferred method until IDE features are fully developed);
2. Directly link to the source code.

Method 1 (recommended): Use the HAR file.

> [!TIP] The HAR file is stored in the `harmony` directory in the installation path of the third-party library.

Open `entry/oh-package.json5` file and add the following dependencies:

```json
"dependencies": {
    "@rnoh/react-native-openharmony": "file:../../node_modules/@rnoh/react-native-harmony/harmony/react_native_openharmony.har",
    "@react-native-ohos/react-native-screens": "file:../../node_modules/@react-native-ohos/react-native-screens/harmony/screens.har"
  }
```

Click the `sync` button in the upper right corner.

Alternatively, run the following instruction on the terminal:

```bash
cd entry
ohpm install
```

Method 2: Directly link to the source code.

> [!TIP] For details, see [Directly Linking Source Code](https://gitcode.com/CPF-RN/usage-docs/blob/master/en/link-source-code.md).

### 3. Configuring CMakeLists and Introducing Package

Open `entry/src/main/cpp/CMakeLists.txt` and add the following code:

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

Open `entry/src/main/cpp/PackageProvider.cpp`, add:

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

### 4. Introducing Component to ArkTS

Find `function buildCustomRNComponent()`, which is usually located in `entry/src/main/ets/pages/index.ets` or `entry/src/main/ets/rn/LoadBundle.ets`, and add the following code:

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

> [!TIP] If the repository uses a mixed solution, the component name needs to be added. 

Find the constant `arkTsComponentNames` in `entry/src/main/ets/pages/index.ets` or `entry/src/main/ets/rn/LoadBundle.ets` and add the component name to the array.

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

### 5. Introducing Package to ArkTS

Open `src/main/ets/RNOHPackagesFactory.ets`, add:

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

### Running

Click the `sync` button in the upper right corner.

Alternatively, run the following instruction on the terminal:

```bash
cd entry
ohpm install
```

Then build and run the code.

## Constraints and Limitations

### Compatibility

To use this repository, you need to use the correct React-Native and RNOH versions. In addition, you need to use DevEco Studio and the ROM on your phone.

Verified in the following versions.

1. RNOH: 0.84.2; SDK: HarmonyOS 6.0.1 Release SDK; IDE: DevEco Studio 6.0.1.260; ROM:6.0.0.130;

## Usage Example

The following code shows the basic use scenario of the repository:

> [!WARNING] The name of the imported repository remains unchanged.

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

## API

> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **HarmonyOS Support** is **yes**, it means that the HarmonyOS platform supports this property; **No** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

Below are the basic props of the HarmonyOS-adapted @react-native-ohos/react-native-screens, all of which have been verified. For more details on additional props, refer to the documentation of react-native-screens at https://reactnavigation.org/docs/screen.

| Name                                                      | Description                                                  | Type     | Required | Platform    | HarmonyOS Support |
| --------------------------------------------------------- | ------------------------------------------------------------ | -------- | -------- | ----------- | ----------------- |
| enableScreens | Supports native and React Native View components | function | No | Android,iOS | Yes |
| enableFreeze | Support for react-freeze, using React Suspense mechanism to prevent partial rendering of React component tree | function | No | Android,iOS | No |
| createNativeStackNavigator | Provides screen switching capabilities | function | No | Android,iOS | No |
| NativeStackNavigationProp | Encapsulation of page switching properties | object | No | Android,iOS | Yes |
| NativeStackNavigationOptions | Encapsulation of navigation bar property settings | object | No | Android,iOS | No |
| FullWindowOverlay | A component that can place its child components on top of other components | object | No | Android,iOS | No |
| useTransitionProgress | Provides animation interpolator for screen transitions | function | No | Android,iOS | No |
| useReanimatedTransitionProgress ReanimatedScreenProvider | Frame callback called during screen transitions, used for react-native-reanimated 2.0 and above versions, and encapsulated using ReanimatedScreenProvider | function | No | Android,iOS | No |
| screensEnabled | Returns whether native screens are enabled | function | No | All | Yes |
| freezeEnabled | Returns whether freeze is enabled | function | No | All | No |
| SearchBar | Native search bar component | component | No | All | Yes |
| ScreenContainer | Screen container component, used to wrap multiple Screens | component | No | All | Yes |
| ScreenStack | Native screen stack container component | component | No | All | Yes |
| ScreenStackItem | Screen container component used inside ScreenStack | component | No | Android | Yes |
| ScreenFooter | Footer container component for formSheet presentation | component | No | Android,iOS | No |
| ScreenContentWrapper | Content wrapper container component, used for scrollable content inside Screen | component | No | All | No |
| isSearchBarAvailableForCurrentPlatform | Returns whether the current platform supports SearchBar | function | No | Android | Yes |
| executeNativeBackPress | Executes the native back button press | function | No | Android,iOS | Yes |
| compatibilityFlags | Compatibility flags object | object | No | All | Yes |
| featureFlags | Feature flags object | object | No | All | Yes |

### ScreenProps Property List

| Name                                         | Description                                                  | Type                                                         | Required | Platform    | HarmonyOS Support |
| -------------------------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ | -------- | ----------- | ----------------- |
| customAnimationOnSwipe | Boolean indicating whether swipe dismissal should trigger the animation provided by "stackAnimation". Defaults to "false". | `boolean` | No | iOS | Yes |
| fullScreenSwipeEnabled | Full screen swipe | `boolean` | No | iOS | Yes |
| fullScreenSwipeShadowEnabled | Boolean indicating whether the full screen dismissal gesture has a shadow under the view during transition. The gesture uses a custom transition, so there is no shadow by default. When enabled, a custom shadow view is added during the transition to mimic the default iOS shadow. Defaults to "true". | `boolean` | No | iOS | Yes |
| gestureEnabled | Whether to enable gesture sliding | `boolean` | No | iOS | Yes |
| gestureResponseDistance | Effective area for gesture sliding | `GestureResponseDistanceType` | No | iOS | Yes |
| homeIndicatorHidden | Whether the home indicator should be hidden on this screen. Defaults to "false". | `boolean` | No | iOS | No |
| hideKeyboardOnSwipe | Whether the keyboard should be hidden when swiping to the previous screen. Defaults to "false". | `boolean` | No | iOS | No |
| scrollEdgeEffects | Configures the scroll edge effect of the content ScrollView | { top/bottom/left/right: 'automatic' \| 'hard' \| 'soft' \| 'hidden' } | No | iOS | No |
| onAppear | Page show | function | No | All | No |
| onDisappear | Page disappear | function | No | All | No |
| onSheetDetentChanged | Callback called when the current screen is in `formSheet` presentation state and its detent has changed | function | No | All | No |
| onWillAppear | Page will show | function | No | All | No |
| onWillDisappear | Page will disappear | function | No | All | No |
| preventNativeDismiss | Indicates whether to prevent the current screen from being dismissed, defaults to `false` | `boolean` | No | iOS | Yes |
| replaceAnimation | Stack entry/exit type | `ScreenReplaceTypes` | No | All | Yes |
| screenId | Way to identify the screen in native code | `string \| undefined` | No | iOS | Yes |
| screenOrientation | Screen display direction | `ScreenOrientationTypes` | No | All | Yes |
| sheetAllowedDetents | Describes the heights at which the sheet can rest, only valid when `presentation` is set to `formSheet` | `number[] \| 'fitToContents' \| 'medium' \| 'large' \| 'all'` | No | All | No |
| sheetElevation | Describes the elevation of the sheet, affecting the shadow on the top edge of the sheet | `number` | No | Android | No |
| sheetExpandsWhenScrolledToEdge | Indicates whether the sheet should expand to a larger detent when scrolled to the edge, defaults to `true` | `boolean` | No | iOS | No |
| sheetCornerRadius | The corner radius that the sheet will attempt to render | `number` | No | iOS | No |
| sheetGrabberVisible | Indicates whether the sheet shows a grabber at the top, defaults to `false` | `boolean` | No | iOS | No |
| stackAnimation | Appearance/disappearance animation when the screen is pushed or popped from the top of the stack | `StackAnimationTypes` | No | All | Yes |
| stackPresentation | How the screen should be presented | `StackPresentationTypes` | No | All | Yes |
| statusBarAnimation | Sets the status bar animation (similar to the `StatusBar` component) | `'none' \| 'fade' \| 'slide'` | No | All | No |
| statusBarColor | Sets the status bar color (deprecated) | `ColorValue` | No | All | No |
| statusBarHidden | Indicates whether the status bar should be hidden on this screen, defaults to `false` | `boolean` | No | All | Yes |
| statusBarStyle | Sets the status bar style (similar to the `StatusBar` component) | `'inverted' \| 'auto' \| 'light' \| 'dark'` | No | All | Yes |
| statusBarTranslucent | Sets whether the status bar is translucent | `boolean` | No | All | Yes |
| transitionDuration | Changes the duration (in milliseconds) of `slide_from_bottom`, `fade_from_bottom`, `fade`, and `simple_push` transitions on iOS, defaults to `500` | `number` | No | iOS | Yes |
| onGestureCancel | Callback called after swipe back is cancelled | function | No | All | Yes |
| activityState | Controlling page visibility for multiple pages inside a container | `number` | No | All | Yes |
| active | Animation interpolation controlling the screen activation state (used by navigator transitions) | `0 \| 1 \| AnimatedInterpolation<number>` | No | All | No |
| shouldFreeze | Whether to freeze the current screen with react-freeze | `boolean` | No | All | No |
| enabled | Whether to enable the native screen implementation; must match the container's value | `boolean` | No | All | No |
| isNativeStack | Internal property, marks whether it is used by the native-stack navigator | `boolean` | No | All | No |
| hasLargeHeader | Internal property, marks whether the current header is an iOS large title | `boolean` | No | iOS | No |
| freezeOnBlur | Whether to suspend rendering when blurred; defaults to true once enableFreeze() is called | `boolean` | No | All | No |
| nativeBackButtonDismissalEnabled | Whether to perform pop on the native side when the Android back button is pressed, defaults to false | `boolean` | No | Android | No |
| navigationBarColor | Deprecated, setting this property has no effect | `ColorValue` | No | Android | No |
| navigationBarTranslucent | Deprecated, setting this property has no effect | `boolean` | No | Android | No |
| navigationBarHidden | Whether to hide the navigation bar, defaults to false | `boolean` | No | Android | Yes |
| onComponentRef | Internal callback used to obtain the component reference | `function` | No | All | No |
| onDismissed | Callback after the screen is dismissed by the hardware back button (Android) or the dismiss gesture | `function` | No | All | No |
| onHeaderHeightChange | Callback after the header height changes | `function` | No | All | No |
| onHeaderBackButtonClicked | Internal callback, called when the Android native header back button is clicked and native dismissal is not enabled | `function` | No | Android | No |
| onNativeDismissCancelled | Internal callback, called after dismiss is prevented when preventNativeDismiss is true | `function` | No | iOS | No |
| onTransitionProgress | Internal callback, called on every frame during native-stack screen transitions | `function` | No | All | No |
| sheetLargestUndimmedDetentIndex | The largest sheet detent index for which the view underneath is not dimmed, only valid for formSheet presentation | `number \| 'none' \| 'last' \| 'medium' \| 'large' \| 'all'` | No | All | No |
| sheetInitialDetentIndex | The initial detent index the sheet expands to after being opened, only valid for formSheet presentation | `number \| 'last'` | No | All | No |
| sheetShouldOverflowTopInset | Whether the sheet content extends behind the status bar / notch area, only valid for formSheet presentation | `boolean` | No | Android | No |
| sheetDefaultResizeAnimationEnabled | Whether to use the default native resize animation when the fitToContents content size changes, only valid for formSheet presentation | `boolean` | No | Android | No |
| swipeDirection | Sets the swipe direction for dismissing the screen (vertical / horizontal) | `SwipeDirectionTypes` | No | iOS | Yes |
| unstable_sheetFooter | Renders the footer component for formSheet presentation | `function` | No | Android | No |

### ScreenStackHeaderSubviewProps Property List

| Name                  | Description                                                  | Type      | Required | Platform | HarmonyOS Support |
| --------------------- | ------------------------------------------------------------ | --------- | -------- | -------- | ----------------- |
| hidesSharedBackground | Indicates whether the background that this item may share with other items in the bar should be hidden. Only applicable to type="right" and type="left" subviews. Available only from iOS 26.0 and above | `boolean` | No | All | No |

### ScreenStackHeaderConfigProps Property List

| Name                                             | Description                                                  | Type                    | Required | Platform | HarmonyOS Support |
| ------------------------------------------------ | ------------------------------------------------------------ | ----------------------- | -------- | -------- | ----------------- |
| backButtonInCustomView | Whether to show the back button with a custom left header | `boolean` | No | All | No |
| backgroundColor | Controls the color of the navigation title | `ColorValue` | No | All | Yes |
| backTitle | Back button text content | `string` | No | iOS | Yes |
| backTitleFontSize | Back button text font size | `number` | No | iOS | Yes |
| backTitleVisible | Whether the back button text is visible | `boolean` | No | iOS | Yes |
| blurEffect | Blur effect applied to the title, used with backgroundColor alpha < 1 | `BlurEffectTypes` | No | iOS | Yes |
| color | Controls the color of items rendered on the title, including back icon, back text, and title text | `ColorValue` | No | All | Yes |
| direction | Whether the stack should use rtl or ltr form | `'rtl' \| 'ltr'` | No | All | No |
| disableBackButtonMenu | Controls whether to show the menu on long press of the back button on iOS >= 14 | `boolean` | No | iOS | No |
| backButtonDisplayMode | Controls the behavior of the back button | `BackButtonDisplayMode` | No | iOS | No |
| headerLeftBarButtonItems | Array of UIBarButtomItems on the left side of the title | `HeaderBarButtonItem[]` | No | iOS | No |
| headerRightBarButtonItems | Array of UIBarButtomItems on the right side of the title | `HeaderBarButtonItem[]` | No | iOS | No |
| hidden | Hides the title bar | `boolean` | No | All | Yes |
| hideBackButton | Hides the title bar back button | `boolean` | No | All | Yes |
| hideShadow | Controls whether to hide the title bar shadow or bottom border | `boolean` | No | All | Yes |
| largeTitle | Sets native property to prefer large title headers (as in iOS Settings) | `boolean` | No | iOS | No |
| largeTitleBackgroundColor | Controls the color of the navigation title when the edge of any scrollable content reaches the matching edge of the navigation bar | `ColorValue` | No | All | No |
| largeTitleColor | Customizes the color used for the large title, defaults to the titleColor property | `ColorValue` | No | iOS | No |
| largeTitleFontFamily | Customizes the font family used for the large title | `string` | No | iOS | No |
| largeTitleFontSize | Customizes the font size used for the large title | `number` | No | iOS | No |
| largeTitleFontWeight | Customizes the font weight used for the large title | `string` | No | iOS | No |
| largeTitleHideShadow | Allows disabling the drop shadow under the navigation title when the edge of any scrollable content reaches the matching edge of the navigation bar | `boolean` | No | All | No |
| title | Title bar title | `string` | No | All | Yes |
| titleColor | Allows setting the text color of the title | `ColorValue` | No | All | Yes |
| titleFontFamily | Custom font family used for the title | `string` | No | All | Yes |
| titleFontSize | Custom font size used for the title | `number` | No | All | Yes |
| titleFontWeight | Custom font weight used for the title | `string` | No | All | Yes |
| topInsetEnabled | Deprecated, setting this property has no effect | `boolean` | No | All | No |
| translucent | Whether the title bar is transparent | `boolean` | No | All | Yes |
| backTitleFontFamily | Custom font family for the back button text | `string` | No | iOS | No |
| onAttached | Callback executed when the header is attached | `function` | No | All | No |
| onDetached | Callback executed when the header is detached | `function` | No | All | No |
| disableTopInsetApplication | Disables top inset handling for the outermost visible header | `boolean` | No | Android | No |
| disableLeftInsetApplication | Disables left inset handling, applied to the whole subtree | `boolean` | No | Android | No |
| disableRightInsetApplication | Disables right inset handling, applied to the whole subtree | `boolean` | No | Android | No |
| disableBottomInsetApplication | Disables bottom inset handling, applied to the whole subtree | `boolean` | No | Android | No |
| experimental_userInterfaceStyle | Overrides the header system appearance (experimental) | `'unspecified' \| 'light' \| 'dark'` | No | iOS | No |

### SearchBarProps Property List

| Name                                      | Description                                                  | Type                                                         | Required | Platform | HarmonyOS Support |
| ----------------------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ | -------- | -------- | ----------------- |
| autoCapitalize | Automatic capitalization behavior, defaults to `systemDefault` | `'systemDefault' \| 'none' \| 'words' \| 'sentences' \| 'characters'` | No | All | No |
| autoFocus | Auto focus the search bar on mount | `boolean` | No | Android | Yes |
| barTintColor | Search field background color | `ColorValue` | No | All | Yes |
| tintColor | Color of cursor caret and cancel button text | `ColorValue` | No | iOS | Yes |
| cancelButtonText | Text used to replace default `Cancel` button text | `string` | No | iOS | Yes |
| disableBackButtonOverride | Specifies whether the back button should close the search bar text input | `boolean` | No | Android | No |
| hideNavigationBar | Indicates whether the navigation bar should be hidden | `boolean` | No | iOS | Yes |
| hideWhenScrolling | Indicates whether the search bar should be hidden when scrolling | `boolean` | No | iOS | No |
| inputType | Sets input type, defaults to `text` | `'text' \| 'phone' \| 'number' \| 'email'` | No | Android | Yes |
| obscureBackground | Indicates whether the underlying content should be obscured | `boolean` | No | iOS | No |
| placeholder | Text displayed when search field is empty | `string` | No | All | Yes |
| placement | Position of the search bar | `SearchBarPlacement` | No | iOS | No |
| allowToolbarIntegration | Indicates whether the system can place the search bar between other toolbar items on iPhone | `boolean` | No | iOS | No |
| textColor | Search field text color | `ColorValue` | No | All | Yes |
| hintTextColor | Search hint text color | `ColorValue` | No | Android | Yes |
| headerIconColor | Color of search and close icons displayed in header | `ColorValue` | No | Android | Yes |
| shouldShowHintSearchIcon | Show search hint icon when search bar is focused | `boolean` | No | Android | Yes |
| ref | Imperative reference supporting focus/blur/clearText/setText/cancelSearch/toggleCancelButton | `SearchBarCommands` | No | All | Yes |
| onClose | Close search | function | No | Android,iOS | Yes |
| onOpen | Show search | function | No | Android,iOS | Yes |
| onSearchButtonPress | Search | function | No | Android,iOS | Yes |
| onCancelButtonPress | Cancel search | function | No | Android,iOS | Yes |
| onChangeText | Search bar text change | function | No | Android,iOS | Yes |
| onFocus | Callback when the search bar gains focus | `function` | No | All | Yes |
| onBlur | Callback when the search bar loses focus | `function` | No | All | Yes |

### StackAnimationTypes Value List

| Name              | Description            | Type                  | Required | Platform | HarmonyOS Support |
| ----------------- | ---------------------- | --------------------- | -------- | -------- | ----------------- |
| default | Default animation | `'default'` | No | All | Yes |
| fade | Fade in/out animation | `'fade'` | No | All | Yes |
| fade_from_bottom | Fade in/out from bottom animation | `'fade_from_bottom'` | No | All | Yes |
| flip | Slide in from right animation | `'flip'` | No | All | Yes |
| none | No animation | `'none'` | No | All | Yes |
| simple_push | Simple push animation | `'simple_push'` | No | All | Yes |
| slide_from_bottom | Slide in from bottom animation | `'slide_from_bottom'` | No | All | Yes |
| slide_from_right | Slide in from right animation | `'slide_from_right'` | No | All | Yes |
| slide_from_left | Slide in from left animation | `'slide_from_left'` | No | All | Yes |
| ios_from_right | iOS style slide in from right animation | `'ios_from_right'` | No | All | Yes |
| ios_from_left | iOS style slide in from left animation | `'ios_from_left'` | No | All | Yes |

### StackPresentationTypes Value List

| Name                      | Description        | Type                          | Required | Platform | HarmonyOS Support |
| ------------------------- | ------------------ | ----------------------------- | -------- | -------- | ----------------- |
| push | Standard push presentation | `'push'` | No | All | Yes |
| modal | Modal presentation | `'modal'` | No | All | Yes |
| transparentModal | Transparent modal presentation | `'transparentModal'` | No | All | Yes |
| containedModal | Contained modal presentation | `'containedModal'` | No | All | Yes |
| containedTransparentModal | Contained transparent modal presentation | `'containedTransparentModal'` | No | All | Yes |
| fullScreenModal | Full screen modal presentation | `'fullScreenModal'` | No | All | Yes |
| formSheet | Form sheet presentation | `'formSheet'` | No | All | No |
| pageSheet | Page sheet presentation | `'pageSheet'` | No | All | Yes |

### ScreenReplaceTypes Value List

| Name | Description  | Type     | Required | Platform | HarmonyOS Support |
| ---- | ------------ | -------- | -------- | -------- | ----------------- |
| push | Push new screen | `'push'` | No | All | Yes |
| pop | Pop current screen | `'pop'` | No | All | Yes |

### ScreenOrientationTypes Value List

| Name            | Description  | Type                | Required | Platform | HarmonyOS Support |
| --------------- | ------------ | ------------------- | -------- | -------- | ----------------- |
| default | Default screen orientation | `'default'` | No | All | Yes |
| all | All screen orientations | `'all'` | No | All | Yes |
| portrait | Portrait orientation | `'portrait'` | No | All | Yes |
| portrait_up | Portrait up orientation | `'portrait_up'` | No | All | Yes |
| portrait_down | Portrait down orientation | `'portrait_down'` | No | All | Yes |
| landscape | Landscape orientation | `'landscape'` | No | All | Yes |
| landscape_left | Landscape left orientation | `'landscape_left'` | No | All | Yes |
| landscape_right | Landscape right orientation | `'landscape_right'` | No | All | Yes |

### BlurEffectTypes Value List

| Name                         | Description                    | Type                             | Required | Platform | HarmonyOS Support |
| ---------------------------- | ------------------------------ | -------------------------------- | -------- | -------- | ----------------- |
| none | No blur effect | `'none'` | No | All | Yes |
| extraLight | Extra light blur effect | `'extraLight'` | No | All | Yes |
| light | Light blur effect | `'light'` | No | All | Yes |
| dark | Dark blur effect | `'dark'` | No | All | Yes |
| regular | Regular blur effect | `'regular'` | No | All | Yes |
| prominent | Prominent blur effect | `'prominent'` | No | All | No |
| systemUltraThinMaterial | System ultra thin material blur effect | `'systemUltraThinMaterial'` | No | All | No |
| systemThinMaterial | System thin material blur effect | `'systemThinMaterial'` | No | All | No |
| systemMaterial | System material blur effect | `'systemMaterial'` | No | All | No |
| systemThickMaterial | System thick material blur effect | `'systemThickMaterial'` | No | All | No |
| systemChromeMaterial | System Chrome material blur effect | `'systemChromeMaterial'` | No | All | No |
| systemUltraThinMaterialLight | System ultra thin material (light) blur effect | `'systemUltraThinMaterialLight'` | No | All | No |
| systemThinMaterialLight | System thin material (light) blur effect | `'systemThinMaterialLight'` | No | All | No |
| systemMaterialLight | System material (light) blur effect | `'systemMaterialLight'` | No | All | No |
| systemThickMaterialLight | System thick material (light) blur effect | `'systemThickMaterialLight'` | No | All | No |
| systemChromeMaterialLight | System Chrome material (light) blur effect | `'systemChromeMaterialLight'` | No | All | No |
| systemUltraThinMaterialDark | System ultra thin material (dark) blur effect | `'systemUltraThinMaterialDark'` | No | All | No |
| systemThinMaterialDark | System thin material (dark) blur effect | `'systemThinMaterialDark'` | No | All | No |
| systemMaterialDark | System material (dark) blur effect | `'systemMaterialDark'` | No | All | No |
| systemThickMaterialDark | System thick material (dark) blur effect | `'systemThickMaterialDark'` | No | All | No |
| systemChromeMaterialDark | System Chrome material (dark) blur effect | `'systemChromeMaterialDark'` | No | All | No |

### GestureResponseDistanceType Property List

| Name   | Description        | Type     | Required | Platform | HarmonyOS Support |
| ------ | ------------------ | -------- | -------- | -------- | ----------------- |
| start | Gesture response start distance | `number` | No | All | Yes |
| end | Gesture response end distance | `number` | No | All | Yes |
| top | Gesture response top distance | `number` | No | All | Yes |
| bottom | Gesture response bottom distance | `number` | No | All | Yes |

### SearchBarPlacement Value List

| Name               | Description                            | Type                   | Required | Platform | HarmonyOS Support |
| ------------------ | -------------------------------------- | ---------------------- | -------- | -------- | ----------------- |
| automatic | Automatically selects search bar position | `'automatic'` | No | All | No |
| inline | Inline search bar position (deprecated from iOS 26) | `'inline'` | No | All | No |
| stacked | Stacked search bar position | `'stacked'` | No | All | No |
| integrated | Integrated search bar position | `'integrated'` | No | All | No |
| integratedButton | Integrated button search bar position | `'integratedButton'` | No | All | No |
| integratedCentered | Integrated centered search bar position | `'integratedCentered'` | No | All | No |

### SharedHeaderBarButtonItem Property List

| Name                  | Description                                            | Type                                                         | Required | Platform | HarmonyOS Support |
| --------------------- | ------------------------------------------------------ | ------------------------------------------------------------ | -------- | -------- | ----------------- |
| index | Position of the item in the navigation items array | `number` | No | All | No |
| title | Label of the item | `string` | No | All | No |
| titleStyle | Style of the item label | `{ fontFamily?: string; fontSize?: number; fontWeight?: string; color?: ColorValue; }` | No | All | No |
| icon | Icon of the item | `PlatformIconIOS` | No | All | No |
| variant | Variant of the item | `'plain' \| 'done' \| 'prominent'` | No | All | No |
| tintColor | Tint color applied to the item | `ColorValue` | No | All | No |
| disabled | Boolean indicating whether the item is disabled | `boolean` | No | All | No |
| width | Width of the item | `number` | No | All | No |
| hidesSharedBackground | Indicates whether the background that this item may share with other items should be hidden | `boolean` | No | All | No |
| sharesBackground | Indicates whether this item can share background with other items in the navigation bar or toolbar | `boolean` | No | All | No |
| identifier | Identifier used to match items during navigation bar or toolbar transitions | `string` | No | All | No |
| badge | Badge to render on the item | `{ value: string; style?: { color?: ColorValue; backgroundColor?: ColorValue; fontFamily?: string; fontSize?: number; fontWeight?: string; }; }` | No | All | No |
| accessibilityLabel | Accessibility label | `string` | No | All | No |
| accessibilityHint | Accessibility hint | `string` | No | All | No |

### HeaderBarButtonItemWithAction Property List

| Name                            | Description                        | Type      | Required | Platform | HarmonyOS Support |
| ------------------------------- | ---------------------------------- | --------- | -------- | -------- | ----------------- |
| onPress | Callback when the item is pressed | function | Yes | All | No |
| selected | Boolean indicating whether the item is selected | `boolean` | No | All | No |
| changesSelectionAsPrimaryAction | Boolean indicating whether the item represents an action or selection [Moved to HeaderBarButtonItemWithMenu in 4.26.2] [Moved to HeaderBarButtonItemWithMenu in 4.26.2] | `boolean` | No | All | No |
| type | Type of the item, fixed to 'button' | `'button'` | No | All | No |

### HeaderBarButtonItemMenuAction Property List

| Name                 | Description                     | Type                                                         | Required | Platform | HarmonyOS Support |
| -------------------- | ------------------------------- | ------------------------------------------------------------ | -------- | -------- | ----------------- |
| type | Type of the menu action, fixed to 'action' | `'action'` | Yes | All | No |
| title | Title of the menu action (formerly label) | `string` | No | All | No |
| onPress | Callback when the menu action is pressed | function | Yes | All | No |
| icon | Icon of the menu action | `PlatformIconIOSSfSymbol` | No | All | No |
| state | State of the menu item | `'on' \| 'off' \| 'mixed'` | No | All | No |
| discoverabilityLabel | Discoverability label of the menu item | `string` | No | All | No |
| subtitle | Subtitle of the menu action | `string` | No | All | No |
| disabled | Whether to apply the disabled style to the menu item | `boolean` | No | All | No |
| destructive | Whether to apply the destructive style to the menu item | `boolean` | No | All | No |
| hidden | Whether to apply the hidden style to the menu item | `boolean` | No | All | No |
| keepsMenuPresented | Whether to keep the menu presented after triggering a menu item action | `boolean` | No | All | No |

### HeaderBarButtonItemSubmenu Property List

| Name  | Description                    | Type                                           | Required | Platform | HarmonyOS Support |
| ----- | ------------------------------ | ---------------------------------------------- | -------- | -------- | ----------------- |
| type | Type of the submenu, fixed to 'submenu' | `'submenu'` | Yes | All | No |
| title | Title of the submenu (formerly label) | `string` | No | All | No |
| icon | Icon of the submenu | `PlatformIconIOSSfSymbol` | No | All | No |
| items | List of items in the submenu | `HeaderBarButtonItemWithMenu['menu']['items']` | Yes | All | No |
| displayInline | Whether the submenu is displayed inline | `boolean` | No | All | No |
| destructive | Whether the submenu is a destructive action | `boolean` | No | All | No |
| singleSelection | Whether the submenu is single selection | `boolean` | No | All | No |
| displayAsPalette | Whether the submenu is displayed as a palette | `boolean` | No | All | No |

### HeaderBarButtonItemWithMenu Property List

| Name | Description    | Type                                                         | Required | Platform | HarmonyOS Support |
| ---- | -------------- | ------------------------------------------------------------ | -------- | -------- | ----------------- |
| menu | Menu configuration of the button. `title` is the menu title (formerly label); `singleSelection` enables single selection; `displayAsPalette` displays the menu as a palette | `{ title?: string; items: (HeaderBarButtonItemMenuAction \| HeaderBarButtonItemSubmenu)[]; singleSelection?: boolean; displayAsPalette?: boolean; }` | Yes | All | No |
| type | Type of the item, fixed to 'menu' | `'menu'` | No | All | No |
| changesSelectionAsPrimaryAction | Whether the button title indicates the selected state (moved from WithAction) | `boolean` | No | All | No |

### HeaderBarButtonItemSpacing Property List

| Name    | Description    | Type     | Required | Platform | HarmonyOS Support |
| ------- | -------------- | -------- | -------- | -------- | ----------------- |
| spacing | Spacing between buttons | `number` | Yes | All | No |
| type | Type of the item, fixed to 'spacing' | `'spacing'` | No | All | No |

### HeaderBarButtonItem Value List

| Name                          | Description              | Type                            | Required | Platform | HarmonyOS Support |
| ----------------------------- | ------------------------ | ------------------------------- | -------- | -------- | ----------------- |
| HeaderBarButtonItemWithAction | Navigation bar button with click action | `HeaderBarButtonItemWithAction` | No | All | No |
| HeaderBarButtonItemWithMenu | Navigation bar button with menu | `HeaderBarButtonItemWithMenu` | No | All | No |
| HeaderBarButtonItemSpacing | Spacing between navigation bar buttons | `HeaderBarButtonItemSpacing` | No | All | No |

### BottomTabs Value List

| Name                               | Description                | Type                                               | Required | Platform | HarmonyOS Support |
| ---------------------------------- | -------------------------- | -------------------------------------------------- | -------- | -------- | ----------------- |
| `normal` | Tab item appearance in the normal (unselected, unfocused) state (includes tabBarItemTitleFontColor / tabBarItemIconColor) | `object` | No | Android | No |
| `selected` | Tab item appearance in the selected state | `object` | No | Android | No |
| `focused` | Tab item appearance in the focused state | `object` | No | Android | No |
| `disabled` | Tab item appearance in the disabled state | `object` | No | Android | No |
| `tabBarItemTitleSmallLabelFontSize` | Font size (sp) of the unselected tab item title | `number` | No | Android | No |
| `tabBarItemTitleLargeLabelFontSize` | Font size (sp) of the selected tab item title | `number` | No | Android | No |
| `navStateRequest` | Navigation state request passed to native (selectedScreenKey + baseProvenance, required) | `object` | **Yes** | Android,iOS | No |
| `rejectStaleNavStateUpdates` | Whether to reject JS navigation updates based on stale state, defaults to false | `boolean` | No | Android,iOS | No |
| `tabBarHidden` | Whether to hide the tab bar, defaults to false | `boolean` | No | Android,iOS | No |
| `nativeContainerStyle` | Native container style (backgroundColor) | `object` | No | Android,iOS | No |
| `direction` | Layout direction of the native container | `'inherit' \| 'ltr' \| 'rtl'` | No | Android,iOS | No |
| `colorScheme` | Color scheme used by the container and its sub-containers | `'inherit' \| 'light' \| 'dark'` | No | Android,iOS | No |
| `onTabSelected` | Callback when the selected tab changes | `function` | No | Android,iOS | No |
| `onTabSelectionRejected` | Callback when native rejects a tab selection request | `function` | No | Android,iOS | No |
| `onTabSelectionPrevented` | Callback when tab selection is prevented by preventNativeSelection | `function` | No | Android,iOS | No |
| `tabBarRespectsIMEInsets` | Whether the tab bar respects keyboard insets (API 30+, defaults to false) | `boolean` | No | Android | No |

### BottomTabsScreen Value List

| Name                             | Description                                      | Type                                        | Required | Platform     | HarmonyOS Support |
| -------------------------------- | ------------------------------------------------ | ------------------------------------------- | -------- | ------------ | ----------------- |
| `title` | Tab title | `string` | No | Android,iOS | No |
| `badgeValue` | Badge text of the tab | `string` | No | Android,iOS | No |
| `onWillAppear` | Triggered when the page is about to enter the foreground and become visible | `BottomTabsScreenEventHandler<EmptyObject>` | No | Android,iOS | No |
| `onDidAppear` | Triggered after the page is fully shown and has entered the foreground | `BottomTabsScreenEventHandler<EmptyObject>` | No | Android,iOS | No |
| `onWillDisappear` | Triggered when the page is about to leave the foreground and become invisible (but has not disappeared yet) | `BottomTabsScreenEventHandler<EmptyObject>` | No | Android,iOS | No |
| `onDidDisappear` | Triggered after the page is fully invisible (has left the foreground) | `BottomTabsScreenEventHandler<EmptyObject>` | No | Android,iOS | No |
| `screenKey` | Unique identifier of the tab | `string` | **Yes** | Android,iOS | No |
| `preventNativeSelection` | Whether to prevent this screen from being selected natively, defaults to false | `boolean` | No | Android,iOS | No |
| `specialEffects` | Effects when the tab is selected again (popToRoot / scrollToTop) | `object` | No | Android,iOS | No |
| `testID` | testID of TabsScreen | `string` | No | Android,iOS | No |
| `accessibilityLabel` | Accessibility label of TabsScreen | `string` | No | Android,iOS | No |
| `tabBarItemTestID` | testID of TabBarItem | `string` | No | Android,iOS | No |
| `tabBarItemAccessibilityLabel` | Accessibility label of TabBarItem | `string` | No | Android,iOS | No |
| `scrollEdgeAppearance` | Tab bar appearance when scroll content reaches the tab bar edge | `object` | No | iOS | No |
| `overrideScrollViewContentInsetAdjustmentBehavior` | Whether to restore the automatic inset behavior of the first ScrollView (Fabric only, defaults to true) | `boolean` | No | iOS | No |
| `experimental_userInterfaceStyle` | Overrides the tab bar system appearance (experimental) | `'unspecified' \| 'light' \| 'dark'` | No | iOS | No |

## Known Issues

- [ ] The formSheet page is currently using a regular page, and the Context.openBindSheet system method used on this page has a misalignment of the binding position of the JS page, resulting in unresponsive button events. Need to be supplemented after system correction.  [issue#4](https://github.com/react-native-oh-library/react-native-harmony-screens/issues/4)

- [ ] RNSScreenFooter depends on formSheet. This function needs to be added after the formSheet function is implemented.  [issue#28](https://github.com/react-native-oh-library/react-native-harmony-screens/issues/28)

- [ ] GestureDetectorProvider cannot be referenced in the 0.77 environment using the original library's reference method. Needs system framework processing.  [issue#29](https://github.com/react-native-oh-library/react-native-harmony-screens/issues/29)

- [ ] When different properties are set through Options in Screen and its child components, the properties received on the ets side are lost in the 0.77 environment. Needs system framework processing.  [issue#30](https://github.com/react-native-oh-library/react-native-harmony-screens/issues/30)

- [ ] sheetInitialDetent: Index of the detent the sheet should expand to after being opened. Attributes of the formsheet function. Not implemented
- [ ] sheetElevation: Integer value describing elevation of the sheet, impacting shadow on the top edge of the sheet.Attributes of the formsheet function. Not implemented
- [ ] sheetAllowedDetents:  Describes heights where a sheet can rest. Attributes of the formsheet function. Not implemented
- [ ] sheetLargestUndimmedDetent: The largest sheet detent for which a view underneath won't be dimmed. Attributes of the formsheet function. Not implemented
- [ ] sheetGrabberVisible: Boolean indicating whether the sheet shows a grabber at the top. Attributes of the formsheet function. Not implemented
- [ ] sheetCornerRadius: The corner radius that the sheet will try to render with. Attributes of the formsheet function. Not implemented
- [ ] sheetExpandsWhenScrolledToEdge: Whether the sheet should expand to larger detent when scrolling. Attributes of the formsheet function. Not implemented
- [ ] onSheetDetentChanged: A callback that gets called when the current screen is in `formSheet` presentation and its detent has changed. Callback for formsheet function. Not implemented
- [ ] scrollEdgeEffects, BottomTabs current architecture is not supported. Not implemented
- [ ] ScreenContentWrapper depends on formSheet. As the dependent formSheet functionality is not yet implemented, this feature is not implemented.

## Others

None

## Directory Structure

````
/rntpc_react-native-screens  # Project root
├── react-native-harmony-screens  # HarmonyOS adapted npm package
│   ├── harmony                   # HarmonyOS native code
│   │   └── screens.har           # har package
│   ├── src                       # RN side TS source
│   │   ├── components             # RN components (Screen, ScreenContainer, ScreenStack, etc.)
│   │   ├── native-stack/views     # native-stack related views
│   │   ├── specs                  # Codegen generated native component specs
│   │   ├── core.ts                # Core logic
│   │   ├── index.ts               # Entry file
│   │   └── utils.ts               # Utility functions
│   ├── scripts                   # Build/publish scripts
│   ├── package.json
│   └── tsconfig.json
├── tester                        # Test project
│   ├── harmony                   # HarmonyOS test project
│   │   ├── entry                  # App entry module
│   │   └── screens                # HarmonyOS adapted source (ArkTS + CPP)
│   │       └── src/main
│   │           ├── cpp            # C++ side (Package, JSIBinder, TurboModule)
│   │           └── ets            # ArkTS side (components, animations, Package)
│   ├── apps/src                  # Test cases
│   ├── android                   # Android test project
│   └── package.json
├── README.md                     # Chinese installation and usage guide
├── README_en.md                  # English installation and usage guide
└── LICENSE                       # Open source license
````

## Contributing

If you encounter any issues during use, feel free to submit an [Issue](https://gitcode.com/CPF-RN/rntpc_react-native-screens/issues). PRs are also very welcome at [PR](https://gitcode.com/CPF-RN/rntpc_react-native-screens/pulls).

## License

This project is licensed under [The MIT License (MIT)](https://github.com/software-mansion/react-native-screens/blob/main/LICENSE). Feel free to enjoy and contribute to open source.

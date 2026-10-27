> Template Version: v0.4.2

<p align="center">
  <h1 align="center"> <code>react-native-splash-screen</code> </h1>
</p>

This project is based on [react-native-splash-screen](https://github.com/crazycodeboy/react-native-splash-screen).

the repository of this third-party supports direct installation from npm. The new package name is `@react-native-ohos/react-native-splash-screen`. The version relationship is as follows:

| Name | Version(Npm Address) | Release Information | Supported RN Version | Supported Autolink | Compile API Version | Community Baseline Version | Source code address |
| ------------ | ------------ | ------------------------------ | ------------- | ------------- |------------------------ | ------------- | ------------- |
| @react-native-ohos/react-native-splash-screen | [~3.5.0](https://www.npmjs.com/package/@react-native-ohos/react-native-splash-screen)   | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-splash-screen/releases) | 0.82.* / 0.84.* | Yes | API12+ | 3.3.0 | [master](https://gitcode.com/CPF-RN/rntpc_react-native-splash-screen/tree/master) |
| @react-native-ohos/react-native-splash-screen | [~3.4.0](https://www.npmjs.com/package/@react-native-ohos/react-native-splash-screen)| [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-splash-screen/releases) | 0.77 | No | API12+ | 3.3.0 | [br_rnoh0.77](https://gitcode.com/CPF-RN/rntpc_react-native-splash-screen/tree/br_rnoh0.77)  |
| @react-native-ohos/react-native-splash-screen | [~3.3.2](https://www.npmjs.com/package/@react-native-ohos/react-native-splash-screen)    | [Gitcode Releases](https://gitcode.com/CPF-RN/rntpc_react-native-splash-screen/releases) | 0.72 | Yes | API12+ | 3.3.0 | [br_rnoh0.72](https://gitcode.com/CPF-RN/rntpc_react-native-splash-screen/tree/br_rnoh0.72)  |
| @react-native-oh-tpl/react-native-splash-screen | [<= 3.3.0-0.0.2@deprecated](https://www.npmjs.com/package/@react-native-oh-tpl/react-native-splash-screen)  | [Github Releases(deprecated)](https://github.com/react-native-oh-library/react-native-splash-screen/releases) | 0.72      | No | API12+ | 3.3.0 | [sig](https://github.com/react-native-oh-library/react-native-splash-screen/tree/sig) |

## Introduction

`react-native-splash-screen`  is a splash screen component library for React Native.<br/>
It provides the ability to display a splash screen before the app finishes loading, and hide it once ready.

## Installation

Go to the project directory and execute the following instruction:

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

The following code shows the basic use scenario of the repository:

> [!WARNING] The name of the imported repository remains unchanged.


##### **Create a launch page `SplashScreenPage.ets` in the entry/src/main/ets/pages directory**

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

##### **Add `pages/SplashScreenPage` in entry/src/resources/base/profile/main_pages.json**
```json
{
  "src": [
    "pages/SplashScreenPage"
  ]
}
```

##### **Open the `entry/src/main/ets/entryability/EntryAbility.ets` file and add the following code:**
```ts
import { RNAbility } from '@rnoh/react-native-openharmony';
import window from '@ohos.window';
import { SplashScreen } from '@react-native-ohos/react-native-splash-screen/src/main/ets/SplashScreen'

export default class EntryAbility extends RNAbility {
  onWindowStageCreate(windowStage: window.WindowStage): void {
    let startWindowIcon = $r('app.media.splashIcon');  // Splash screen image (replace with your app's launch image, usually in entry/src/main/resources/base/media)
    let startWindowBackground = "#FFFFFF"; // Splash screen background color
    let startPageUrl = 'pages/SplashScreenPage'; // Splash page route
    SplashScreen.show(this.context, windowStage, startWindowIcon, startWindowBackground, startPageUrl).then(() => {
      super.onWindowStageCreate(windowStage);
    })
  }
}
```

##### Close the launch screen at the appropriate time (e.g., after initialization is complete).
```tsx
import React, { Component } from 'react';
import SplashScreen from 'react-native-splash-screen';
export default class WelcomePage extends Component {
    componentDidMount() {
        SplashScreen.hide(); // close launch screen
    }
}
```

## Link

|                                      | Is supported autolink | Supported RN Version |
|--------------------------------------|-----------------------|----------------------|
| ~3.5.0                              |  Yes                  | 0.82/0.84          |

Using AutoLink need to be configured according to this document, Autolink Framework Guide Documentation: https://gitcode.com/CPF-RN/ohos_react_native/blob/master/docs/zh-cn/Autolinking.md

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
    "@rnoh/react-native-openharmony": "~0.84.2" // ohpm version
    // "@rnoh/react-native-openharmony" : "./react_native_openharmony.har" // a locally available HAR package
    // "@rnoh/react-native-openharmony" : "./react_native_openharmony" // source code directory
  }
}
```

### 2. Introducing Native Code

Currently, two methods are available:

- Use the HAR file.
- Directly link to the source code.

Method 1 (recommended): Use the HAR file.

> [!TIP] The HAR file is stored in the `harmony` directory in the installation path of the third-party library.

Open `entry/oh-package.json5` file and add the following dependencies:

```json
"dependencies": {
    "@react-native-ohos/react-native-splash-screen": "file:../../node_modules/@react-native-ohos/react-native-splash-screen/harmony/splash_screen.har"
  }
```

Click the `sync` button in the upper right corner.

Alternatively, run the following instruction on the terminal:

```bash
cd entry
ohpm install
```

Method 2: Directly link to the source code.

> [!TIP] For details, see [Directly Linking Source Code](https://gitcode.com/CPF-RN/usage-docs/blob/master/zh-cn/link-source-code.md).

### 3. Configuring CMakeLists and Introducing SplashScreenPackage

> If you are using version <= 3.3.0-0.0.2, please skip this chapter.

Open `entry/src/main/cpp/CMakeLists.txt` and add the following code:


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

Open `entry/src/main/cpp/PackageProvider.cpp` and add the following code:

```diff
#include "RNOH/PackageProvider.h"
+ #include "SplashScreenPackage.h"

using namespace rnoh;

std::vector<std::shared_ptr<Package>> PackageProvider::getPackages(Package::Context ctx) {
    return {
+        std::make_shared<SplashScreenPackage>(ctx)
}
```

### 4. Introducing  SplashScreenPackage to ArkTS

Open the `entry/src/main/ets/RNPackagesFactory.ts` file and add the following code:

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

### Running

Click the `sync` button in the upper right corner.

Alternatively, run the following instruction on the terminal:

```bash
cd entry
ohpm install
```

Then build and run the code.

The following code demonstrates the basic usage scenario of this library:

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

## Constraints

###  Compatibility

To use this repository, you need to use the correct React-Native and RNOH versions. In addition, you need to use DevEco Studio and the ROM on your phone.

Verified in the following versions.

1. RNOH: 0.82.7; SDK: HarmonyOS 6.0.1 Release SDK; IDE: DevEco Studio 6.0.1 Release; ROM:6.0.0.328 SP26;
2. RNOH: 0.84.1; SDK: HarmonyOS 6.0.1 Release SDK; IDE: DevEco Studio 6.0.1 Release; ROM:6.0.0.120 SP7;

## Interface Description

### API

> [!TIP] The **Platform** column indicates the platform where the properties are supported in the original third-party library.

> [!TIP] If the value of **OpenHarmony Support** is **yes**, it means that the OpenHarmony platform supports this property; **no** means the opposite; **partially** means some capabilities of this property are supported. The usage method is the same on different platforms and the effect is the same as that of iOS or Android.

### 1. JS-side API

| Name | Type | Parameter Type | Return Value | Required | Platform | OpenHarmony Platform Support | Description |
| ---- | ---- | -------------- | ------------ | -------- | -------- | ---------------------------- | ----------- |
| `show()` | function | / | / | No | iOS / Android | Yes | Display the splash screen |
| `hide()` | function | / | / | Yes | iOS / Android | Yes | Hide the splash screen |

### 2. HarmonyOS Native-side API

| Name | Type | Parameter Type | Return Value | Required | Platform | OpenHarmony Platform Support | Description |
| ---- | ---- | -------------- | ------------ | -------- | -------- | ---------------------------- | ----------- |
| `SplashScreen.show(abilityContext, windowStage, iconResource, backgroundColor, pageUrl)` | function | `UIAbilityContext`, `window.WindowStage`, `Resource`, `string`, `string` | `Promise<void>` | Yes | HarmonyOS | Yes | Create a fullscreen sub-window and display the splash screen |

**Parameter Description:**

| Name | Description | Type |
| ---- | ----------- | ---- |
| `abilityContext` | Application context, usually pass `this.context` | `UIAbilityContext` |
| `windowStage` | Window stage, only pass in `onWindowStageCreate` | `window.WindowStage` |
| `iconResource` | Splash screen image, e.g. `$r('app.media.splashIcon')` | `Resource` |
| `backgroundColor` | Splash screen background color, e.g. `"#FFFFFF"` | `string` |
| `pageUrl` | ArkUI page route loaded in the sub-window, e.g. `'pages/SplashScreenPage'` | `string` |

> [!WARNING] If `SplashScreen.show(...)` is not called on the Native layer, the JS-side `show()` will not work because `windowStage` and other configuration are missing. Recommended flow: **Native `show` → RN loading → JS `hide()`**; call JS `show()` / `hide()` again when you need to mask the screen again.

## Known Issues

None

## Others

- In iOS, the working principle of the show() method is as follows: In the entry application method, the home page of the App is first loaded, and then a while loop is used to keep the interface on the App's splash screen. During this time, the home page still loads asynchronously. Once the loading is complete, stopping the while loop will hide the splash screen and display the home page.
  In HarmonyOS, the home page is loaded by calling windowStage.loadContent in the entry onWindowStageCreate method. If a while loop is used at this point, the home page cannot be loaded asynchronously.

## Directory Structure

```
/rntpc_react-native-splash-screen  # Project root
├── harmony              # HarmonyOS adaptation code
│    ├─ splash_screen.har           # HAR package
│    └─ splash_screen               # Core HarmonyOS adaptation
│          ├─ index.ets             # Entry point for HarmonyOS adaptation
│          ├─ ts.ets                # Package export
│          └─ src/main
│              ├─ cpp/                    # C++ native module
│              └─ ets/
│                  ├─ SplashScreen.ts         # Core module implementation
│                  ├─ SplashScreenPackage.ets # Package registration
│                  ├─ SplashScreenView.ets    # Splash screen view component
│                  └─ Logger.ts               # Logger utility
├── src                  # React Native code
│    └─ RNSplashScreenModule.ts   # TurboModule implementation
├── index.js                      # Main entry file
├── index.d.ts                    # Type definitions
├── README_en.md           # English documentation
├── README.md              # Chinese documentation
```

## How to Contribute

If you find any problem when using the project, submit an [issue](https://gitcode.com/CPF-RN/rntpc_react-native-splash-screen/issues) or a [PR](https://gitcode.com/CPF-RN/rntpc_react-native-splash-screen/pulls).

## License

This project is licensed under [The MIT License (MIT)](https://github.com/crazycodeboy/react-native-splash-screen/blob/v3.3.0/LICENSE).

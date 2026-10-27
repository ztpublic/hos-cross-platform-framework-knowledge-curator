/*
 * Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file.
 */

import {RNAbility} from '@rnoh/react-native-openharmony';
import { AbilityConstant, Want } from '@kit.AbilityKit';
import { window } from '@kit.ArkUI';
import { SplashScreen } from '@react-native-ohos/react-native-splash-screen/src/main/ets/SplashScreen'


export default class EntryAbility extends RNAbility {

  onCreate(want: Want) {
    super.onCreate(want)
  }

  getPagePath() {
    return 'pages/Index';
  }
  onWindowStageCreate(windowStage: window.WindowStage): void {
    // 默认：RNT / 文档 Demo
    let startWindowIcon = $r('app.media.app_icon'); // 启动页图片（用户改成自己 App 的启动图片，一般在 entry/src/main/resources/base/media 文件夹中）
    let startWindowBackground = "#FFFFFF"; // 启动页背景色
    let startPageUrl = 'pages/SplashScreenPage'; // 启动页
    SplashScreen.show(this.context, windowStage, startWindowIcon, startWindowBackground, startPageUrl).then(() => {
      super.onWindowStageCreate(windowStage);
    })

    /*
    // 上游 examples：drawable-xxhdpi/launch_screen.png
    // 切上游 Demo 时：index.js 改为 import App from './App'，并解开下面配置
    let startWindowIcon = $r('app.media.launch_screen');
    let startWindowBackground = "#FFFFFF";
    let startPageUrl = 'pages/SplashScreenPage';
    SplashScreen.show(this.context, windowStage, startWindowIcon, startWindowBackground, startPageUrl).then(() => {
      super.onWindowStageCreate(windowStage);
    }).catch((err: Object) => {
      console.error('SplashScreen.show failed: ' + JSON.stringify(err));
      super.onWindowStageCreate(windowStage);
    })
    */
  }
}

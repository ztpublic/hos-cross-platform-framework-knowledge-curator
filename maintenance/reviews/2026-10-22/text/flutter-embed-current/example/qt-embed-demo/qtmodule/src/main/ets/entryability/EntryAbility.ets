/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import UIAbility from '@ohos.app.ability.UIAbility';
import Window from '@ohos.window'
import Want from '@ohos.app.ability.Want';
import qpa from 'libplugins_platforms_qopenharmony.so'
import hilog from '@ohos.hilog';
import { FlutterManager } from '@ohos/flutter_ohos';
import { EmbeddedBridgeNAPI } from 'embedded'

type handleAbilityClose = ()=>void

export default class EntryAbility extends UIAbility {
  tag = "QtForOpenHarmony";
  domain = 0xFF00;
  private launchApplication = "libqtmodule.so"
  private launchParams = ""
  private name: string = 'qt_entry_window'
  private closeAbility: handleAbilityClose | undefined = undefined

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam) {
    FlutterManager.getInstance().pushUIAbility(this);
    EmbeddedBridgeNAPI.getInstance();
    let name = want.parameters?.windowName as string;
    if (name != undefined && name !== '')
      this.name = name
    let newAbility = want.parameters?.newAbility as boolean;
    if (newAbility !== undefined)
      return;
  }

  newLocalStorage() {
    return new LocalStorage()
  }

  onDestroy() {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onDestroy');
  }

  onWindowStageCreate(windowStage: Window.WindowStage) {
    FlutterManager.getInstance().pushWindowStage(this, windowStage);
    // Main window is created, set main page for this ability
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onWindowStageCreate');
    let localStore = new LocalStorage();
    localStore.setOrCreate('idName', this.name);
    let w = windowStage.getMainWindowSync();
    let newAbility = this.launchWant.parameters?.newAbility as boolean;

    windowStage.loadContent('pages/Index', localStore).then(()=>{
      qpa.handleJsTopWindowCreated(this.name, this);
      if (newAbility !== undefined) {
        this.context.setMissionLabel(this.launchWant.parameters?.title as string);
        return;
      }
    })

    if (newAbility !== undefined)
      return;
    this.launchParamSet();
    qpa.startQtApplication(this);
  }

  async launchParamSet(){
    let params:string[] = [];
    if(this.launchWant.uri != undefined){
      params.push(this.launchWant.uri)
    }

    //NOTE：应用启动参数在此处添加
    this.launchParams = params.join(' ');
  }

  onWindowStageDestroy() {
    // Main window is destroyed, release UI related resources
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onWindowStageDestroy');
  }

  onForeground() {
    // Ability has brought to foreground
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onForeground');
  }

  onBackground() {
    // Ability has back to background
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onBackground');
  }

  /**
   * note：启动模式为singleton/specified才生效
   * @param want
   * @param launchParam
   */
  onNewWant(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    qpa.handleOnNewWant(want, launchParam);
  }

  /**
   * note：该接口需要配置PREPARE_APP_TERMINATE权限（module.json5）
   * @returns
   */
  onPrepareToTerminate() {
    //关闭窗体时通知qt 后面添加
    if (this.closeAbility != undefined) {
      this.closeAbility()
      return true;
    }
    //阻止ts程序退出，由qt程序确定是否退出
    return false;
  }
};

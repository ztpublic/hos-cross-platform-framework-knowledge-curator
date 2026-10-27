import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import QAbilityStage from '../qabilitystage/QAbilityStage';
import UIAbility from '@ohos.app.ability.UIAbility';
import Want from '@ohos.app.ability.Want';
import Window from '@ohos.window'
import hilog from '@ohos.hilog';
import abilityAccessCtrl, { PermissionRequestResult } from '@ohos.abilityAccessCtrl';
import { BusinessError } from '@ohos.base';
import qpa from 'libqohos.so';
import {LOG_DOMAIN, LOG_TAG} from '../common/QtAppConstants';

export default class QAbility extends UIAbility {

  private localStorage: LocalStorage | null = null;

  // P024b (clipboard read): ohos.permission.READ_PASTEBOARD is a user_grant
  // permission. It is declared in module.json5 (Qt template), but the Qt OHOS
  // QPA clipboard read path only CHECKS the grant status - it never requests
  // it. Without an explicit requestPermissionsFromUser() call the permission
  // stays ungranted and QClipboard reads silently return empty data
  // (qohosclipboardobject.cpp getPasteboardDataWithLazyFetch). Request it
  // once on first launch; the system shows a consent dialog and remembers
  // the answer for subsequent launches.
  private requestReadPasteboardPermission(): void {
    const atManager: abilityAccessCtrl.AtManager = abilityAccessCtrl.createAtManager();
    atManager.requestPermissionsFromUser(this.context, ['ohos.permission.READ_PASTEBOARD'])
      .then((result: PermissionRequestResult) => {
        hilog.info(LOG_DOMAIN, LOG_TAG,
          'P024b: READ_PASTEBOARD grant status: ' + result.authResults.join(','));
      })
      .catch((err: BusinessError) => {
        hilog.error(LOG_DOMAIN, LOG_TAG,
          'P024b: READ_PASTEBOARD request failed: ' + err.code + ' ' + err.message);
      });
  }

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam) {
    hilog.info(LOG_DOMAIN, LOG_TAG, 'QAbility::onCreate(): want.parameters: ' + JSON.stringify(want.parameters));
    QAbilityStage.initQtAppContextIfNeeded(this.context.getApplicationContext(), this.context.resourceDir);
    this.requestReadPasteboardPermission();
    qpa.handleAbilityOnCreate(this, want, launchParam);
  }

  onDestroy(): void | Promise<void> {
    hilog.info(LOG_DOMAIN, LOG_TAG, 'QAbility::onDestroy()');
    return qpa.handleAbilityOnDestroy(this);
  }

  onPrepareToTerminate(): boolean {
    hilog.info(LOG_DOMAIN, LOG_TAG, 'QAbility::onPrepareToTerminate()');
    return qpa.handleAbilityOnPrepareToTerminate(this);
  }

  onPrepareToTerminateAsync(): Promise<boolean> {
    hilog.info(LOG_DOMAIN, LOG_TAG, 'QAbility::onPrepareToTerminateAsync()');
    return qpa.handleAbilityOnPrepareToTerminateAsync(this);
  }

  onWindowStageCreate(windowStage: Window.WindowStage) {
    hilog.info(LOG_DOMAIN, LOG_TAG, 'QAbility::onWindowStageCreate(): this.launchWant.parameters: ' + JSON.stringify(this.launchWant.parameters));
    qpa.handleAbilityOnWindowStageCreate(this, windowStage);
  }

  onWindowStageRestore(windowStage: Window.WindowStage): void {
    qpa.handleAbilityOnWindowStageRestore(this, windowStage);
  }

  onNewWant(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    hilog.info(LOG_DOMAIN, LOG_TAG, 'QAbility::onNewWant(): want.parameters: ' + JSON.stringify(want.parameters));
    qpa.handleAbilityOnNewWant(this, want, launchParam);
  }

  onForeground() {
    hilog.info(LOG_DOMAIN, LOG_TAG, 'QAbility::onForeground()');
    qpa.handleAbilityOnForeground(this);
  }

  onBackground() {
    hilog.info(LOG_DOMAIN, LOG_TAG, 'QAbility::onBackground()');
    qpa.handleAbilityOnBackground(this);
  }

  onWindowStageDestroy() {
    hilog.info(LOG_DOMAIN, LOG_TAG, 'QAbility::onStageDestroy()');
    qpa.handleAbilityOnWindowStageDestroy(this);
  }

  onContinue(wantParam: Record<string, Object>): AbilityConstant.OnContinueResult | Promise<AbilityConstant.OnContinueResult> {
    return qpa.handleAbilityOnContinue(this, wantParam);
  }
}

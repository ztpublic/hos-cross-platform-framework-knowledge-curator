import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import QAbilityStage from '../qabilitystage/QAbilityStage';
import UIAbility from '@ohos.app.ability.UIAbility';
import Want from '@ohos.app.ability.Want';
import Window from '@ohos.window'
import hilog from '@ohos.hilog';
import qpa from 'libqohos.so';
import { abilityAccessCtrl, Permissions, common } from '@kit.AbilityKit';
import {LOG_DOMAIN, LOG_TAG} from '../common/QtAppConstants';

export default class QAbility extends UIAbility {

  private localStorage: LocalStorage | null = null;

  private static permissionsRequested: boolean = false;

  // Guard against OHOS calling onWindowStageCreate a SECOND time on the same
  // ability instance (scene recovery: when the app is backgrounded with no
  // visible windows, the system destroys the window stage and recreates it on
  // the next foreground). The Qt OHOS QPA's handleAbilityOnWindowStageCreate
  // is NOT idempotent (unlike handleAbilityOnWindowStageRestore) and aborts
  // with: "Got Ability without 'io.qt.private.abilityInstanceId' param and no
  // pending auto-started instance". Route the second call to
  // handleAbilityOnWindowStageRestore, which safely skips page reloading.
  private windowStageCreateHandled: boolean = false;

  // Request user_grant permissions (e.g. CUSTOM_SCREEN_CAPTURE for screenshot).
  // Must be called from UIAbility (has UI context) - AbilityStage lacks it (error 401).
  private async requestAppPermissions(): Promise<void> {
    if (QAbility.permissionsRequested) {
      return;
    }
    QAbility.permissionsRequested = true;
    const permissions: Permissions[] = [
      'ohos.permission.CUSTOM_SCREEN_CAPTURE',
    ];
    try {
      const atManager = abilityAccessCtrl.createAtManager();
      const context = this.context as common.UIAbilityContext;
      const result = await atManager.requestPermissionsFromUser(context, permissions);
      for (let i = 0; i < permissions.length; i++) {
        hilog.info(LOG_DOMAIN, LOG_TAG,
          `QAbility: permission ${permissions[i]} authResult=${result.authResults[i]}`);
      }
    } catch (err) {
      hilog.error(LOG_DOMAIN, LOG_TAG,
        `QAbility: requestPermissionsFromUser failed: ${JSON.stringify(err)}`);
    }
  }

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam) {
    hilog.info(LOG_DOMAIN, LOG_TAG, 'QAbility::onCreate(): want.parameters: ' + JSON.stringify(want.parameters));
    QAbilityStage.initQtAppContextIfNeeded(this.context.getApplicationContext(), this.context.resourceDir);
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
    if (this.windowStageCreateHandled) {
      // Scene recovery after window stage destruction: do NOT call
      // handleAbilityOnWindowStageCreate again (QPA fatal, see field comment).
      // handleAbilityOnWindowStageRestore checks the internal
      // createdOrRestored flag and skips reloading safely.
      hilog.warn(LOG_DOMAIN, LOG_TAG, 'QAbility::onWindowStageCreate(): repeated call (scene recovery), routing to handleAbilityOnWindowStageRestore');
      qpa.handleAbilityOnWindowStageRestore(this, windowStage);
      return;
    }
    this.windowStageCreateHandled = true;
    // Request user_grant permissions now that UI context exists,
    // then initialize the Qt window stage
    this.requestAppPermissions().then(() => {
      qpa.handleAbilityOnWindowStageCreate(this, windowStage);
    });
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

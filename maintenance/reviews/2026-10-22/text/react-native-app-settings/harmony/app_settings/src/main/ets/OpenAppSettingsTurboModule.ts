import { UITurboModule } from '@rnoh/react-native-openharmony/ts';
import { OpenAppSettings } from './generated/turboModules/OpenAppSettings';

export class OpenAppSettingsTurboModule extends UITurboModule implements OpenAppSettings.Spec {
  static readonly NAME = 'OpenAppSettings';

  async open(): Promise<void> {
    await this.ctx.uiAbilityContext.startAbility({
      bundleName: 'com.huawei.hmos.settings',
      abilityName: 'com.huawei.hmos.settings.MainAbility',
      uri: 'application_info_entry',
      parameters: { settingsParamBundleName: this.ctx.uiAbilityContext.abilityInfo.bundleName },
    });
  }
}

// 鸿蒙原生 SQLite Package：通过 getUITurboModuleFactoryByNameMap 把 'HarmonySqlite' 注册给 RN。
import { RNOHPackage } from '@rnoh/react-native-openharmony';
import type { UITurboModule, UITurboModuleContext } from '@rnoh/react-native-openharmony/ts';
import { HarmonySqliteTurboModule } from './HarmonySqliteTurboModule';

export class HarmonySqlitePackage extends RNOHPackage {
  getUITurboModuleFactoryByNameMap(): Map<string, (ctx: UITurboModuleContext) => UITurboModule | null> {
    return new Map([
      [HarmonySqliteTurboModule.NAME, (ctx: UITurboModuleContext) => new HarmonySqliteTurboModule(ctx)],
    ]);
  }
}

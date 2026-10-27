/**
 * ArkTS TurboModule 实现：react-native-keep-awake 的鸿蒙化（可发布形态，随库 HAR 分发）。
 *
 * 上游 Android 通过 WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON 实现常亮，
 * iOS 通过 UIApplication.idleTimerDisabled；鸿蒙对应系统能力为
 * window.Window.setWindowKeepScreenOn(boolean)。
 *
 * 模块名与 JS spec 中 TurboModuleRegistry.getEnforcing('KeepAwake') 保持一致。
 */
import { UITurboModule, UITurboModuleContext } from '@rnoh/react-native-openharmony';

export class KeepAwakeTurboModule extends UITurboModule {
  public static readonly NAME = 'KeepAwake';

  constructor(ctx: UITurboModuleContext) {
    super(ctx);
  }

  private setKeepScreenOn(on: boolean): void {
    this.ctx.rnInstance
      .getRNWindow()
      .then((win) => win.setWindowKeepScreenOn(on))
      .catch((e: Error) => {
        this.ctx.logger.error('KeepAwake', `setWindowKeepScreenOn(${on}) failed: ${e.message}`);
      });
  }

  activate(): void {
    this.setKeepScreenOn(true);
  }

  deactivate(): void {
    this.setKeepScreenOn(false);
  }
}

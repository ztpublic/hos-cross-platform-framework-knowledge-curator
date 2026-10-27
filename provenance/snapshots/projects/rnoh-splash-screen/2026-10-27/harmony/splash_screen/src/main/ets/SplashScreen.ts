// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.

import { TurboModule, TurboModuleContext } from '@rnoh/react-native-openharmony/ts';
import { TM } from './generated/ts'
import window from '@ohos.window';
import image from '@ohos.multimedia.image';
import Logger from './Logger';

export class SplashScreen extends TurboModule implements TM.SplashScreen.Spec {
  static NAME = "SplashScreen"

  // 启动图片。hide / 读图失败必须 release() 后再置空，仅把字段写成 undefined 不够
  public static startWindowIcon: image.PixelMap | undefined = undefined;

  // 追踪当前 ImageSource，供 hide 和读图失败路径统一 release，
  // 避免 createImageSource 返回值当局部变量丢掉后无法释放
  private static imageSource: image.ImageSource | undefined = undefined;

  // 子窗口。createSubWindow 成功后才赋值；失败路径会拆掉半成品窗
  private static splashWindow: window.Window | undefined = undefined;

  private static cachedAbilityContext: any;
  private static cachedWindowStage: window.WindowStage;
  private static cachedIconResource: any;
  private static cachedBackgroundColor: string;
  private static cachedPageUrl: string;

  // show / hide 共用这一条链，前序失败也继续往后走，避免后续操作永久 pending
  private static pendingOperation: Promise<void> = Promise.resolve();

  constructor(ctx: TurboModuleContext) {
    super(ctx)
  }

  /**
   * TurboModule 销毁入口，由 RN 框架在模块回收时调用。
   * 必须等到关窗和 release 完成；hide() 只是踢出 hideInternal，这里直接 await。
   * 不清 cached* 配置 —— Ability 还活着，RN 重建后 JS show() 仍要靠它们。
   * 异常只打日志，避免销毁路径再把错误甩给宿主。
   */
  async __onDestroy__(): Promise<void> {
    try {
      await SplashScreen.hideInternal();
    } catch (err) {
      Logger.error('SplashScreen.__onDestroy__() failed: ' + JSON.stringify(err));
    }
  }

  /**
   * JS 侧调用，使用 EntryAbility 中缓存的配置重新显示启动屏。
   * Spec 是 void；建窗失败会 reject，这里用 catch 接住，避免未处理 rejection。
   */
  public show() {
    SplashScreen.showWithCachedConfig().catch((err: Object) => {
      Logger.error('SplashScreen.show() failed: ' + JSON.stringify(err));
    });
  }

  private static async showWithCachedConfig(): Promise<void> {
    if (!SplashScreen.cachedWindowStage || !SplashScreen.cachedAbilityContext ||
      !SplashScreen.cachedIconResource) {
      const message = 'SplashScreen.show() called before native SplashScreen.show(...) configuration';
      Logger.error(message);
      throw new Error(message);
    }
    if (SplashScreen.splashWindow) {
      Logger.debug('SplashScreen window already exists, skip show.');
      return;
    }
    await SplashScreen.show(
      SplashScreen.cachedAbilityContext,
      SplashScreen.cachedWindowStage,
      SplashScreen.cachedIconResource,
      SplashScreen.cachedBackgroundColor,
      SplashScreen.cachedPageUrl,
    );
  }

  /**
   * 显示启动屏。必须等到子窗口 showWindow 成功才返回，
   * 否则 JS hide() 会打在空句柄上，随后窗口才出来就关不掉。
   */
  public static async show(abilityContext: any,
                           windowStage: window.WindowStage,
                           iconResource: any,
                           backgroundColor: string,
                           pageUrl: string): Promise<void> {
    SplashScreen.cachedAbilityContext = abilityContext;
    SplashScreen.cachedWindowStage = windowStage;
    SplashScreen.cachedIconResource = iconResource;
    SplashScreen.cachedBackgroundColor = backgroundColor;
    SplashScreen.cachedPageUrl = pageUrl;

    return SplashScreen.runExclusive<void>(async () => {
      if (SplashScreen.splashWindow) {
        Logger.debug('SplashScreen window already exists, skip show.');
        return;
      }

      try {
        const resourceMgr = abilityContext.resourceManager;
        const fileData = await resourceMgr.getMediaContent(iconResource);
        const buffer = fileData.buffer;
        // 反复 show 时先释放上一张图，避免旧 PixelMap 一直占 Native 内存
        SplashScreen.releaseImageResources();
        const imageSource = image.createImageSource(buffer);
        SplashScreen.imageSource = imageSource;
        const pixelMap = await imageSource.createPixelMap();
        SplashScreen.startWindowIcon = pixelMap;
        await SplashScreen.setupSplashWindow(windowStage, backgroundColor, pageUrl);
      } catch (err) {
        SplashScreen.releaseImageResources();
        throw SplashScreen.toError(err);
      }
    });
  }

  // 建窗 / 铺页 / 显示任一步失败，都拆掉半成品窗口，避免残留看不见的子窗
  private static async setupSplashWindow(windowStage: window.WindowStage,
                                         backgroundColor: string,
                                         pageUrl: string): Promise<void> {
    let splashWindow: window.Window | undefined = undefined;
    try {
      splashWindow = await windowStage.createSubWindow('SplashScreenWindow');
      SplashScreen.splashWindow = splashWindow;
      try {
        splashWindow.setFullScreen(true);
      } catch (fullScreenErr) {
        Logger.error('Failed to set full screen. Cause: ' + JSON.stringify(fullScreenErr));
        throw SplashScreen.toError(fullScreenErr);
      }
      await splashWindow.setUIContent(pageUrl);
      try {
        splashWindow.setWindowBackgroundColor(backgroundColor);
      } catch (bgErr) {
        Logger.error('Failed to set window background color. Cause: ' + JSON.stringify(bgErr));
      }
      await splashWindow.showWindow();
      Logger.debug('Succeeded in showing the window.');
    } catch (err) {
      Logger.error('Failed to setup the splash window. Cause: ' + JSON.stringify(err));
      if (splashWindow) {
        await SplashScreen.destroyWindowSafe(splashWindow);
      } else {
        SplashScreen.splashWindow = undefined;
      }
      throw SplashScreen.toError(err);
    }
  }

  /**
   * 关闭启动屏
   */
  public hide() {
    SplashScreen.hide();
  }

  /**
   * Spec 是 void。真正关窗在 hideInternal 里排队等待，失败只打日志。
   */
  public static hide() {
    SplashScreen.hideInternal().catch((err: Object) => {
      Logger.error('SplashScreen.hide() failed: ' + JSON.stringify(err));
    });
  }

  private static async hideInternal(): Promise<void> {
    await SplashScreen.runExclusive<void>(async () => {
      try {
        await SplashScreen.destroySplashWindowAsync();
      } finally {
        SplashScreen.releaseImageResources();
      }
    });
  }

  /**
   * PixelMap / ImageSource 在 Native 堆上，字段置空不够，必须调 release()。
   * 句柄先置空，界面不能再画已释放的图。两段分开调，避免一张失败导致另一张没机会释放。
   */
  private static releaseImageResources(): void {
    const pixelMap = SplashScreen.startWindowIcon;
    SplashScreen.startWindowIcon = undefined;
    SplashScreen.releasePixelMap(pixelMap);

    const imageSource = SplashScreen.imageSource;
    SplashScreen.imageSource = undefined;
    SplashScreen.releaseImageSource(imageSource);
  }

  // 用带 callback 的 release，避免无参 overload 返回 Promise 成为未处理 rejection
  private static releasePixelMap(pixelMap: image.PixelMap | undefined): void {
    if (!pixelMap) {
      return;
    }
    try {
      pixelMap.release((err) => {
        SplashScreen.logReleaseError('startWindowIcon', err);
      });
    } catch (err) {
      SplashScreen.logReleaseError('startWindowIcon', err);
    }
  }

  private static releaseImageSource(imageSource: image.ImageSource | undefined): void {
    if (!imageSource) {
      return;
    }
    try {
      imageSource.release((err) => {
        SplashScreen.logReleaseError('imageSource', err);
      });
    } catch (err) {
      SplashScreen.logReleaseError('imageSource', err);
    }
  }

  private static logReleaseError(tag: string, err: Object | undefined): void {
    if (!err) {
      return;
    }
    Logger.error('Failed to release ' + tag + '. Cause: ' + JSON.stringify(err));
  }

  private static async destroySplashWindowAsync(): Promise<void> {
    const win = SplashScreen.splashWindow;
    if (!win) {
      return;
    }
    try {
      await win.destroyWindow();
    } finally {
      if (SplashScreen.splashWindow === win) {
        SplashScreen.splashWindow = undefined;
      }
    }
  }

  private static async destroyWindowSafe(win: window.Window): Promise<void> {
    try {
      await win.destroyWindow();
    } catch (err) {
      Logger.error('Failed to destroy window on error path: ' + JSON.stringify(err));
    } finally {
      if (SplashScreen.splashWindow === win) {
        SplashScreen.splashWindow = undefined;
      }
    }
  }

  // 前序任务失败也把链往前推，避免后续 show/hide 一直挂着
  private static runExclusive<T>(operation: () => Promise<T>): Promise<T> {
    const result = SplashScreen.pendingOperation.then(() => {
      return operation();
    });
    SplashScreen.pendingOperation = result.then(() => {
      return;
    }, () => {
      return;
    });
    return result;
  }

  private static toError(err: Object): Error {
    if (err instanceof Error) {
      return err;
    }
    return new Error(String(err));
  }
}

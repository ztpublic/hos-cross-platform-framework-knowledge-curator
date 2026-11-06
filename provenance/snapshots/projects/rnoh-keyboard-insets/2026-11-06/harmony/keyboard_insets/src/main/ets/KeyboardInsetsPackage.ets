import {
  EtsUITurboModule,
  RNOHPackage,
  type UITurboModule,
  type UITurboModuleContext,
} from '@rnoh/react-native-openharmony';

import type { AnimatorResult } from '@ohos.animator';
import { KeyboardAvoidMode, type UIContext } from '@ohos.arkui.UIContext';
import deviceInfo from '@ohos.deviceInfo';
import window from '@ohos.window';

interface KeyboardAnimation {
  duration: number;
  easing: string;
}

class KeyboardInsetsObserver extends EtsUITurboModule {
  private windowInstance?: window.Window;
  private uiContext?: UIContext;
  private keyboardHeightChange?: (height: number) => void;
  private avoidAreaChange?: (options: window.AvoidAreaOptions) => void;
  private keyboardWillShow?: (info: window.KeyboardInfo) => void;
  private keyboardWillHide?: (info: window.KeyboardInfo) => void;
  private keyboardDidShow?: (info: window.KeyboardInfo) => void;
  private keyboardDidHide?: (info: window.KeyboardInfo) => void;
  private animator?: AnimatorResult;
  private destroyed = false;
  private hasEmittedState = false;
  private nativeAnimationActive = false;
  private currentPositionPx = 0;
  private stableKeyboardHeightPx = 0;
  private lastNonZeroKeyboardHeightPx = 0;
  private targetPositionPx = 0;
  private targetHeightPx = 0;
  private targetShown = false;
  private currentShown = false;
  private transitioning = false;
  private transitionGeneration = 0;

  constructor(ctx: UITurboModuleContext) {
    super(ctx);
    this.initialize().catch((error: Error) => {
      console.error(`[KeyboardInsetsObserver] initialize failed: ${String(error)}`);
    });
  }

  private async initialize() {
    const rnWindow = await this.ctx.rnInstance.getRNWindow();
    if (this.destroyed) {
      return;
    }
    this.windowInstance = rnWindow;
    this.uiContext = rnWindow.getUIContext();

    this.keyboardHeightChange = (keyboardHeight: number) => {
      this.handleKeyboardHeightChange(keyboardHeight);
    };
    rnWindow.on('keyboardHeightChange', this.keyboardHeightChange);

    this.avoidAreaChange = (options: window.AvoidAreaOptions) => {
      if (options.type === window.AvoidAreaType.TYPE_KEYBOARD) {
        this.handleKeyboardPositionChange(options.area.bottomRect.height);
      }
    };
    try {
      rnWindow.on('avoidAreaChange', this.avoidAreaChange);
    } catch (error) {
      console.warn(`[KeyboardInsetsObserver] avoidAreaChange unavailable: ${String(error)}`);
    }

    this.registerKeyboardAnimationEvents(rnWindow);

    const initialArea = rnWindow.getWindowAvoidArea(window.AvoidAreaType.TYPE_KEYBOARD);
    this.publishStableState(
      initialArea.bottomRect.height,
      initialArea.bottomRect.height > 0,
    );
  }

  private handleKeyboardHeightChange(keyboardHeight: number) {
    this.handleKeyboardPositionChange(keyboardHeight);
  }

  private handleKeyboardPositionChange(rawPositionPx: number) {
    if (this.nativeAnimationActive) {
      return;
    }
    const positionPx = Math.max(rawPositionPx, 0);
    this.publishStableState(positionPx, positionPx > 0);
  }

  private registerKeyboardAnimationEvents(rnWindow: window.Window) {
    if (
      deviceInfo.sdkApiVersion >= 18 &&
      canIUse('SystemCapability.Window.SessionManager')
    ) {
      this.keyboardDidShow = (info: window.KeyboardInfo) => {
        const currentHeightPx = this.readKeyboardAreaHeightPx();
        const finalHeightPx = Math.max(info.endRect.height, 0);
        this.publishStableState(
          finalHeightPx > 0 ? finalHeightPx : currentHeightPx,
          true,
        );
      };
      this.keyboardDidHide = () => {
        this.publishStableState(0, false);
      };
      try {
        rnWindow.on('keyboardDidShow', this.keyboardDidShow);
        rnWindow.on('keyboardDidHide', this.keyboardDidHide);
      } catch (error) {
        console.warn(`[KeyboardInsetsObserver] keyboardDid events unavailable: ${String(error)}`);
      }
    }

    if (
      deviceInfo.sdkApiVersion < 20 ||
      !canIUse('SystemCapability.Window.SessionManager')
    ) {
      return;
    }

    this.keyboardWillShow = (info: window.KeyboardInfo) => {
      this.handleKeyboardWillChange(info, true);
    };
    this.keyboardWillHide = (info: window.KeyboardInfo) => {
      this.handleKeyboardWillChange(info, false);
    };

    let showRegistered = false;
    try {
      rnWindow.on('keyboardWillShow', this.keyboardWillShow);
      showRegistered = true;
      rnWindow.on('keyboardWillHide', this.keyboardWillHide);
    } catch (error) {
      if (showRegistered && this.keyboardWillShow !== undefined) {
        try {
          rnWindow.off('keyboardWillShow', this.keyboardWillShow);
        } catch (_) {
          // The platform may reject unregistering an unsupported event too.
        }
      }
      this.keyboardWillShow = undefined;
      this.keyboardWillHide = undefined;
      console.warn(`[KeyboardInsetsObserver] keyboardWill events unavailable: ${String(error)}`);
    }
  }

  private handleKeyboardWillChange(info: window.KeyboardInfo, shown: boolean) {
    const fallbackPositionPx = shown ? this.readKeyboardAreaHeightPx() : 0;
    const eventPositionPx = shown ? Math.max(info.endRect.height, 0) : 0;
    const targetPositionPx = shown
      ? (eventPositionPx > 0 ? eventPositionPx : fallbackPositionPx)
      : 0;
    const targetHeightPx = shown ? targetPositionPx : this.getHideMetadataHeightPx();

    if (info.animated === false) {
      this.publishStableState(targetPositionPx, shown);
      return;
    }

    const animation = this.getKeyboardAnimation(info.config);
    if (animation === undefined) {
      // Without system timing metadata, follow the height callbacks directly.
      this.nativeAnimationActive = false;
      return;
    }

    this.nativeAnimationActive = true;
    this.startTransition(targetPositionPx, targetHeightPx, shown, animation);
  }

  private getKeyboardAnimation(
    config?: window.WindowAnimationConfig,
  ): KeyboardAnimation | undefined {
    if (config === undefined) {
      return undefined;
    }
    const duration = config.duration;
    if (
      duration === undefined ||
      !Number.isFinite(duration) ||
      duration <= 0
    ) {
      return undefined;
    }

    let easing: string;
    if (config.curve === window.WindowAnimationCurve.LINEAR) {
      easing = 'linear';
    } else if (config.curve === window.WindowAnimationCurve.CUBIC_BEZIER) {
      const params = config.param;
      if (
        params !== undefined &&
        params.length === 4 &&
        params.every((value: number) => Number.isFinite(value) && value >= 0 && value <= 1)
      ) {
        easing = `cubic-bezier(${params.join(', ')})`;
      } else {
        return undefined;
      }
    } else if (config.curve === window.WindowAnimationCurve.INTERPOLATION_SPRING) {
      const params = config.param;
      if (
        params !== undefined &&
        params.length === 4 &&
        params.every((value: number) => Number.isFinite(value))
      ) {
        easing = `interpolating-spring(${params.join(', ')})`;
      } else {
        return undefined;
      }
    } else {
      return undefined;
    }

    return { duration, easing };
  }

  private readKeyboardAreaHeightPx(): number {
    const rnWindow = this.windowInstance;
    if (rnWindow === undefined) {
      return 0;
    }
    try {
      return Math.max(
        rnWindow.getWindowAvoidArea(window.AvoidAreaType.TYPE_KEYBOARD).bottomRect.height,
        0,
      );
    } catch (error) {
      console.warn(`[KeyboardInsetsObserver] keyboard avoid area read failed: ${String(error)}`);
      return 0;
    }
  }

  private getHideMetadataHeightPx(): number {
    if (this.transitioning && this.targetShown && this.targetHeightPx > 0) {
      return this.targetHeightPx;
    }
    if (this.stableKeyboardHeightPx > 0) {
      return this.stableKeyboardHeightPx;
    }
    return this.lastNonZeroKeyboardHeightPx;
  }

  private publishStableState(
    rawPositionPx: number,
    shown: boolean,
  ) {
    const positionPx = shown ? Math.max(rawPositionPx, 0) : 0;
    const heightPx = shown ? positionPx : this.stableKeyboardHeightPx;
    if (
      this.hasEmittedState &&
      !this.transitioning &&
      positionPx === this.currentPositionPx &&
      shown === this.currentShown &&
      heightPx === this.targetHeightPx
    ) {
      return;
    }
    this.transitionGeneration += 1;
    this.cancelAnimator();
    this.nativeAnimationActive = false;
    if (shown) {
      this.stableKeyboardHeightPx = positionPx;
      this.lastNonZeroKeyboardHeightPx = positionPx;
    }
    this.currentPositionPx = positionPx;
    this.currentShown = shown;
    this.targetPositionPx = positionPx;
    this.targetHeightPx = heightPx;
    this.targetShown = shown;
    this.transitioning = false;
    this.postKeyboardState(positionPx, this.targetHeightPx, shown, false);
  }

  private postKeyboardState(
    positionPx: number,
    heightPx: number,
    shown: boolean,
    transitioning: boolean,
  ) {
    this.hasEmittedState = true;
    this.ctx.rnInstance.postMessageToCpp('KeyboardInsets::KeyboardChanged', {
      position: px2vp(positionPx),
      height: px2vp(heightPx),
      shown,
      transitioning,
      systemAvoidsKeyboard:
        this.uiContext?.getKeyboardAvoidMode() !== KeyboardAvoidMode.NONE,
    });
  }

  private isCurrentTransition(generation: number): boolean {
    return !this.destroyed && this.transitioning && generation === this.transitionGeneration;
  }

  private startTransition(
    targetPositionPx: number,
    targetHeightPx: number,
    targetShown: boolean,
    animation: KeyboardAnimation,
  ) {
    if (this.destroyed) {
      return;
    }

    if (
      this.transitioning &&
      targetPositionPx === this.targetPositionPx &&
      targetHeightPx === this.targetHeightPx &&
      targetShown === this.targetShown
    ) {
      return;
    }
    if (
      !this.transitioning &&
      targetPositionPx === this.currentPositionPx &&
      targetShown === this.currentShown
    ) {
      this.nativeAnimationActive = false;
      return;
    }

    this.transitionGeneration += 1;
    const generation = this.transitionGeneration;
    this.cancelAnimator();

    const beginPositionPx = this.currentPositionPx;
    this.targetPositionPx = targetPositionPx;
    this.targetHeightPx = targetHeightPx;
    this.targetShown = targetShown;
    this.currentShown = targetShown;
    this.transitioning = true;

    // Establish the status boundary before the first native position frame.
    this.postKeyboardState(
      beginPositionPx,
      targetHeightPx,
      targetShown,
      true,
    );

    if (beginPositionPx === targetPositionPx) {
      this.finishTransition(generation);
      return;
    }

    const uiContext = this.uiContext;
    if (uiContext === undefined) {
      console.error('[KeyboardInsetsObserver] animator creation skipped: UI context unavailable');
      this.finishTransition(generation);
      return;
    }

    let animator: AnimatorResult;
    try {
      animator = uiContext.createAnimator({
        duration: animation.duration,
        easing: animation.easing,
        delay: 0,
        fill: 'forwards',
        direction: 'normal',
        iterations: 1,
        begin: beginPositionPx,
        end: targetPositionPx,
      });
    } catch (error) {
      console.error(`[KeyboardInsetsObserver] animator creation failed: ${String(error)}`);
      this.finishTransition(generation);
      return;
    }

    this.animator = animator;
    animator.onFrame = (value: number) => {
      if (!this.isCurrentTransition(generation)) {
        return;
      }
      const positionPx = Math.max(value, 0);
      this.currentPositionPx = positionPx;
      this.postKeyboardState(
        positionPx,
        targetHeightPx,
        targetShown,
        true,
      );
    };
    animator.onFinish = () => {
      this.finishTransition(generation);
    };
    animator.onCancel = () => {
      if (!this.isCurrentTransition(generation)) {
        return;
      }
      this.animator = undefined;
      this.transitioning = false;
      this.nativeAnimationActive = false;
      this.postKeyboardState(
        this.currentPositionPx,
        targetHeightPx,
        targetShown,
        false,
      );
    };

    try {
      animator.play();
    } catch (error) {
      console.error(`[KeyboardInsetsObserver] animator play failed: ${String(error)}`);
      this.animator = undefined;
      this.finishTransition(generation);
    }
  }

  private finishTransition(generation: number) {
    if (!this.isCurrentTransition(generation)) {
      return;
    }

    const targetPositionPx = this.targetPositionPx;
    const targetHeightPx = this.targetHeightPx;
    const targetShown = this.targetShown;
    this.currentPositionPx = targetPositionPx;
    this.currentShown = targetShown;
    if (targetShown) {
      this.stableKeyboardHeightPx = targetHeightPx;
      this.lastNonZeroKeyboardHeightPx = targetHeightPx;
    }
    this.animator = undefined;
    this.transitioning = false;
    this.nativeAnimationActive = false;
    this.postKeyboardState(
      targetPositionPx,
      targetHeightPx,
      targetShown,
      false,
    );
  }

  private cancelAnimator() {
    const oldAnimator = this.animator;
    this.animator = undefined;
    if (oldAnimator === undefined) {
      return;
    }

    // cancel() may synchronously invoke callbacks or emit its terminal frame.
    // Detach them before canceling; transitionGeneration already invalidated
    // this animator before this method is called.
    oldAnimator.onFrame = () => {};
    oldAnimator.onFinish = () => {};
    oldAnimator.onCancel = () => {};
    try {
      oldAnimator.cancel();
    } catch (error) {
      console.error(`[KeyboardInsetsObserver] animator cancellation failed: ${String(error)}`);
    }
  }

  override __onDestroy__() {
    this.destroyed = true;
    this.transitionGeneration += 1;
    this.cancelAnimator();
    const rnWindow = this.windowInstance;
    if (rnWindow !== undefined) {
      if (this.keyboardHeightChange !== undefined) {
        rnWindow.off('keyboardHeightChange', this.keyboardHeightChange);
      }
      if (this.avoidAreaChange !== undefined) {
        try {
          rnWindow.off('avoidAreaChange', this.avoidAreaChange);
        } catch (_) {
          // The listener may not have been available on this platform.
        }
      }
      if (this.keyboardWillShow !== undefined) {
        try {
          rnWindow.off('keyboardWillShow', this.keyboardWillShow);
        } catch (_) {
          // API 12–19 do not expose this API.
        }
      }
      if (this.keyboardWillHide !== undefined) {
        try {
          rnWindow.off('keyboardWillHide', this.keyboardWillHide);
        } catch (_) {
          // API 12–19 do not expose this API.
        }
      }
      if (this.keyboardDidShow !== undefined) {
        try {
          rnWindow.off('keyboardDidShow', this.keyboardDidShow);
        } catch (_) {
          // API 12–17 do not expose this API.
        }
      }
      if (this.keyboardDidHide !== undefined) {
        try {
          rnWindow.off('keyboardDidHide', this.keyboardDidHide);
        } catch (_) {
          // API 12–17 do not expose this API.
        }
      }
    }
    this.windowInstance = undefined;
    this.uiContext = undefined;
    this.keyboardHeightChange = undefined;
    this.avoidAreaChange = undefined;
    this.keyboardWillShow = undefined;
    this.keyboardWillHide = undefined;
    this.keyboardDidShow = undefined;
    this.keyboardDidHide = undefined;
    super.__onDestroy__();
  }
}

/**
 * Registers the Harmony component package and the keyboard observer module.
 */
export class KeyboardInsetsPackage extends RNOHPackage {
  override async createEagerUITurboModuleByNameMap(
    ctx: UITurboModuleContext,
  ): Promise<Map<string, UITurboModule>> {
    return new Map([['KeyboardInsetsObserver', new KeyboardInsetsObserver(ctx)]]);
  }
}

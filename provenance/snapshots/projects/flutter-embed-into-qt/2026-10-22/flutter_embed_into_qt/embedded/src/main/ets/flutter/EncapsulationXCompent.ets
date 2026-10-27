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
import { ComponentContent, UIContext, window } from '@kit.ArkUI';
import { MixedModuleResult } from '../../cpp/types/libembeddedbridge/Index';
import { FlutterPage, FlutterPlugin, FlutterView, FlutterWindow, Log } from '@ohos/flutter_ohos';
import { FlutterWindowManager } from './FlutterWIndowManager';
import { MyFlutterWindow } from './MyFlutterWindow';

// 使用 @Builder 装饰器定义混合嵌入式组件
@Builder
function mixedXComponent(array: Array<string>) {
  if (array && array.length == 2) {
    FlutterComponent({ engineId: array[0], initialRoute: array[1] })
  }
}

const TAG = 'FlutterComponent';
const DEFAULT_INITIAL_ROUTE = '/';

@Component
export struct FlutterComponent {
  @Prop engineId: string = '';
  @Prop initialRoute: string = DEFAULT_INITIAL_ROUTE;
  flutterWindow?: FlutterWindow;
  flutterView?: FlutterView;
  private flutterPlugin?: FlutterPlugin;
  @LocalStorageLink('viewId') viewId: string = "";

  aboutToAppear(): void {
    let record: Record<string, Object> = {
      'dart_entrypoint_args': [this.engineId],
      'route': this.initialRoute,
    };
    this.flutterWindow = new MyFlutterWindow(this.engineId, getContext(this), record);
    this.flutterWindow.aboutToInit();
    this.flutterView = this.flutterWindow.getFlutterView();
    FlutterWindowManager.getInstance().addFlutterWindow(this.engineId, this.flutterWindow);
  }

  aboutToDisappear(): void {
    Log.d(TAG, 'aboutToDisappear()')
  }

  onPageShow(): void {
    this.flutterWindow?.onPageShow();
  }

  onPageHide(): void {
    this.flutterWindow?.onPageHide();
  }

  build() {
    FlutterPage({
      viewId: this.flutterView?.getId(),
      defaultFocusOnTouch: true,
      onFocusListener: () => {
        Log.d(TAG, 'FlutterPage,onFocus, get focus')
        this.flutterWindow?.postWindowStageEvent(window.WindowStageEventType.ACTIVE);
      },
      onBlurListener: () => {
        Log.d(TAG, 'FlutterPage,onBlur, lose focus')
        this.flutterWindow?.postWindowStageEvent(window.WindowStageEventType.INACTIVE);
      }
    })
  }
}

// 定义创建函数的返回值类型，用于ArkTS和Native交互

// 提供创建ArkTS组件的入口函数
export function createMixedXComponent(engineId: string, initialRoute: string): MixedModuleResult {
  Log.d('testLog', 'Process：create node');
  Log.d(TAG, `createMixedXComponent,engineId=${engineId},initialRoute=${initialRoute}`);
  let uiContent = AppStorage.get<UIContext>("context"); // 从AppStorage中获取UI上下文
  let content = new ComponentContent<Array<string>>(
    uiContent!,
    wrapBuilder<[Array<string>]>(mixedXComponent),
    [engineId, initialRoute]
  );
  return { content: content }; // 返回包含组件内容的对象
}

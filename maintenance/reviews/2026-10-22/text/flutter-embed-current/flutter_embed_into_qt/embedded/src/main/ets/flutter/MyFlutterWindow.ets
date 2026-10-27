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
import { FlutterEngine, FlutterWindow } from "@ohos/flutter_ohos";
import { GeneratedPluginRegistrant } from "../plugins/GeneratedPluginRegistrant";
import MessagePlugin from "../plugins/MessagePlugin";
import { MessagePluginManager } from "../plugins/MessagePluginManager";


export class MyFlutterWindow extends FlutterWindow {
  private engineId: string;

  constructor(engineId: string, context: Context, params: Record<string, Object> = {}) {
    super(context, params);
    this.engineId = engineId;
  }

  configureFlutterEngine(flutterEngine: FlutterEngine): void {
    super.configureFlutterEngine(flutterEngine);
    GeneratedPluginRegistrant.registerWith(flutterEngine);
    const plugin = new MessagePlugin(this.engineId);
    MessagePluginManager.getInstance().setPlugin(this.engineId, plugin);
    this.delegate?.addPlugin(plugin);
  }

  protected isWindowStageEventCallbackHandle(): boolean {
    return false;
  }
}
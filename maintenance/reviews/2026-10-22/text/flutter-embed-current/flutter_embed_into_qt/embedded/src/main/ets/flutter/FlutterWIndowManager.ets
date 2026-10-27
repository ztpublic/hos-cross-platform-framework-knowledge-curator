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
import { FlutterWindow } from "@ohos/flutter_ohos";
import { HashMap } from "@kit.ArkTS";


export class FlutterWindowManager {
  static manager: FlutterWindowManager;
  map: HashMap<string, FlutterWindow> = new HashMap();

  static getInstance(): FlutterWindowManager {
    if (FlutterWindowManager.manager == undefined) {
      FlutterWindowManager.manager = new FlutterWindowManager();
    }
    return FlutterWindowManager.manager;
  }

  addFlutterWindow(engineId: string, flutterWindow?: FlutterWindow) {
    this.map.set(engineId, flutterWindow);
  }

  getFlutterWindow(engineId: string): FlutterWindow | undefined {
    return this.map.get(engineId);
  }

  removeFlutterWindow(engineId: string) {
    if (this.map.hasKey(engineId)) {
      this.map.remove(engineId);
    }
  }
}
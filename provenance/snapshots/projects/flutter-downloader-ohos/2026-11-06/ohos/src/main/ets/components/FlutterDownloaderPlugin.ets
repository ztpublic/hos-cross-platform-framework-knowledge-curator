/*
* Copyright (c) 2024 SwanLink (Jiangsu) Technology Development Co., LTD.
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

import {
  AbilityAware,
  AbilityPluginBinding,
  Any,
  DartExecutor,
  FlutterCallbackInformation,
  FlutterEngine,
  FlutterInjector,
  FlutterPlugin,
  FlutterPluginBinding,
  MethodCall,
  MethodCallHandler
} from '@ohos/flutter_ohos';
import MethodChannel, { MethodResult } from '@ohos/flutter_ohos/src/main/ets/plugin/common/MethodChannel';
import preferences from '@ohos.data.preferences';
import fs from '@ohos.file.fs';
import { TaskDao } from './TaskDao';
import { TaskDbHelper } from './TaskDbHelper';
import { DownloadStatus } from './DownloadStatus';
import { ValuesBucket } from '@kit.ArkData';
import { TaskEntry } from './TaskEntry';
import { fileUri, picker } from '@kit.CoreFileKit';
import { common, Want, wantConstant } from '@kit.AbilityKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { request } from '@kit.BasicServicesKit';
import Logger from './Logger';
import { DartCallback } from '@ohos/flutter_ohos/src/main/ets/embedding/engine/dart/DartExecutor';

const invalidTaskId = "invalid_task_id";
const invalidStatus = "invalid_status";
const invalidData = "invalid_data";
const TAG = 'FlutterDownloaderPlugin';
const CHANNEL = "vn.hunghd/downloader";

export default class FlutterDownloaderPlugin implements FlutterPlugin, MethodCallHandler, AbilityAware {
  public static readonly SHARED_PREFERENCES_KEY = 'vn.hunghd.downloader.pref';
  public static readonly CALLBACK_DISPATCHER_HANDLE_KEY = "callback_dispatcher_handle_key";
  private flutterChannel: MethodChannel | null = null;
  private backgroundChannel: MethodChannel | null = null;
  private flutterPluginBinding: FlutterPluginBinding | null = null;
  private taskDao: TaskDao | null = null;
  private context: common.UIAbilityContext | null = null;
  private callbackHandle: number = 0;
  private backgroundFlutterEngine: FlutterEngine | null = null;
  private debugMode: number = 0;
  private tasks: Set<string> = new Set();

  onAttachedToEngine(binding: FlutterPluginBinding): void {
    this.flutterPluginBinding = binding;
  }

  onDetachedFromEngine(binding: FlutterPluginBinding): void {
    this.onDetachedFromAbility();
    this.context = null
    this.flutterPluginBinding = null;
  }

  onAttachedToAbility(binding: AbilityPluginBinding): void {
    if (this.flutterChannel != null) {
      return;
    }
    this.context = binding.getAbility().context;
    this.flutterChannel =
      new MethodChannel(this.flutterPluginBinding!.getBinaryMessenger(), CHANNEL);
    this.flutterChannel.setMethodCallHandler(this);
    this.taskDao = new TaskDao(TaskDbHelper.getInstance(this.context))
  }

  onDetachedFromAbility(): void {
    if (this.flutterChannel != null) {
      this.flutterChannel.setMethodCallHandler(null);
      this.flutterChannel = null;
    }
    if (this.backgroundChannel != null) {
      this.backgroundChannel.setMethodCallHandler(null);
      this.backgroundChannel = null;
    }
  }

  getUniqueClassName(): string {
    return TAG;
  }

  onMethodCall(call: MethodCall, result: MethodResult): void {
    Logger.info(TAG, `method: ${call.method}`);
    switch (call.method) {
      case 'initialize':
        this.initialize(call, result);
        break;
      case 'registerCallback':
        this.registerCallback(call, result);
        break;
      case 'enqueue':
        this.enqueue(call, result);
        break;
      case 'loadTasks':
        this.loadTasks(call, result);
        break;
      case 'loadTasksWithRawQuery':
        this.loadTasksWithRawQuery(call, result);
        break;
      case 'cancel':
        this.cancel(call, result);
        break;
      case 'cancelAll':
        this.cancelAll(call, result);
        break;
      case 'pause':
        this.pause(call, result);
        break;
      case 'resume':
        this.resume(call, result);
        break;
      case 'retry':
        this.retry(call, result);
        break;
      case 'open':
        this.open(call, result);
        break;
      case 'remove':
        this.remove(call, result);
        break;
      default:
        result.notImplemented();
        break;
    }
  }

  private remove(call: MethodCall, result: MethodResult) {
    const taskId = this.requireArgument<string>(call, "task_id");
    const shouldDeleteContent = this.requireArgument<boolean>(call, "should_delete_content");
    const task = this.taskDao!.loadTask(taskId);
    if (task != null) {
      if (task.status == DownloadStatus.ENQUEUED || task.status == DownloadStatus.RUNNING) {
        request.agent.remove(taskId, (err: BusinessError) => {
          if (err) {
            Logger.error(TAG, `Failed to removing a download task, Code: ${err.code}, message: ${err.message}`);
            return;
          }
          Logger.info(TAG, `Succeeded in removing a download task.`);
        });
      }
      if (shouldDeleteContent) {
        const saveFilePath = task.savedDir + '/' + task.filename;
        if (fs.accessSync(saveFilePath)) {
          fs.unlink(saveFilePath).then(() => {
            Logger.info(TAG, "remove file succeed");
          }).catch((err: BusinessError) => {
            Logger.error(TAG, "remove file failed with error message: " + err.message + ", error code: " + err.code);
          });
        }
      }
      this.taskDao!.deleteTask(taskId)
      result.success(null);
    } else {
      result.error(invalidTaskId, "not found task corresponding to given task id", null);
    }
  }

  private open(call: MethodCall, result: MethodResult) {
    const taskId = this.requireArgument<string>(call, "task_id");
    const task = this.taskDao!.loadTask(taskId);
    if (task == null) {
      result.error(invalidTaskId, "not found task with id $taskId", null);
      return;
    }

    if (task.status != DownloadStatus.COMPLETE) {
      result.error(invalidStatus, "only completed tasks can be opened", null);
      return;
    }
    this.openFile(`${task.savedDir}/${task.filename}`, task.mimeType).then(openResult => {
      result.success(openResult);
    }).catch((e: BusinessError) => {
      result.success(false);
    });
  }

  private async retry(call: MethodCall, result: MethodResult): Promise<void> {
    const taskId = this.requireArgument<string>(call, "task_id");
    const task = this.taskDao!.loadTask(taskId);
    if (task != null) {
      if (task.status == DownloadStatus.FAILED || task.status == DownloadStatus.CANCELED) {
        const newTaskId =
          await this.downloadFile(task.url, task.headers, task.filename!, task.savedDir, task.allowCellular,
            task.showNotification, task.openFileFromNotification);
        result.success(newTaskId);
        this.sendUpdateProgress(newTaskId, DownloadStatus.ENQUEUED, task.progress);
        const values: ValuesBucket = {};
        values[TaskEntry.COLUMN_NAME_TASK_ID] = newTaskId;
        values[TaskEntry.COLUMN_NAME_STATUS] = DownloadStatus.ENQUEUED;
        values[TaskEntry.COLUMN_NAME_PROGRESS] = task.progress;
        values[TaskEntry.COLUMN_NAME_RESUMABLE] = 0;
        this.taskDao!.updateTask(taskId, values);
      } else {
        result.error(invalidStatus, "only failed and canceled task can be retried", null);
      }
    } else {
      result.error(invalidTaskId, "not found task corresponding to given task id", null);
    }
  }

  private resume(call: MethodCall, result: MethodResult) {
    const taskId = this.requireArgument<string>(call, "task_id");
    const task = this.taskDao!.loadTask(taskId);
    if (task != null) {
      if (task.status == DownloadStatus.PAUSED) {
        request.agent.getTask(this.context!, taskId).then((downloadTask: request.agent.Task) => {
          downloadTask.resume((err: BusinessError) => {
            if (err) {
              Logger.error(TAG, `Failed to resume the download task, Code: ${err.code}, message: ${err.message}`);
              result.error(
                invalidData,
                `Failed to resume the download task, Code: ${err.code}, message: ${err.message}`,
                null
              )
              return;
            }
            Logger.info(TAG, `Succeeded in resuming a download task. `);
            result.success(taskId);
          })
        })
      } else {
        Logger.error(TAG, "only paused task can be resumed");
        result.error(invalidStatus, "only paused task can be resumed", null);
      }
    } else {
      Logger.error(TAG, "not found task corresponding to given task id");
      result.error(invalidTaskId, "not found task corresponding to given task id", null);
    }
  }

  private pause(call: MethodCall, result: MethodResult) {
    const taskId = this.requireArgument<string>(call, "task_id");
    request.agent.getTask(this.context!, taskId).then((task: request.agent.Task) => {
      task.pause((err: BusinessError) => {
        if (err) {
          Logger.error(TAG, `Failed to pause the download task, Code: ${err.code}, message: ${err.message}`);
          return;
        }
        Logger.info(TAG, `Succeeded in pausing a download task. `);
      })
    })
    result.success(null);
  }

  private cancel(call: MethodCall, result: MethodResult) {
    const taskId = this.requireArgument<string>(call, "task_id");
    this.cancelTask(taskId);
    result.success(null);
  }

  private cancelAll(call: MethodCall, result: MethodResult) {
    for (const taskId of this.tasks) {
      this.cancelTask(taskId);
    }
    result.success(null);
  }

  private loadTasksWithRawQuery(call: MethodCall, result: MethodResult) {
    const query = this.requireArgument<string>(call, "query");
    const tasks = this.taskDao!.loadTasksWithRawQuery(query);
    const array: Array<Record<string, Any>> = new Array();
    for (const task of tasks) {
      const item: Record<string, Any> = {
        "task_id": task.taskId,
        "status": task.status,
        "progress": task.progress,
        "url": task.url,
        "file_name": task.filename,
        "saved_dir": task.savedDir,
        "time_created": task.timeCreated,
        "allow_cellular": task.allowCellular
      }
      array.push(item);
    }
    result.success(array);
  }

  private async loadTasks(call: MethodCall, result: MethodResult) {
    const tasks = await this.taskDao!.loadAllTasks();
    const array: Array<Record<string, Any>> = new Array();
    for (const task of tasks) {
      const item: Record<string, Any> = {
        "task_id": task.taskId,
        "status": task.status,
        "progress": task.progress,
        "url": task.url,
        "file_name": task.filename,
        "saved_dir": task.savedDir,
        "time_created": task.timeCreated,
        "allow_cellular": task.allowCellular
      }
      array.push(item);
      this.tasks.add(task.taskId);
    }
    result.success(array);
  }

  private requireArgument<T>(call: MethodCall, key: string): T {
    const value: Any = call.argument(key);
    if (value === null || value === undefined) {
      throw new Error(`Required key '${key}' was null`);
    }
    return value as T; // 使用类型断言返回值
  }

  private async enqueue(call: MethodCall, result: MethodResult): Promise<void> {
    const url = this.requireArgument<string>(call, 'url');
    const savedDir = this.requireArgument<string>(call, 'saved_dir');
    const filename: string = call.argument('file_name') ?? url.substring(url.lastIndexOf("/") + 1, url.length);
    const headers = this.requireArgument<string>(call, 'headers');
    const showNotification = this.requireArgument<boolean>(call, 'show_notification');
    // 如果showNotification为false则提示：鸿蒙不支持将showNotification设置为false。（鸿蒙官方暂不支持该属性）
    if (!showNotification) {
        result.error("8", "ohos does not support setting showNotification to false.", "ohos does not support setting showNotification to false.");
        return;
    }
    const openFileFromNotification = this.requireArgument<boolean>(call, 'open_file_from_notification');
    const allowCellular = this.requireArgument<boolean>(call, 'allow_cellular');
    const taskId =
      await this.downloadFile(url, headers, filename, savedDir, allowCellular, showNotification,
        openFileFromNotification);
    result.success(taskId);
    this.sendUpdateProgress(taskId, DownloadStatus.ENQUEUED, 0);
    this.taskDao!.insertOrUpdateNewTask(
      taskId,
      url,
      DownloadStatus.ENQUEUED,
      0,
      filename,
      savedDir,
      headers,
      showNotification,
      openFileFromNotification,
      allowCellular
    );
    return Promise.resolve();
  }

  private registerCallback(call:
    MethodCall, result:
    MethodResult
  ): void {
    const args: number[] = call.args as number[];
    this.callbackHandle = args[0];
    result.success(null);
  }

  private initialize(call:
    MethodCall, result:
    MethodResult
  ): void {
    const args: number[] = call.args as number[];
    this.callbackHandle = args[0];
    this.debugMode = args[1];
    preferences.getPreferencesSync(this.context, { name: FlutterDownloaderPlugin.SHARED_PREFERENCES_KEY })
      .putSync(FlutterDownloaderPlugin.CALLBACK_DISPATCHER_HANDLE_KEY, this.callbackHandle);
    result.success(null)

    this.startBackgroundIsolate(this.callbackHandle);
  }

  private sendUpdateProgress(id: string, status: DownloadStatus, progress: number): void {
    const args: Map<string, Any> = new Map<string, Any>();
    args["task_id"] = id
    args["status"] = status
    args["progress"] = progress
    this.flutterChannel!.invokeMethod("updateProgress", args)
  }

  private async openFile(savedFile: string, mimeType: string): Promise<boolean> {
    let uri = fileUri.getUriFromPath(savedFile);
    const want: Want = {
      action: 'ohos.want.action.viewData',
      flags: wantConstant.Flags.FLAG_AUTH_READ_URI_PERMISSION,
      uri: uri,
      type: mimeType
    }
    try {
      await this.context!.startAbility(want);
      return Promise.resolve(true);
    } catch (err) {
      // 处理入参错误异常
      let code = (err as BusinessError).code;
      let message = (err as BusinessError).message;
      Logger.error(TAG, `startAbility failed, code is ${code}, message is ${message}`);
      return Promise.resolve(false);
    }
  }

  private cancelTask(taskId: string): void {
    request.agent.getTask(this.context!, taskId).then((task: request.agent.Task) => {
      task.stop((err: BusinessError) => {
        if (err) {
          Logger.error(TAG, `Failed to stop the download task, Code: ${err.code}, message: ${err.message}`);
          return;
        }
        Logger.info(TAG, `Succeeded in stopping a download task. `);
        const values: ValuesBucket = {};
        values[TaskEntry.COLUMN_NAME_STATUS] = DownloadStatus.CANCELED;
        values[TaskEntry.COLUMN_NAME_PROGRESS] = -1
        this.taskDao!.updateTask(taskId, values);
        request.agent.remove(taskId, (err: BusinessError) => {
          if (err) {
            Logger.error(TAG, `Failed to removing a download task, Code: ${err.code}, message: ${err.message}`);
            return;
          }
          Logger.info(TAG, `Succeeded in removing a download task. taskId: ${taskId}`);
        });
      });
    });
  }

  private callback(taskId: string, filename: string, status: DownloadStatus,
    progress: request.agent.Progress, isResume?: number): void {
    const task = this.taskDao!.loadTask(taskId);
    const syncTask = this.taskDao!.getSyncData(taskId);
    if (task != null) {
      let lastProgress = 0;
      if (status == DownloadStatus.COMPLETE) {
        lastProgress = 100;
      } else if (status == DownloadStatus.FAILED) {
        lastProgress = -1;
      } else {
        lastProgress = Math.trunc(progress.processed * 100 / progress.sizes[0]);
        // 状态是异步获取的，不存在由PAUSED状态直接变为RUNNING状态的情况，只能是PAUSED-->Resume-->RUNNING
        const isErrorRunning = status === DownloadStatus.RUNNING && syncTask && syncTask[TaskEntry.COLUMN_NAME_STATUS] === DownloadStatus.PAUSED && isResume !== 0
        if (isErrorRunning) {
          status = DownloadStatus.PAUSED;
        }
        const args: Map<string, Any> = new Map<string, Any>();
        args["task_id"] = taskId
        args["status"] = status
        args["progress"] = lastProgress
        this.backgroundChannel?.invokeMethod('', [this.callbackHandle, taskId, status, lastProgress])
      }
      const values: ValuesBucket = {};
      values[TaskEntry.COLUMN_NAME_FILE_NAME] = filename;
      values[TaskEntry.COLUMN_NAME_STATUS] = status;
      values[TaskEntry.COLUMN_NAME_PROGRESS] = lastProgress;
      if (!isResume) {
        values[TaskEntry.COLUMN_NAME_RESUMABLE] = isResume!;
      }
      this.taskDao!.updateTask(taskId, values);
    }
  }

  private renameSavedFile(filename: string): string {
    // 匹配文件名结尾的 "(数字)"
    const regex = /\((\d+)\)$/;

    // 找到最后一个点的位置
    const lastDotIndex = filename.lastIndexOf('.');

    const baseName = lastDotIndex === -1 ? filename : filename.substring(0, lastDotIndex); // 文件名
    const extension = lastDotIndex === -1 ? '' : filename.substring(lastDotIndex); // 包含点的扩展名

    // 如果基本名称的结尾匹配到 "(数字)"，则替换
    if (regex.test(baseName)) {
      return baseName.replace(regex, (match, number: string) => {
        const newNumber = parseInt(number, 10) + 1; // 数字加 1
        return `(${newNumber})`; // 返回新的 "(新数字)"
      }) + extension; // 返回新的文件名
    } else {
      // 如果没有匹配到 "(数字)"，则在扩展名之前添加 "(1)"
      return `${baseName}(1)${extension}`;
    }
  }

  private async downloadFile(
    url: string,
    headers: string,
    filename: string,
    savedDir: string,
    allowCellular: boolean,
    showNotification: boolean,
    openFileFromNotification: boolean
  ): Promise<string> {
    const tempDir = this.context!.getApplicationContext().tempDir;
    const tempFile = `${tempDir}/${filename}`;
    const documentViewPicker = new picker.DocumentViewPicker(this.context!);
    const documentSaveOptions = new picker.DocumentSaveOptions();
    documentSaveOptions.pickerMode = picker.DocumentPickerMode.DOWNLOAD;
    const documentSaveResult = await documentViewPicker.save(documentSaveOptions)
    const downloadDirUri = documentSaveResult[0];
    const downloadDir = new fileUri.FileUri(downloadDirUri).path;
    const saveFileDir = `${downloadDir}/${savedDir}`;
    if (!fs.accessSync(saveFileDir)) fs.mkdirSync(`${saveFileDir}`, true);
    let savedFile = `${saveFileDir}/${filename}`;
    while (fs.accessSync(savedFile)) {
      filename = this.renameSavedFile(filename);
      savedFile = `${saveFileDir}/${filename}`;
    }
    const requestConfig: request.agent.Config = {
      action: request.agent.Action.DOWNLOAD,
      url: url,
      saveas: tempFile,
      metered: allowCellular,
      overwrite: true,
      gauge: showNotification
    };

    const downloadTask = await request.agent.create(this.context!, requestConfig);
    const taskId = downloadTask.tid;
    downloadTask.start((err: BusinessError) => {
      if (err) {
        Logger.error(TAG, `Failed to start the download task, Code: ${err.code}, message: ${err.message}`);
        return;
      }
      Logger.info(TAG, `Succeeded in starting a download task.`);
      this.tasks.add(taskId);
    });

    downloadTask.on('progress', (progress) => {
      Logger.info(TAG, `downloader running`);
      this.callback(taskId, filename, DownloadStatus.RUNNING, progress);
    });

    downloadTask.on('pause', (progress) => {
      Logger.info(TAG, `downloader paused`);
      this.callback(taskId, filename, DownloadStatus.PAUSED, progress, 1);
    });

    downloadTask.on('resume', (progress) => {
      Logger.info(TAG, `downloader resumed`);
      this.callback(taskId, filename, DownloadStatus.RUNNING, progress, 0);
    });

    downloadTask.on('failed', (progress) => {
      Logger.info(TAG, `downloader failed`);
      this.callback(taskId, filename, DownloadStatus.FAILED, progress);
    });

    downloadTask.on('remove', () => {
      this.tasks.delete(taskId);
    });

    downloadTask.on('completed', () => {
      Logger.info(TAG, `Request download completed`);
      this.backgroundChannel?.invokeMethod('', [this.callbackHandle, taskId, DownloadStatus.COMPLETE, 100])
      fs.moveFileSync(tempFile, savedFile, 0);
      const task = this.taskDao!.loadTask(taskId);
      if (task != null) {
        if (openFileFromNotification) {
          const mimeType = task.mimeType;
          this.openFile(savedFile, mimeType);
        }
        const values: ValuesBucket = {};
        values[TaskEntry.COLUMN_NAME_SAVED_DIR] = saveFileDir;
        values[TaskEntry.COLUMN_NAME_STATUS] = DownloadStatus.COMPLETE;
        values[TaskEntry.COLUMN_NAME_PROGRESS] = 100;
        this.taskDao!.updateTask(taskId, values);
      }

      request.agent.remove(taskId, (err: BusinessError) => {
        if (err) {
          Logger.error(TAG, `Failed to removing a download task, Code: ${err.code}, message: ${err.message}`);
          return;
        }
        Logger.info(TAG, `Succeeded in removing a download task.`);
      })
    })

    return Promise.resolve(taskId);
  }

  private async startBackgroundIsolate(handle: number) {
    if (this.backgroundFlutterEngine == null) {
      let context = this.context ?? getContext(this);
      this.backgroundFlutterEngine = new FlutterEngine(context, null, null, null);
      await this.backgroundFlutterEngine.init(context, null, true);

      let flutterCallback = FlutterCallbackInformation.lookupCallbackInformation(handle);
      if (flutterCallback == null) {
        Logger.error(TAG, `Fatal: failed to find callback".`);
        return
      }
      let resourceManager = context.getApplicationContext().resourceManager;
      let appBundlePath = FlutterInjector.getInstance().getFlutterLoader().findAppBundlePath();
      this.backgroundFlutterEngine?.dartExecutor.executeDartCallback(
        new DartCallback(resourceManager, appBundlePath, flutterCallback)
      )
    }
    this.backgroundChannel =
      new MethodChannel(this.backgroundFlutterEngine.getDartExecutor().getBinaryMessenger(), "vn.hunghd/downloader_background");
    this.backgroundChannel.setMethodCallHandler(this);
  }
}
/**
 * MIT License
 *
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

import { TurboModule, TurboModuleContext } from '@rnoh/react-native-openharmony/ts';
import { pasteboard, BusinessError } from '@kit.BasicServicesKit';
import util from '@ohos.util';
import image from '@ohos.multimedia.image';
import logger from './Logger';
import abilityAccessCtrl, { Permissions } from '@ohos.abilityAccessCtrl';
import url from '@ohos.url';

const TAG = "RNCClipboardTurboModule"
const prefixPNG = "data:image/png;base64,"
// 统一使用标准的 "image/jpeg" MIME，与 getImage()/addListener 监听路径
// （getImageFormatFromPasteData -> createDataUri -> "data:image/jpeg;base64,"）保持一致，
// 否则 getImageJPG() 返回 "data:image/jpg;..." 而监听器返回 "data:image/jpeg;..."，
// 两份图片数据无法直接相等比对。
const prefixJPG = "data:image/jpeg;base64,"
const imageMimePrefix = "image/"
const defaultImageMimeType = "image/png"
const PERMISSIONS: Array<Permissions> = [
  'ohos.permission.READ_PASTEBOARD'
]

export class RNCClipboardTurboModule extends TurboModule {
  private clipboardListener: (() => void) | null = null;
  private isListening: boolean = false;
  private isDestroyed: boolean = false;
  private lastClipboardWritePromise: Promise<void> = Promise.resolve();

  /**
   * 将一次剪贴板写入操作排队，确保所有 setData/clear 严格串行执行，
   * 避免并发触发 "Another setData is being processed"。
   */
  private enqueueClipboardWrite(task: () => Promise<void>, onSkipped?: () => void): Promise<void> {
    if (this.isDestroyed) {
      onSkipped?.();
      return Promise.resolve();
    }
    const run = this.lastClipboardWritePromise.then(() => {
      if (this.isDestroyed) {
        onSkipped?.();
        return;
      }
      return task();
    });
    // 即便 task 失败也不能让整条链中断，否则后续写入永远排不上
    this.lastClipboardWritePromise = run.then(() => {}, () => {});
    return run;
  }

  private isValidString(value: string | undefined | null): boolean {
    return typeof value === 'string';
  }

  private rejectIfDestroyed(reject: (reason: Error) => void): boolean {
    if (this.isDestroyed) {
      reject(new Error('RNCClipboardTurboModule has been destroyed'));
      return true;
    }
    return false;
  }

  constructor(protected ctx: TurboModuleContext) {
    super(ctx);
    this.clipboardListener = () => {
      if (this.isDestroyed) {
        return;
      }
      try {
        logger.debug(TAG, '[RNOH]:RNCClipboardTurboModule clipboard changed, emitting event');
        this.ctx.rnInstance.emitDeviceEvent('RNCClipboard_TEXT_CHANGED', null);
      } catch (err) {
        logger.error(TAG, `emitDeviceEvent failed: ${(err as Error).message}`);
      }
    };
    logger.debug(TAG, 'RNCClipboardTurboModule constructor');
  }

  /**
   * Returns TurboModule constants. Clipboard does not expose any constants.
   * @returns An empty constants object.
   * @throws Does not throw.
   */
  getConstants() {
    logger.debug(TAG, 'RNCClipboardTurboModule call getConstants');
    return {};
  }

  /**
   * Reads the primary plain-text content from the system clipboard.
   * @returns A Promise that resolves with the clipboard text.
   * @throws Rejects with `{ code: 0, message: "User refuses authorization" }` if the user denies `ohos.permission.READ_PASTEBOARD`.
   * @throws Rejects with a pasteboard `BusinessError` if reading PasteData fails.
   */
  getString(): Promise<string> {
    return new Promise<string>((resolve, reject) => {
      if (this.rejectIfDestroyed(reject)) {
        return;
      }
      this.requestPermission().then(res => {
        if (this.rejectIfDestroyed(reject)) {
          return;
        }
        if (res) {
          let systemPasteboard: pasteboard.SystemPasteboard;
          try {
            systemPasteboard = pasteboard.getSystemPasteboard();
          } catch (e) {
            reject(e);
            return;
          }
          systemPasteboard.getData().then((pasteData) => {
            let text = pasteData.getPrimaryText();
            logger.debug(TAG, 'getString, clipboard text read successfully');
            resolve(text ?? '');
          }).catch((err) => {
            logger.error(TAG, `getString,Failed to get PasteData. Cause:${err.message}`);
            reject(err);
          })
        } else {
          reject({
            code: 0,
            message: "User refuses authorization"
          })
        }
      }).catch((err) => {
        logger.error(TAG, `getString,Unexpected error after permission check. Cause:${err.message}`);
        reject(err);
      })
    });
  }

  /**
   * Reads all plain-text records from the system clipboard.
   * @returns A Promise that resolves with an array of clipboard text records.
   * @throws Rejects with `{ code: 0, message: "User refuses authorization" }` if the user denies clipboard read permission.
   * @throws Rejects with a pasteboard `BusinessError` if reading PasteData fails.
   */
  getStrings(): Promise<string[]> {
    return new Promise<string[]>((resolve, reject) => {
      if (this.rejectIfDestroyed(reject)) {
        return;
      }
      this.requestPermission().then(res => {
        if (this.rejectIfDestroyed(reject)) {
          return;
        }
        if (res) {
          logger.debug(TAG, 'RNCClipboardTurboModule call getStrings');
          let systemPasteboard: pasteboard.SystemPasteboard;
          try {
            systemPasteboard = pasteboard.getSystemPasteboard();
          } catch (e) {
            reject(e);
            return;
          }
          systemPasteboard.getData().then((pasteData) => {
            let count = pasteData.getRecordCount();
            let resultSet = []
            logger.debug(TAG, `getStrings fun,getRecordCount :${count}`);
            for (let index = 0; index < count; index++) {
              let record = pasteData.getRecord(index)
              if (record.mimeType == pasteboard.MIMETYPE_TEXT_PLAIN) {
                if (record.plainText !== undefined && record.plainText !== null) {
                  resultSet.push(record.plainText)
                }
              }
            }
            resolve(resultSet)
          }).catch((err) => {
            logger.error(TAG, `getString fun,Failed to get PasteData. Cause:${err.message}`);
            reject(err)
          })
        } else {
          reject({
            code: 0,
            message: "User refuses authorization"
          })
        }
      }).catch((err) => {
        logger.error(TAG, `getStrings,Unexpected error after permission check. Cause:${err.message}`);
        reject(err);
      })
    });
  }

  /**
   * Writes a plain-text string to the system clipboard.
   * @param content The text to store on the clipboard.
   * @returns void. Write failures are logged and are not thrown to JS.
   * @throws Does not throw to the JS side. Pasteboard `setData` failures are logged.
   * @remarks Silent-failure semantics (same as upstream iOS/Android `setString`): this
   * `void` method does not return a Promise and does not throw or reject to JS. Invalid
   * input or pasteboard write failures are only recorded in native logs.
   */
  setString(content: string) {
    logger.debug(TAG, 'setString fun');
    if (!this.isValidString(content)) {
      logger.error(TAG, 'setString fun,Invalid content: expected a string');
      return;
    }
    try {
      let systemPasteboard = pasteboard.getSystemPasteboard();
      let pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_PLAIN, content);
      this.enqueueClipboardWrite(() => systemPasteboard.setData(pasteData).then(() => {
        logger.debug(TAG, 'setString fun,Succeeded PasteData');
      })).catch((err) => {
        logger.error(TAG, `setString fun,Failed to set PasteData. Cause:${err.message}`);
      });
    } catch (err) {
      logger.error(TAG, `setString fun,Failed to create PasteData. Cause:${(err as Error).message}`);
    }
    return;
  }

  /**
   * Checks whether the clipboard has content or is empty.
   * Matches Android: `true` if there is any clipboard record (`getRecordCount() > 0`),
   * not iOS `hasStrings` exact string matching.
   * @returns A Promise that resolves to `true` if the clipboard has content; `false` if it is empty.
   * @throws Rejects with `{ code: 0, message: "User refuses authorization" }` if the user denies clipboard read permission.
   * @throws Rejects with a pasteboard `BusinessError` if reading PasteData fails.
   */
  hasString(): Promise<boolean> {
    return new Promise<boolean>((resolve, reject) => {
      if (this.rejectIfDestroyed(reject)) {
        return;
      }
      this.requestPermission().then(res => {
        if (this.rejectIfDestroyed(reject)) {
          return;
        }
        if (res) {
          logger.debug(TAG, 'RNCClipboardTurboModule call hasString');
          let systemPasteboard: pasteboard.SystemPasteboard;
          try {
            systemPasteboard = pasteboard.getSystemPasteboard();
          } catch (e) {
            reject(e);
            return;
          }
          systemPasteboard.getData().then((pasteData) => {
            let count = pasteData.getRecordCount();
            resolve(count > 0)
          }).catch((err) => {
            logger.error(TAG, `RNCClipboardTurboModule hasString,Failed to get PasteData. Cause:${err.message}`);
            reject(err)
          })
        } else {
          reject({
            code: 0,
            message: "User refuses authorization"
          })
        }
      }).catch((err) => {
        logger.error(TAG, `hasString,Unexpected error after permission check. Cause:${err.message}`);
        reject(err);
      })
    });
  }

  /**
   * Checks whether the clipboard contains a plain-text record that is a number.
   * @returns A Promise that resolves to `true` if a numeric plain-text record exists; otherwise `false`.
   * @throws Rejects with `{ code: 0, message: "User refuses authorization" }` if the user denies clipboard read permission.
   * @throws Rejects with a pasteboard `BusinessError` if reading PasteData fails.
   */
  hasNumber(): Promise<boolean> {
    return new Promise<boolean>((resolve, reject) => {
      if (this.rejectIfDestroyed(reject)) {
        return;
      }
      this.requestPermission().then(res => {
        if (this.rejectIfDestroyed(reject)) {
          return;
        }
        if (res) {
          logger.debug(TAG, '[RNOH]:RNCClipboardTurboModule call hasNumber');
          let systemPasteboard: pasteboard.SystemPasteboard;
          try {
            systemPasteboard = pasteboard.getSystemPasteboard();
          } catch (e) {
            reject(e);
            return;
          }
          let reg = /^[\d]*$/;
          systemPasteboard.getData().then((pasteData) => {
            let count = pasteData.getRecordCount();
            let result = false
            for (let index = 0; index < count; index++) {
              let record = pasteData.getRecord(index);
              if (record.mimeType == pasteboard.MIMETYPE_TEXT_PLAIN && reg.test(record.plainText)) {
                result = true
                logger.debug(TAG, `[RNOH]:RNCClipboardTurboModule hasNumber,result out:${result}`);
                break
              }
            }
            resolve(result)
          }).catch((err) => {
            logger.error(TAG, `[RNOH]: hasNumber,Failed to get PasteData. Cause:${err.message}`);
            reject(err)
          })
        } else {
          reject({
            code: 0,
            message: "User refuses authorization"
          })
        }
      }).catch((err) => {
        logger.error(TAG, `hasNumber,Unexpected error after permission check. Cause:${err.message}`);
        reject(err);
      })
    });
  }

  /**
   * Reads the primary clipboard image and returns it as a PNG base64 Data URI.
   * @returns A Promise that resolves with a `data:image/png;base64,...` string, if there is no image.
   * @throws Rejects with `{ code: 0, message: "User refuses authorization" }` if the user denies clipboard read permission.
   * @throws Rejects with `{ code: 2, message: "Failed to request permission" }` if the permission request fails.
   * @throws Rejects with a packing or pasteboard error if image encoding or `getData` fails.
   */
  getImagePNG(): Promise<string> {
    return new Promise<string>((resolve, reject) => {
      if (this.rejectIfDestroyed(reject)) {
        return;
      }
      this.requestPermission().then(res => {
        if (this.rejectIfDestroyed(reject)) {
          return;
        }
        if (res) {
          logger.debug(TAG, 'RNCClipboardTurboModule call getImagePNG');
          let systemPasteboard: pasteboard.SystemPasteboard;
          try {
            systemPasteboard = pasteboard.getSystemPasteboard();
          } catch (e) {
            reject(e);
            return;
          }

          systemPasteboard.getData().then(async (pasteData) => {
            let pixMap: image.PixelMap | null | undefined = null;
            try {
              pixMap = pasteData.getPrimaryPixelMap();
            } catch (err) {
              logger.debug(TAG, `getImagePNG, no PixelMap. Cause:${(err as BusinessError).message}`);
              reject({	 
                code: 1,	 
                message: "Clipboard does not contain an image"	 
              });
              return;
            }
            if (!pixMap) {
              logger.debug(TAG, 'RNCClipboardTurboModule call getImagePNG,pixMap is null or undefined');
              reject({	 
                code: 1,	 
                message: "Clipboard does not contain an image"	 
              });
              return;
            }
            try {
              const imagePackerApi = image.createImagePacker();
              try {
                let packOpt: image.PackingOption = {
                  format: "image/png",
                  quality: 96
                }
                let data = await imagePackerApi.packing(pixMap, packOpt);
                let uint8Array = new Uint8Array(data);
                let base64Helper = new util.Base64Helper();
                let base64Str = await base64Helper.encodeToString(uint8Array, util.Type.BASIC)
                let finalStr = prefixPNG + base64Str
                resolve(finalStr)
              } catch (err) {
                logger.error(TAG, `RNCClipboardTurboModule call getImagePNG,failed to packing. Cause:${(err as Error).message}`);
                reject(err)
              } finally {
                try {
                  await imagePackerApi.release();
                  logger.debug(TAG, 'RNCClipboardTurboModule call getImagePNG, releasing image packaging');
                } catch (error) {
                  logger.error(TAG, 'RNCClipboardTurboModule call getImagePNG, releasing image packaging error');
                }
              }
            } catch (err) {
              logger.error(TAG, `RNCClipboardTurboModule call getImagePNG,failed to create ImagePacker. Cause:${(err as Error).message}`);
              reject(err)
            } finally {
              try {
                await pixMap.release();
                logger.debug(TAG, 'RNCClipboardTurboModule call getImagePNG, releasing PixelMap');
              } catch (error) {
                logger.error(TAG, 'RNCClipboardTurboModule call getImagePNG, releasing PixelMap error');
              }
            }
          }).catch((err) => {
            logger.error(TAG, 'RNCClipboardTurboModule call getImagePNG,failed to getData');
            reject(err)
          })
        } else {
          reject({
            code: 0,
            message: "User refuses authorization"
          })
        }
      }).catch((err) => {
        logger.error(TAG, 'RNCClipboardTurboModule call getImagePNG,failed to request permission');
        reject({
          code: 2,
          message: "Failed to request permission"
        })
      });
    });
  }

  /**
   * Reads the primary clipboard image and returns it as a JPEG base64 Data URI.
   * @returns A Promise that resolves with a `data:image/jpeg;base64,...` string, if there is no image.
   * @throws Rejects with `{ code: 0, message: "User refuses authorization" }` if the user denies clipboard read permission.
   * @throws Rejects with `{ code: 2, message: "Failed to request permission" }` if the permission request fails.
   * @throws Rejects with a packing or pasteboard error if image encoding or `getData` fails.
   */
  getImageJPG(): Promise<string> {
    return new Promise<string>((resolve, reject) => {
      if (this.rejectIfDestroyed(reject)) {
        return;
      }
      this.requestPermission().then(res => {
        if (this.rejectIfDestroyed(reject)) {
          return;
        }
        if (res) {
          logger.debug(TAG, 'RNCClipboardTurboModule call getImageJPG');
          let systemPasteboard: pasteboard.SystemPasteboard;
          try {
            systemPasteboard = pasteboard.getSystemPasteboard();
          } catch (e) {
            reject(e);
            return;
          }

          systemPasteboard.getData().then(async (pasteData) => {
            let pixMap: image.PixelMap | null | undefined = null;
            try {
              pixMap = pasteData.getPrimaryPixelMap();
            } catch (err) {
              logger.debug(TAG, `getImageJPG, no PixelMap. Cause:${(err as BusinessError).message}`);
              reject({	 
                code: 1,	 
                message: "Clipboard does not contain an image"	 
              });
              return;
            }
            if (!pixMap) {
              logger.debug(TAG, 'RNCClipboardTurboModule call getImageJPG,pixMap is null or undefined');
              reject({	 
                code: 1,	 
                message: "Clipboard does not contain an image"	 
              });
              return;
            }
            try {
              const imagePackerApi = image.createImagePacker();
              try {
                let packOpt: image.PackingOption = {
                  format: "image/jpeg",
                  quality: 96
                }
                let data = await imagePackerApi.packing(pixMap, packOpt);
                let uint8Array = new Uint8Array(data);
                let base64Helper = new util.Base64Helper();
                let base64Str = await base64Helper.encodeToString(uint8Array, util.Type.BASIC)
                let finalStr = prefixJPG + base64Str
                resolve(finalStr)
              } catch (err) {
                logger.error(TAG, `RNCClipboardTurboModule call getImageJPG,failed to packing. Cause:${(err as Error).message}`);
                reject(err)
              } finally {
                try {
                  await imagePackerApi.release();
                  logger.debug(TAG, 'RNCClipboardTurboModule call getImageJPG, releasing image packaging');
                } catch (error) {
                  logger.error(TAG, 'RNCClipboardTurboModule call getImageJPG, releasing image packaging error');
                }
              }
            } catch (err) {
              logger.error(TAG, `RNCClipboardTurboModule call getImageJPG,failed to create ImagePacker. Cause:${(err as Error).message}`);
              reject(err)
            } finally {
              try {
                await pixMap.release();
                logger.debug(TAG, 'RNCClipboardTurboModule call getImageJPG, releasing PixelMap');
              } catch (error) {
                logger.error(TAG, 'RNCClipboardTurboModule call getImageJPG, releasing PixelMap error');
              }
            }
          }).catch((err) => {
            logger.error(TAG, 'RNCClipboardTurboModule call getImageJPG,failed to getData');
            reject(err)
          })
        } else {
          reject({
            code: 0,
            message: "User refuses authorization"
          })
        }
      }).catch((err) => {
        logger.error(TAG, 'RNCClipboardTurboModule call getImageJPG,failed to request permission');
        reject({
          code: 2,
          message: "Failed to request permission"
        })
      });
    });
  }

  /**
   * Writes a base64-encoded image to the system clipboard.
   * @param content Base64 image data written to the pasteboard.
   * @returns void. Write failures are logged and are not thrown to JS.
   * @throws Does not throw to the JS side. Image decode or pasteboard `setData` failures are logged.
   * @remarks Silent-failure semantics (same as upstream iOS `setImage`): this `void` method
   * does not return a Promise and does not throw or reject to JS. Invalid input, image
   * decode errors, or pasteboard write failures are only recorded in native logs.
   */
  setImage(content: string) {
    logger.debug(TAG, 'RNCClipboardTurboModule call setImage');
    if (!this.isValidString(content)) {
      logger.error(TAG, 'setImage,Invalid content: expected a string');
      return;
    }
    void this.applySetImage(content);
  }

  private async applySetImage(content: string): Promise<void> {
    let imageSource: image.ImageSource | null = null;
    try {
      let iconBase64 = content;

      let base64 = new util.Base64Helper();
      let uint8 = await base64.decode(iconBase64, util.Type.BASIC);
      let arrayBuffer = uint8.buffer.slice(uint8.byteOffset, uint8.byteLength + uint8.byteOffset);
      imageSource = image.createImageSource(arrayBuffer);
      logger.debug(TAG, 'RNCClipboardTurboModule call setImage100');

      // 获取图片格式信息
      let imageFormat = this.getImageFormatFromArrayBuffer(arrayBuffer);
      logger.debug(TAG, `[RNOH]:RNCClipboardTurboModule call setImage, detected format: ${imageFormat}`);

      imageSource.getImageInfo().then(value => {
        let hValue = Math.round(value.size.height);
        let wValue = Math.round(value.size.width);
        let defaultSize: image.Size = {
          height: hValue,
          width: wValue
        };
        let opts: image.DecodingOptions = {
          editable: true,
          desiredSize: defaultSize
        };
        logger.debug(TAG, '[RNOH]:RNCClipboardTurboModule call setImage200');
        let pixMap: image.PixelMap | null | undefined = null;
        return imageSource!.createPixelMap(opts).then(async (createdPixMap) => {
          pixMap = createdPixMap;
          const releasePixMap = async () => {
            if (!pixMap) {
              return;
            }
            try {
              await pixMap.release();
              logger.debug(TAG, 'RNCClipboardTurboModule call setImage, releasing PixelMap');
            } catch (error) {
              logger.error(TAG, 'RNCClipboardTurboModule call setImage, releasing PixelMap error');
            }
            pixMap = null;
          };
          try {
            logger.debug(TAG, 'RNCClipboardTurboModule call setImage300');

            let systemPasteboard = pasteboard.getSystemPasteboard();
            // 先以原始图片格式创建 PasteData，再 addRecord 将 PixelMap 插入头部作为主记录。
            // addRecord 插入到头部，PixelMap 仍是 primary（hasImage/getPrimaryPixelMap 可用），
            // 同时保留 image/jpeg 或 image/png 子记录，供 getImage 检测格式并与 getImageJPG/getImagePNG 一致。
            let pasteData = pasteboard.createData(imageFormat, arrayBuffer);
            pasteData.addRecord(pasteboard.MIMETYPE_PIXELMAP, pixMap);
            let record = pasteData.getRecord(0);
            if (record) {
              if (!record.data) {
                record.data = {};
              }
              record.data[imageFormat] = arrayBuffer;
            }

            if (this.isDestroyed) {
              logger.debug(TAG, 'setImage skipped: module destroyed before enqueue, releasing PixelMap');
              await releasePixMap();
              return;
            }

            this.enqueueClipboardWrite(() => systemPasteboard.setData(pasteData).then(async () => {
              logger.debug(TAG, `[RNOH]:RNCClipboardTurboModule call setImage305,successed in setting pasteData with format: ${imageFormat}`);
              await releasePixMap();
            }), async () => {
              logger.debug(TAG, 'setImage skipped: module destroyed while queued, releasing PixelMap');
              await releasePixMap();
            }).catch(async (err) => {
              logger.error(TAG, `RNCClipboardTurboModule call setImage305,failed in setting pasteData. Cause:${err && err.message ? err.message : JSON.stringify(err)}`);
              await releasePixMap();
            });
          } catch (err) {
            logger.error(TAG, `setImage failed to create pasteData: ${(err as Error).message}`);
            await releasePixMap();
          }
        }).catch(async (err) => {
          logger.warn(TAG, `setImage createPixelMap failed: ${err && err.message ? err.message : JSON.stringify(err)}`);
          if (pixMap) {
            try {
              await pixMap.release();
              logger.debug(TAG, 'RNCClipboardTurboModule call setImage, releasing PixelMap');
            } catch (error) {
              logger.error(TAG, 'RNCClipboardTurboModule call setImage, releasing PixelMap error');
            }
            pixMap = null;
          }
        });
      }).catch((err) => {
        logger.error(TAG, `setImage getImageInfo failed: ${err && err.message ? err.message : JSON.stringify(err)}`);
      }).finally(async () => {
        if (imageSource) {
          try {
            await imageSource.release();
          } catch (error) {
            logger.error(TAG, 'RNCClipboardTurboModule call setImage, releasing ImageSource error');
          }
          imageSource = null;
        }
      });
    } catch (err) {
      logger.error(TAG, `setImage failed: ${(err as Error).message}`);
      if (imageSource) {
        try {
          await imageSource.release();
        } catch (error) {
          logger.error(TAG, 'RNCClipboardTurboModule call setImage, releasing ImageSource error');
        }
        imageSource = null;
      }
    }
  }

  // 从 ArrayBuffer 中检测图片格式
  private getImageFormatFromArrayBuffer(buffer: ArrayBuffer): string {
    let uint8Array = new Uint8Array(buffer);

    // 检查 JPEG (FF D8 FF)
    if (uint8Array.length >= 3 &&
      uint8Array[0] === 0xFF &&
      uint8Array[1] === 0xD8 &&
      uint8Array[2] === 0xFF) {
      return "image/jpeg";
    }

    // 检查 PNG (89 50 4E 47 0D 0A 1A 0A)
    if (uint8Array.length >= 8 &&
      uint8Array[0] === 0x89 &&
      uint8Array[1] === 0x50 &&
      uint8Array[2] === 0x4E &&
      uint8Array[3] === 0x47 &&
      uint8Array[4] === 0x0D &&
      uint8Array[5] === 0x0A &&
      uint8Array[6] === 0x1A &&
      uint8Array[7] === 0x0A) {
      return "image/png";
    }

    // 检查 WEBP (52 49 46 46 ... 57 45 42 50)
    if (uint8Array.length >= 12 &&
      uint8Array[0] === 0x52 &&
      uint8Array[1] === 0x49 &&
      uint8Array[2] === 0x46 &&
      uint8Array[3] === 0x46 &&
      uint8Array[8] === 0x57 &&
      uint8Array[9] === 0x45 &&
      uint8Array[10] === 0x42 &&
      uint8Array[11] === 0x50) {
      return "image/webp";
    }

    // 检查 GIF (47 49 46 38)
    if (uint8Array.length >= 4 &&
      uint8Array[0] === 0x47 &&
      uint8Array[1] === 0x49 &&
      uint8Array[2] === 0x46 &&
      uint8Array[3] === 0x38) {
      return "image/gif";
    }

    // 检查 BMP (42 4D)
    if (uint8Array.length >= 2 &&
      uint8Array[0] === 0x42 &&
      uint8Array[1] === 0x4D) {
      return "image/bmp";
    }

    // 默认返回 PNG
    logger.debug(TAG, 'RNCClipboardTurboModule getImageFormatFromArrayBuffer, unknown format, defaulting to image/png');
    return "image/png";
  }

  /**
   * Reads the clipboard image and returns a base64 Data URI in the detected format (JPEG or PNG).
   * @returns A Promise that resolves with a `data:image/*;base64,...` string, or an empty string if no image is found.
   * @throws Rejects with `{ code: 0, message: "User refuses authorization" }` if the user denies clipboard read permission.
   * @throws Rejects with a pasteboard or packing error if reading or encoding the image fails.
   */
  getImage(): Promise<string> {
    return new Promise<string>((resolve, reject) => {
      if (this.rejectIfDestroyed(reject)) {
        return;
      }
      logger.debug(TAG, 'RNCClipboardTurboModule call getImage');
      this.requestPermission().then(async res => {
        if (this.rejectIfDestroyed(reject)) {
          return;
        }
        if (!res) {
          reject({
            code: 0,
            message: "User refuses authorization"
          })
          return
        }

        try {
          let systemPasteboard = pasteboard.getSystemPasteboard();
          let pasteData = await systemPasteboard.getData();

          let pixelMapObj: image.PixelMap | null | undefined = null;
          try {
            pixelMapObj = pasteData.getPrimaryPixelMap();
          } catch (err) {
            logger.debug(TAG, `getImage, no PixelMap. Cause:${(err as BusinessError).message}`);
          }
          if (pixelMapObj) {
            try {
              // 与 getImageJPG/getImagePNG 相同：从 PixelMap 按检测到的格式重新打包
              let imageFormat = this.getImageFormatFromPasteData(pasteData) ?? defaultImageMimeType;
              logger.debug(TAG, `[RNOH]:RNCClipboardTurboModule getImage, format: ${imageFormat}, using same repack as getImageJPG/getImagePNG`);
              let imageDataUri = await this.packPixelMapAsDataUri(pixelMapObj, imageFormat);
              resolve(imageDataUri);
              return;
            } finally {
              try {
                await pixelMapObj.release();
                logger.debug(TAG, 'RNCClipboardTurboModule call getImage, releasing PixelMap');
              } catch (releaseErr) {
                logger.error(TAG, `RNCClipboardTurboModule call getImage, releasing PixelMap error. Cause:${(releaseErr as Error).message}`);
              }
            }
          }

          // 无图片：resolve("")，与 Android getImage 一致；打包/编码失败由 getImageDataUriFromPasteData 抛出，外层 catch reject
          let imageDataUri = await this.getImageDataUriFromPasteData(pasteData);
          resolve(imageDataUri);
        } catch (err) {
          logger.error(TAG, `[RNOH]:RNCClipboardTurboModule call getImage,failed to get image. Cause:${err.message}`);
          reject(err)
        }
      }).catch((err) => {
        logger.error(TAG, `[RNOH]:RNCClipboardTurboModule call getImage,failed to request permission. Cause:${err.message}`);
        reject(err)
      })
    });
  }

  // 从剪贴板数据中检测图片格式
  private getImageFormatFromPasteData(pasteData: pasteboard.PasteData): string | null {
    let count = pasteData.getRecordCount();
    for (let index = 0; index < count; index++) {
      let record = pasteData.getRecord(index);
      // 检查 record.mimeType
      let mimeType = record.mimeType?.toLowerCase() || "";
      if (mimeType === "image/jpeg" || mimeType === "image/jpg") {
        return "image/jpeg";
      }
      if (mimeType === "image/png") {
        return "image/png";
      }
      // 检查 record.data 中的 mimeType 或二进制魔数
      if (record.data) {
        for (let mt of Object.keys(record.data)) {
          let lowerMt = mt.toLowerCase();
          if (lowerMt === "image/jpeg" || lowerMt === "image/jpg") {
            return "image/jpeg";
          }
          if (lowerMt === "image/png") {
            return "image/png";
          }
          let value = record.data[mt];
          if (value instanceof ArrayBuffer) {
            let detected = this.getImageFormatFromArrayBuffer(value);
            if (detected === "image/jpeg" || detected === "image/png") {
              return detected;
            }
          }
        }
      }
    }
    return null;
  }

  private async getImageDataUriFromPasteData(pasteData: pasteboard.PasteData): Promise<string> {
    let count = pasteData.getRecordCount();
    for (let index = 0; index < count; index++) {
      let record = pasteData.getRecord(index);
      let dataUri = await this.getImageDataUriFromRecord(record);
      if (dataUri.length > 0) {
        return dataUri
      }
    }

    let pixelMapObj: image.PixelMap | undefined | null = null;
    try {
      pixelMapObj = pasteData.getPrimaryPixelMap();
    } catch (err) {
      logger.error(TAG, `[RNOH]:RNCClipboardTurboModule getImage,no primary PixelMap. Cause:${err.message}`);
      return "";
    }

    if (pixelMapObj) {
      try {
        let imageFormat = this.getImageFormatFromPasteData(pasteData) ?? defaultImageMimeType;
        return await this.packPixelMapAsDataUri(pixelMapObj, imageFormat)
      } catch (err) {
        logger.error(TAG, `[RNOH]:RNCClipboardTurboModule getImageDataUriFromPasteData, packing/encoding failed. Cause:${(err as Error).message}`);
        throw err;
      } finally {
        try {
          await pixelMapObj.release();
          logger.debug(TAG, 'RNCClipboardTurboModule getImageDataUriFromPasteData, releasing PixelMap');
        } catch (releaseErr) {
          logger.error(TAG, `RNCClipboardTurboModule getImageDataUriFromPasteData, releasing PixelMap error. Cause:${(releaseErr as Error).message}`);
        }
      }
    }

    return ""
  }

  private async getImageDataUriFromRecord(record: pasteboard.PasteDataRecord): Promise<string> {
    if (this.isImageMimeType(record.mimeType)) {
      let dataUri = await this.getImageDataUriByMimeType(record, record.mimeType);
      if (dataUri.length > 0) {
        return dataUri
      }
    }

    if (record.data) {
      let customMimeTypes = Object.keys(record.data);
      for (let index = 0; index < customMimeTypes.length; index++) {
        let mimeType = customMimeTypes[index];
        if (this.isImageMimeType(mimeType)) {
          return await this.createDataUri(mimeType, record.data[mimeType])
        }
      }
    }

    if (record.pixelMap) {
      let mimeType = this.isImageMimeType(record.mimeType) ? record.mimeType : defaultImageMimeType;
      // record.pixelMap belongs to the system PasteData record; do not release it here.
      return await this.packPixelMapAsDataUri(record.pixelMap, mimeType)
    }

    return ""
  }

  private async getImageDataUriByMimeType(record: pasteboard.PasteDataRecord, mimeType: string): Promise<string> {
    if (record.data && record.data[mimeType]) {
      return await this.createDataUri(mimeType, record.data[mimeType])
    }

    try {
      let value = await record.getData(mimeType);
      if (value instanceof ArrayBuffer) {
        return await this.createDataUri(mimeType, value)
      }

      if (this.isPixelMap(value)) {
        let pixelMap = value as image.PixelMap;
        try {
          return await this.packPixelMapAsDataUri(pixelMap, mimeType);
        } finally {
          try {
            await pixelMap.release();
            logger.debug(TAG, 'RNCClipboardTurboModule getImageDataUriByMimeType, releasing PixelMap');
          } catch (e) {
            logger.error(TAG, `getImageDataUriByMimeType, failed to release PixelMap. Cause:${(e as Error).message}`);
          }
        }
      }
    } catch (err) {
      logger.error(TAG, `[RNOH]:RNCClipboardTurboModule getImage, failed to read mimeType:${mimeType}. Cause:${err.message}`);
    }

    return ""
  }

  private async packPixelMapAsDataUri(pixelMapObj: image.PixelMap, mimeType: string): Promise<string> {
    const imagePackerApi = image.createImagePacker();
    let packingMimeType = this.getPackingMimeType(mimeType);
    let packOpt: image.PackingOption = {
      format: packingMimeType,
      quality: 96
    }

    try {
      let data = await imagePackerApi.packing(pixelMapObj, packOpt);
      return await this.createDataUri(packingMimeType, data)
    } finally {
      try {
        await imagePackerApi.release();
        logger.debug(TAG, 'RNCClipboardTurboModule call getImage, releasing image packaging');
      } catch (error) {
        logger.error(TAG, `[RNOH]:RNCClipboardTurboModule call getImage, releasing image packaging error. Cause:${(error as Error).message}`);
      }
    }
  }

  private async createDataUri(mimeType: string, data: ArrayBuffer): Promise<string> {
    let uint8Array = new Uint8Array(data);
    let base64Helper = new util.Base64Helper();
    let base64Str = await base64Helper.encodeToString(uint8Array, util.Type.BASIC);
    return `data:${mimeType};base64,${base64Str}`
  }

  private getPackingMimeType(mimeType: string): string {
    let normalizedMimeType = mimeType.toLowerCase();
    if (normalizedMimeType == "image/jpeg" || normalizedMimeType == "image/jpg") {
      return "image/jpeg"
    }
    if (normalizedMimeType == "image/webp") {
      return "image/webp"
    }
    return defaultImageMimeType
  }

  private isImageMimeType(mimeType: string): boolean {
    return mimeType != undefined && mimeType.length > 0 && mimeType.toLowerCase().indexOf(imageMimePrefix) == 0
  }

  private isPixelMap(value: pasteboard.ValueType): boolean {
    return value != undefined && typeof value == "object" && "getImageInfo" in value
  }

  /**
   * Writes multiple plain-text strings to the clipboard. An empty array clears the clipboard.
   * @param content The text records to store. `content[0]` becomes the primary record.
   * @returns void. Write failures are logged and are not thrown to JS.
   * @throws Does not throw to the JS side. Pasteboard `setData`/`clear` failures are logged.
   * @remarks Silent-failure semantics (same as upstream iOS `setStrings`): this `void`
   * method does not return a Promise and does not throw or reject to JS. Invalid input or
   * pasteboard write/clear failures are only recorded in native logs.
   */
  setStrings(content: string[]) {
    logger.debug(TAG, 'RNCClipboardTurboModule call setStrings fun');
    if (!content || !Array.isArray(content)) {
      logger.error(TAG, 'setStrings,Invalid content: expected a string array');
      return;
    }

    if (content.length > 0) {
      for (let i = 0; i < content.length; i++) {
        if (content[i] === undefined || content[i] === null) {
          logger.error(TAG, `setStrings,Invalid content at index ${i}: null or undefined`);
          return;
        }
        if (!this.isValidString(content[i])) {
          logger.error(TAG, `setStrings,Invalid content at index ${i}: expected string elements`);
          return;
        }
      }
    }

    try {
      let systemPasteboard = pasteboard.getSystemPasteboard();

      if (content.length === 0) {
        this.enqueueClipboardWrite(() => systemPasteboard.clear().then(() => {
          logger.debug(TAG, 'setStrings,Succeeded in clearing PasteData.');
        })).catch((err: BusinessError) => {
          logger.error(TAG, `setStrings,Failed to clear PasteData. Cause:${err.message}`);
        });
        return;
      }

      // addRecord inserts records at the head of the list, so build in reverse
      // to keep content[0] as the primary record.
      let pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_PLAIN, content[content.length - 1]);
      for (let i = content.length - 2; i >= 0; i--) {
        pasteData.addRecord(pasteboard.MIMETYPE_TEXT_PLAIN, content[i]);
      }

      this.enqueueClipboardWrite(() => systemPasteboard.setData(pasteData).then(() => {
        logger.debug(TAG, 'setStrings,Succeeded in setting PasteData.');
      })).catch((err) => {
        logger.error(TAG, `setStrings,Failed to set PasteData. Cause:${err.message}`);
      });
    } catch (err) {
      logger.error(TAG, `setStrings,Failed to create PasteData. Cause:${(err as Error).message}`);
    }
  }

  /**
   * Checks whether the clipboard contains an image record (PixelMap or image MIME type).
   * @returns A Promise that resolves to `true` if an image record is present.
   * @throws Rejects with `{ code: 0, message: "User refuses authorization" }` if the user denies clipboard read permission.
   * @throws Rejects with a pasteboard `BusinessError` if reading PasteData fails.
   */
  hasImage(): Promise<boolean> {
    return new Promise<boolean>((resolve, reject) => {
      if (this.rejectIfDestroyed(reject)) {
        return;
      }
      this.requestPermission().then(res => {
        if (this.rejectIfDestroyed(reject)) {
          return;
        }
        if (res) {
          logger.debug(TAG, 'RNCClipboardTurboModule call hasImage');
          let systemPasteboard: pasteboard.SystemPasteboard;
          try {
            systemPasteboard = pasteboard.getSystemPasteboard();
          } catch (e) {
            reject(e);
            return;
          }
          systemPasteboard.getData().then(async (pasteData) => {
            let pixelMapObj: image.PixelMap | null | undefined = null;
            try {
              pixelMapObj = pasteData.getPrimaryPixelMap();
            } catch (err) {
              logger.debug(TAG, `hasImage, no PixelMap. Cause:${(err as BusinessError).message}`);
              resolve(false);
              return;
            }
            let result = false
            if (pixelMapObj) {
              logger.debug(TAG, 'RNCClipboardTurboModule call hasImage,hasPixelobj');
              result = true
              try {
                await pixelMapObj.release();
                logger.debug(TAG, 'RNCClipboardTurboModule call hasImage, releasing PixelMap');
              } catch (e) {
                logger.error(TAG, `hasImage, failed to release PixelMap. Cause:${(e as Error).message}`);
              }
            }
            resolve(result)
          }).catch((err) => {
            logger.error(TAG, `hasImage,Failed to get PasteData. Cause:${err.message}`);
            reject(err)
          })
        } else {
          reject({
            code: 0,
            message: "User refuses authorization"
          })
        }
      }).catch((err) => {
        logger.error(TAG, `hasImage,Unexpected error after permission check. Cause:${err.message}`);
        reject(err);
      });
    });
  }

  /**
   * Checks whether the clipboard contains a URL (URI or plain-text URL).
   * @returns A Promise that resolves to `true` if a valid URL is found.
   * @throws Rejects with `{ code: 0, message: "User refuses authorization" }` if the user denies clipboard read permission.
   * @throws Rejects with a pasteboard `BusinessError` if reading PasteData fails.
   */
  hasURL(): Promise<boolean> {
    return new Promise<boolean>((resolve, reject) => {
      if (this.rejectIfDestroyed(reject)) {
        return;
      }
      this.requestPermission().then(res => {
        if (this.rejectIfDestroyed(reject)) {
          return;
        }
        if (res) {
          logger.debug(TAG, 'RNCClipboardTurboModule call hasURL');
          let systemPasteboard: pasteboard.SystemPasteboard;
          try {
            systemPasteboard = pasteboard.getSystemPasteboard();
          } catch (e) {
            reject(e);
            return;
          }
          systemPasteboard.getData().then((pasteData) => {
            let count = pasteData.getRecordCount();
            let result = false;
            let isValidUrl = (string) => {
              try {
                url.URL.parseURL(string);
                return true;
              } catch (err) {
                return false;
              }
            };
            for (let index = 0; index < count; index++) {
              let record = pasteData.getRecord(index);
              if (record.mimeType == pasteboard.MIMETYPE_TEXT_URI) {
                if (isValidUrl(record.uri)) {
                  logger.debug(TAG, 'hasURL,mimeType=MIMETYPE_TEXT_URI');
                  result = true
                  break
                }
              } else if (record.mimeType == pasteboard.MIMETYPE_TEXT_PLAIN) {
                if (isValidUrl(record.plainText)) {
                  logger.debug(TAG, 'hasURL,find URL in MIMETYPE_TEXT_PLAIN');
                  result = true
                  break
                }
              }
            }
            resolve(result)
          }).catch((err) => {
            logger.error(TAG, `[RNOH]: hasURL,Failed to get PasteData. Cause:${err.message}`);
            reject(err)
          });
        } else {
          reject({
            code: 0,
            message: "User refuses authorization"
          })
        }
      }).catch((err) => {
        logger.error(TAG, `hasURL,Unexpected error after permission check. Cause:${err.message}`);
        reject(err);
      });
    });
  }

  /**
   * Checks whether the clipboard contains a Web URL (http/https/ftp, or a bare domain like example.com).
   * Aligns with iOS `UIPasteboardDetectionPatternProbableWebURL` for common cases.
   * @returns A Promise that resolves to `true` if a Web URL is found.
   * @throws Rejects with `{ code: 0, message: "User refuses authorization" }` if the user denies clipboard read permission.
   * @throws Rejects with a pasteboard `BusinessError` if reading PasteData fails.
   */
  hasWebURL(): Promise<boolean> {
    return new Promise<boolean>((resolve, reject) => {
      if (this.rejectIfDestroyed(reject)) {
        return;
      }
      this.requestPermission().then(res => {
        if (this.rejectIfDestroyed(reject)) {
          return;
        }
        if (res) {
          logger.debug(TAG, 'RNCClipboardTurboModule call hasWebURL');
          let systemPasteboard: pasteboard.SystemPasteboard;
          try {
            systemPasteboard = pasteboard.getSystemPasteboard();
          } catch (e) {
            reject(e);
            return;
          }
          systemPasteboard.getData().then((pasteData) => {
            let count = pasteData.getRecordCount();
            let result = false
            let reg = /(http|ftp|https):\/\/[\w\-_]+(\.[\w\-_]+)+([\w\-\.,@?^=%&:/~\+#]*[\w\-\@?^=%&/~\+#])?/;

            for (let index = 0; index < count; index++) {
              let record = pasteData.getRecord(index);
              if (record.mimeType == pasteboard.MIMETYPE_TEXT_URI) {
                if (reg.test(record.uri)) {
                  logger.debug(TAG, 'hasWebURL,find webURL in MIMETYPE_TEXT_URI');
                  result = true
                  break
                }
              } else if (record.mimeType == pasteboard.MIMETYPE_TEXT_PLAIN) {
                if (reg.test(record.plainText)) {
                  logger.debug(TAG, 'hasWebURL,find webURL in MIMETYPE_TEXT_PLAIN');
                  result = true
                  break
                }
              }

            }
            resolve(result)
          }).catch((err) => {
            logger.error(TAG, `hasWebURL,Failed to get PasteData. Cause:${err.message}`);
            reject(err)
          });
        } else {
          reject({
            code: 0,
            message: "User refuses authorization"
          })
        }
      }).catch((err) => {
        logger.error(TAG, `hasWebURL,Unexpected error after permission check. Cause:${err.message}`);
        reject(err);
      });
    });
  }

  /**
   * Requests `ohos.permission.READ_PASTEBOARD` from the user.
   * @returns A Promise that resolves to `true` if the permission is granted; otherwise `false`.
   * @throws Does not reject. Permission request failures resolve to `false`.
   */
  requestPermission(): Promise<boolean> {
    return new Promise<boolean>((resolve) => {
      const atManager = abilityAccessCtrl.createAtManager();
      atManager.requestPermissionsFromUser(this.ctx.uiAbilityContext, PERMISSIONS).then(result => {
        if (result.authResults[0] == 0) {
          resolve(true);
        } else {
          logger.debug(TAG, `getString,text out:用户拒绝授权`);
          resolve(false);
        }
      }).catch(() => {
        logger.error(TAG, `getString,text out:用户拒绝授权`);
        resolve(false);
      });
    });
  }

  /**
   * Starts listening for system clipboard changes via `@ohos.pasteboard` `update`.
   * Repeated calls are ignored while a listener is already registered.
   * Matches Android/iOS: emits `RNCClipboard_TEXT_CHANGED` with `null` and does not read clipboard content.
   * @returns void.
   * @throws Does not throw.
   * @remarks Side effect: registers a pasteboard `update` callback and emits `RNCClipboard_TEXT_CHANGED`
   * to the JS side whenever the clipboard changes.
   */
  setListener(): void {
    if (this.isListening) {
      logger.debug(TAG, '[RNOH]:RNCClipboardTurboModule setListener already listening');
      return;
    }
    logger.debug(TAG, '[RNOH]:RNCClipboardTurboModule setListener');
    try {
      let systemPasteboard = pasteboard.getSystemPasteboard();
      systemPasteboard.on('update', this.clipboardListener);
      this.isListening = true;
    } catch (err) {
      logger.error(TAG, `setListener failed. Cause:${(err as Error).message}`);
      this.isListening = false;
    }
  }

  /**
   * Stops listening for system clipboard changes and clears the native callback.
   * No-op if a listener has not been registered.
   * @returns void.
   * @throws Does not throw.
   * @remarks Side effect: unregisters the pasteboard `update` callback. `isListening` is
   * cleared only after `off()` succeeds, so a failed unsubscribe does not leave a stale
   * "not listening" state while the system still holds the callback.
   */
  removeListener(): void {
    if (!this.isListening || !this.clipboardListener) {
      logger.debug(TAG, '[RNOH]:RNCClipboardTurboModule removeListener not listening');
      return;
    }
    logger.debug(TAG, '[RNOH]:RNCClipboardTurboModule removeListener');
    const listener = this.clipboardListener;
    try {
      let systemPasteboard = pasteboard.getSystemPasteboard();
      systemPasteboard.off('update', listener);
      this.isListening = false;
    } catch (err) {
      logger.error(TAG, `removeListener failed. Cause:${(err as Error).message}`);
    }
  }

  /**
   * NativeEventEmitter / Spec stub required by JS `new NativeEventEmitter(module).addListener(...)`.
   * Actual clipboard-change registration happens in `setListener()`.
   * @param _eventName Event name passed by NativeEventEmitter.
   * @returns void.
   * @throws Does not throw. Missing this method would crash RNOH 0.82 with an invalid napi return value.
   */
  addListener(_eventName: string): void {
    logger.debug(TAG, `[RNOH]:RNCClipboardTurboModule addListener stub: ${_eventName}`);
  }

  /**
   * NativeEventEmitter / Spec stub corresponding to JS `removeAllListeners`.
   * Actual cleanup happens in `removeListener()`.
   * @param _count Listener count passed by NativeEventEmitter.
   * @returns void.
   * @throws Does not throw. 
   */
  removeListeners(_count: number): void {
    logger.debug(TAG, `[RNOH]:RNCClipboardTurboModule removeListeners stub: ${_count}`);
  }

  /**
   * TurboModule lifecycle hook: release pasteboard listener when the module is destroyed.
   * Nulls `clipboardListener` before `off()` so a failed unsubscribe cannot keep
   * the module-side reference alive and pin the RN instance.
   */
  __onDestroy__(): void {
    this.isDestroyed = true;
    const listener = this.clipboardListener;
    this.clipboardListener = null;
    if (listener && this.isListening) {
      try {
        pasteboard.getSystemPasteboard().off('update', listener);
      } catch (err) {
        logger.error(TAG, `off() failed: ${(err as Error).message}`);
      }
    }
    this.isListening = false;
    this.lastClipboardWritePromise = Promise.resolve();
    this.ctx = null as TurboModuleContext;

    logger.debug(TAG, 'RNCClipboardTurboModule __onDestroy__ completed');
  }
}
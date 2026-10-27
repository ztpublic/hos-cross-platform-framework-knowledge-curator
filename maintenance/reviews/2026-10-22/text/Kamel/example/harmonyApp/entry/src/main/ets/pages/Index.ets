import { kamelInit, kamelCacheInfo } from 'libkamel_napi.so';
import { image } from '@kit.ImageKit';
import { util, worker, ErrorEvent, MessageEvents } from '@kit.ArkTS';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { common } from '@kit.AbilityKit';
import { fileIo } from '@kit.CoreFileKit';

const TAG: string = 'KamelPage';

interface InitResult {
  ok: boolean;
  cacheDir?: string;
  error?: string;
}

interface LoadResult {
  ok: boolean;
  url?: string;
  source?: string;
  byteCount?: number;
  elapsedMs?: number;
  base64?: string;
  error?: string;
}

interface CacheInfoResult {
  ok: boolean;
  memoryEntries?: number;
  memoryMax?: number;
  diskCacheDir?: string;
  error?: string;
}

class Preset {
  label: string;
  url: string;
  constructor(label: string, url: string) {
    this.label = label;
    this.url = url;
  }
}

@Entry
@Component
struct Index {
  @State url: string = 'https://picsum.photos/300/200';
  @State status: string = '尚未加载';
  @State ok: boolean = true;
  @State pixelMap: image.PixelMap | undefined = undefined;
  @State loading: boolean = false;

  private workerInstance: worker.ThreadWorker | undefined = undefined;
  private readonly presets: Preset[] = [
    new Preset('Photo 1', 'https://picsum.photos/300/200'),
    new Preset('Photo 2', 'https://picsum.photos/seed/kamel/300/200'),
    new Preset('Photo 3', 'https://picsum.photos/seed/ohos/300/200'),
    new Preset('Photo 4', 'https://picsum.photos/400/260'),
    new Preset('SVG', 'https://placehold.co/300x200')
  ];

  aboutToAppear(): void {
    this.initKamel();
  }

  aboutToDisappear(): void {
    if (this.workerInstance !== undefined) {
      try {
        this.workerInstance.terminate();
      } catch (e) {
        hilog.warn(0x0000, TAG, 'terminate worker failed: %{public}s', `${e}`);
      }
      this.workerInstance = undefined;
    }
    if (this.pixelMap !== undefined) {
      this.pixelMap.release();
      this.pixelMap = undefined;
    }
  }

  build() {
    Scroll() {
      Column() {
        Text('Kamel · OpenHarmony')
          .fontSize(22)
          .fontWeight(FontWeight.Bold)
        Text('CMP 图片加载库 · Kotlin/Native ohosArm64/ohosX64')
          .fontSize(13)
          .fontColor('#6B7280')
          .margin({ bottom: 12 })

        TextInput({ text: this.url, placeholder: '图片 URL' })
          .onChange((v: string) => {
            this.url = v;
          })

        Flex({ wrap: FlexWrap.Wrap }) {
          ForEach(this.presets, (item: Preset) => {
            Button(item.label)
              .fontSize(11)
              .height(30)
              .margin({ right: 6, top: 8 })
              .backgroundColor(this.url === item.url ? '#0F766E' : '#E5E7EB')
              .fontColor(this.url === item.url ? '#FFFFFF' : '#111827')
              .onClick(() => {
                this.url = item.url;
                this.load();
              })
          }, (item: Preset) => item.url)
        }
        .margin({ top: 8 })
        .width('100%')

        Row({ space: 8 }) {
          Button(this.loading ? '加载中…' : '加载')
            .layoutWeight(1)
            .enabled(!this.loading)
            .onClick(() => {
              this.load();
            })
          Button('缓存信息')
            .layoutWeight(1)
            .onClick(() => {
              this.showCacheInfo();
            })
        }
        .margin({ top: 14 })
        .width('100%')

        if (this.pixelMap) {
          Image(this.pixelMap)
            .width('100%')
            .height(240)
            .objectFit(ImageFit.Contain)
            .margin({ top: 16 })
            .border({ width: 1, color: '#E5E7EB' })
        }

        Text(this.status)
          .fontSize(12)
          .fontColor(this.ok ? '#374151' : '#DC2626')
          .margin({ top: 12 })
          .width('100%')

        Text('再次点击同一预设图验证内存缓存命中（source=memory）；杀进程重启后可能显示 network（Ktor HttpCache 透明磁盘缓存）')
          .fontSize(11)
          .fontColor('#9CA3AF')
          .margin({ top: 8 })
          .width('100%')
      }
      .padding(20)
      .width('100%')
      .alignItems(HorizontalAlign.Start)
    }
    .width('100%')
    .height('100%')
  }

  private initKamel(): void {
    let cacheDir: string = '';
    let caBundlePath: string = '';
    try {
      const ctx = this.getUIContext().getHostContext();
      if (ctx !== undefined) {
        cacheDir = ctx.cacheDir;
        caBundlePath = this.installCaBundle(ctx);
      }
    } catch (e) {
      const rec = e as Record<string, Object>;
      hilog.warn(0x0000, TAG, 'context/CA bundle failed: %{public}s', `${rec['message']}`);
    }
    const config: string = JSON.stringify({
      cacheDir: cacheDir,
      caBundlePath: caBundlePath,
      memoryCacheSize: 100,
      diskCacheBytes: 10485760
    });
    try {
      const raw: string = kamelInit(config);
      const parsed: InitResult = JSON.parse(raw) as InitResult;
      if (parsed.ok) {
        this.ok = true;
        const ca = caBundlePath === '' ? '，CA 未安装（HTTPS 会失败）' : '';
        this.status = cacheDir === '' ? `初始化成功（未获取到 cacheDir${ca}）` : `初始化成功 cacheDir=${parsed.cacheDir ?? ''}${ca}`;
      } else {
        this.ok = false;
        this.status = `初始化失败: ${parsed.error ?? 'unknown'}`;
      }
    } catch (e) {
      const rec = e as Record<string, Object>;
      const msg = `异常: code=${rec['code']} name=${rec['name']} message=${rec['message']}`;
      this.ok = false;
      this.status = msg;
      hilog.error(0x0000, TAG, '%{public}s', msg);
    }
  }

  /**
   * The curl engine behind libohoskamel.so cannot read the HarmonyOS system
   * trust store, so HTTPS needs an explicit CA bundle (CURLOPT_CAINFO).
   * Copy the Mozilla bundle shipped in rawfile into the sandbox and hand
   * the file path to KamelInit.
   */
  private installCaBundle(ctx: common.Context): string {
    const targetPath: string = `${ctx.filesDir}/cacert.pem`;
    const data: Uint8Array = ctx.resourceManager.getRawFileContentSync('cacert.pem');
    const file = fileIo.openSync(targetPath,
      fileIo.OpenMode.READ_WRITE | fileIo.OpenMode.CREATE | fileIo.OpenMode.TRUNC);
    try {
      const buffer = data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength);
      fileIo.writeSync(file.fd, buffer);
    } finally {
      fileIo.closeSync(file);
    }
    return targetPath;
  }

  private ensureWorker(): boolean {
    if (this.workerInstance !== undefined) {
      return true;
    }
    try {
      const w: worker.ThreadWorker = new worker.ThreadWorker('entry/ets/workers/KamelWorker.ets');
      w.onmessage = (e: MessageEvents): void => {
        this.onLoadResult(e.data as string);
      };
      w.onerror = (e: ErrorEvent): void => {
        this.loading = false;
        this.ok = false;
        this.status = `Worker 异常: ${e.message}`;
        hilog.error(0x0000, TAG, 'worker error: %{public}s', e.message);
      };
      this.workerInstance = w;
      return true;
    } catch (e) {
      const rec = e as Record<string, Object>;
      const msg = `Worker 创建异常: code=${rec['code']} name=${rec['name']} message=${rec['message']}`;
      hilog.error(0x0000, TAG, '%{public}s', msg);
      return false;
    }
  }

  private load(): void {
    if (this.loading) {
      return;
    }
    if (this.workerInstance === undefined && !this.ensureWorker()) {
      this.ok = false;
      this.status = 'Worker 创建失败，无法加载';
      return;
    }
    const w = this.workerInstance;
    if (w === undefined) {
      return;
    }
    this.loading = true;
    this.ok = true;
    this.status = '正在通过 Worker 调用 libkamel_napi.so …';
    const request: string = JSON.stringify({
      url: this.url,
      headers: '',
      timeoutMs: 15000
    });
    try {
      w.postMessage(request);
    } catch (e) {
      this.loading = false;
      this.ok = false;
      this.status = `postMessage 失败: ${e}`;
    }
  }

  private onLoadResult(raw: string): void {
    let parsed: LoadResult;
    try {
      parsed = JSON.parse(raw) as LoadResult;
    } catch (e) {
      this.loading = false;
      this.ok = false;
      this.status = `响应解析失败: ${raw}`;
      return;
    }
    const b64: string = parsed.base64 ?? '';
    if (!parsed.ok || b64 === '') {
      this.loading = false;
      this.ok = false;
      this.status = `加载失败: ${parsed.error ?? 'empty image'} elapsedMs=${parsed.elapsedMs ?? 0}`;
      return;
    }
    this.decodeImage(b64, parsed.source ?? '', parsed.byteCount ?? 0, parsed.elapsedMs ?? 0);
  }

  private decodeImage(b64: string, source: string, byteCount: number, elapsedMs: number): void {
    const helper = new util.Base64Helper();
    const bytes: Uint8Array = helper.decodeSync(b64);
    const sliced = bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength);
    const src = image.createImageSource(sliced);
    src.createPixelMap().then((pm: image.PixelMap) => {
      this.replacePixelMap(pm);
      this.ok = true;
      this.status = `加载成功 source=${source} byteCount=${byteCount} elapsedMs=${elapsedMs}`;
      this.loading = false;
    }).catch((err: Error) => {
      this.ok = false;
      this.status = `PixelMap 解码失败: ${err.message}`;
      this.loading = false;
    });
  }

  private replacePixelMap(pm: image.PixelMap): void {
    if (this.pixelMap !== undefined) {
      this.pixelMap.release();
    }
    this.pixelMap = pm;
  }

  private showCacheInfo(): void {
    try {
      const raw: string = kamelCacheInfo();
      const parsed: CacheInfoResult = JSON.parse(raw) as CacheInfoResult;
      if (parsed.ok) {
        this.ok = true;
        this.status = `memoryEntries=${parsed.memoryEntries ?? 0} memoryMax=${parsed.memoryMax ?? 0} diskCacheDir=${parsed.diskCacheDir ?? ''}`;
      } else {
        this.ok = false;
        this.status = `缓存信息获取失败: ${parsed.error ?? 'unknown'}`;
      }
    } catch (e) {
      const rec = e as Record<string, Object>;
      const msg = `异常: code=${rec['code']} name=${rec['name']} message=${rec['message']}`;
      this.ok = false;
      this.status = msg;
      hilog.error(0x0000, TAG, '%{public}s', msg);
    }
  }
}

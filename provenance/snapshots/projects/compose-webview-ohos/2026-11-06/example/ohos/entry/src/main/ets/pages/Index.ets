import { webview } from '@kit.ArkWeb';
import {
  webviewVersion,
  webviewJsBridgeScript,
  webviewDispatchMessage,
  webviewFormatCookieDate,
  webviewCookieToString,
} from 'libwebview_napi.so';

const TAG: string = 'WebviewDemo';

interface NativeResult {
  ok: boolean;
  script?: string;
  error?: string;
  version?: string;
  upstream?: string;
  date?: string;
  cookie?: string;
  callbackId?: number;
}

/**
 * javaScriptProxy object: window.kmpNativeBridge.call(message).
 * This mirrors the upstream Android wiring where addJavascriptInterface("androidJsBridge")
 * receives the JSON produced by window.kmpJsBridge.postMessage.
 */
class KmpNativeProxy {
  private handler: (msg: string) => void = (_: string) => {
  };

  setHandler(h: (msg: string) => void): void {
    this.handler = h;
  }

  call(message: string): void {
    this.handler(message);
  }
}

@Entry
@Component
struct Index {
  @State currentIndex: number = 0;

  // ---- Tab 1: basic WebView (mirrors sample BasicWebViewSample) ----
  @State urlInput: string = 'https://example.com/';
  @State urlLoaded: string = 'https://example.com/';
  @State progress: number = 0;
  @State pageStatus: string = 'idle';
  private webController: webview.WebviewController = new webview.WebviewController();

  // ---- Tab 2: JS bridge (mirrors sample jsbridge + samples/index.html) ----
  @State bridgeStatus: string = 'loading local page...';
  @State bridgeLog: string = '';
  private bridgeController: webview.WebviewController = new webview.WebviewController();
  private proxy: KmpNativeProxy = new KmpNativeProxy();

  // ---- Tab 3: engine (version handshake + cookie model) ----
  @State versionText: string = '';
  @State cookieDateText: string = '';
  @State cookieText: string = '';
  @State scriptInfo: string = '';

  aboutToAppear(): void {
    this.proxy.setHandler((msg: string) => {
      this.onNativeMessage(msg);
    });
    this.refreshVersion();
    this.refreshScriptInfo();
  }

  private onNativeMessage(msg: string): void {
    // The proxy runs off the UI thread; marshal dispatch + runJavaScript to the UI loop.
    setTimeout(() => {
      let response: string = '';
      try {
        response = webviewDispatchMessage(msg);
      } catch (e) {
        this.bridgeStatus = 'dispatch threw: ' + JSON.stringify(e);
        return;
      }
      this.bridgeLog = 'JS> ' + msg + '\nKN< ' + response + '\n\n' + this.bridgeLog;
      let parsed: NativeResult;
      try {
        parsed = JSON.parse(response) as NativeResult;
      } catch (e) {
        this.bridgeStatus = 'bad JSON from native';
        return;
      }
      if (parsed.ok && parsed.script !== undefined) {
        this.bridgeStatus = 'handled, callbackId=' + parsed.callbackId;
        try {
          this.bridgeController.runJavaScript(parsed.script);
        } catch (e) {
          this.bridgeStatus = 'runJavaScript failed: ' + JSON.stringify(e);
        }
      } else {
        this.bridgeStatus = 'native error: ' + (parsed.error ?? 'unknown');
      }
    }, 0);
  }

  private injectBridge(): void {
    let response: string = '';
    try {
      response = webviewJsBridgeScript();
    } catch (e) {
      this.bridgeStatus = 'script entry threw: ' + JSON.stringify(e);
      return;
    }
    let parsed: NativeResult;
    try {
      parsed = JSON.parse(response) as NativeResult;
    } catch (e) {
      this.bridgeStatus = 'bad JSON from script entry';
      return;
    }
    if (!parsed.ok || parsed.script === undefined) {
      this.bridgeStatus = 'script error: ' + (parsed.error ?? 'unknown');
      return;
    }
    // Engine protocol script + the postMessage wiring, identical to the Android actual
    // (window.kmpJsBridge.postMessage = function (m) { window.androidJsBridge.call(m); }).
    const wiring = 'window.kmpJsBridge.postMessage=function(m){window.kmpNativeBridge.call(m);};';
    try {
      this.bridgeController.runJavaScript(parsed.script + '\n' + wiring);
      this.bridgeStatus = 'bridge injected (kmpJsBridge ready)';
    } catch (e) {
      this.bridgeStatus = 'inject failed: ' + JSON.stringify(e);
    }
  }

  private refreshVersion(): void {
    try {
      const parsed = JSON.parse(webviewVersion()) as NativeResult;
      this.versionText = parsed.ok
        ? 'engine ' + (parsed.version ?? '?') + '  (upstream ' + (parsed.upstream ?? '?') + ')'
        : 'error: ' + (parsed.error ?? 'unknown');
    } catch (e) {
      this.versionText = 'webviewVersion threw: ' + JSON.stringify(e);
    }
  }

  private refreshScriptInfo(): void {
    try {
      const parsed = JSON.parse(webviewJsBridgeScript()) as NativeResult;
      this.scriptInfo = parsed.ok
        ? (parsed.script?.length ?? 0) + ' chars, defines window.kmpJsBridge.callNative/onCallback'
        : 'error: ' + (parsed.error ?? 'unknown');
    } catch (e) {
      this.scriptInfo = 'threw: ' + JSON.stringify(e);
    }
  }

  private refreshCookieDate(): void {
    const millis: number = Date.now() + 86400000; // tomorrow
    try {
      const parsed = JSON.parse(webviewFormatCookieDate('{"expiresDate":"' + millis + '"}')) as NativeResult;
      this.cookieDateText = parsed.ok
        ? 'getCookieExpirationDate(+24h) = ' + (parsed.date ?? '?')
        : 'error: ' + (parsed.error ?? 'unknown');
    } catch (e) {
      this.cookieDateText = 'threw: ' + JSON.stringify(e);
    }
  }

  private refreshCookie(): void {
    const request = '{"name":"session","value":"kmp","domain":"example.com","path":"/",' +
      '"expiresDate":"' + (Date.now() + 3600000) + '","maxAge":"3600"}';
    try {
      const parsed = JSON.parse(webviewCookieToString(request)) as NativeResult;
      this.cookieText = parsed.ok
        ? 'Cookie.toString() = ' + (parsed.cookie ?? '?')
        : 'error: ' + (parsed.error ?? 'unknown');
    } catch (e) {
      this.cookieText = 'threw: ' + JSON.stringify(e);
    }
  }

  @Builder
  tabBar(title: string, index: number) {
    Column() {
      Text(title)
        .fontSize(14)
        .fontColor(this.currentIndex === index ? '#2563eb' : '#6b7280')
        .margin({ top: 8, bottom: 8 })
    }
    .width('100%')
    .height('100%')
    .justifyContent(FlexAlign.Center)
  }

  build() {
    Column() {
      Tabs({ barPosition: BarPosition.End, index: this.currentIndex }) {
        // ---------------- Tab 1: WebView ----------------
        TabContent() {
          Column() {
            Row() {
              TextInput({ text: this.urlInput, placeholder: 'https://...' })
                .layoutWeight(1)
                .fontSize(14)
                .onChange((v: string) => {
                  this.urlInput = v;
                })
              Button('Go')
                .fontSize(14)
                .margin({ left: 8 })
                .onClick(() => {
                  this.urlLoaded = this.urlInput;
                  this.pageStatus = 'loading';
                })
            }
            .width('100%')
            .padding(12)

            Progress({ value: this.progress, total: 100 })
              .width('100%')
              .height(4)
              .color('#2563eb')

            Web({ controller: this.webController, src: this.urlLoaded })
              .javaScriptAccess(true)
              .domStorageAccess(true)
              .fileAccess(true)
              .onPageBegin(() => {
                this.pageStatus = 'loading';
              })
              .onProgressChange((event: OnProgressChangeEvent) => {
                this.progress = event.newProgress;
              })
              .onPageEnd(() => {
                this.pageStatus = 'loaded';
              })
              .onErrorReceive((event: OnErrorReceiveEvent) => {
                if (event.request.isMainFrame()) {
                  this.pageStatus = 'error: ' + event.error.getErrorInfo();
                }
              })
              .layoutWeight(1)

            Row() {
              Button('Back')
                .fontSize(13)
                .onClick(() => {
                  try {
                    if (this.webController.accessBackward()) {
                      this.webController.backward();
                    }
                  } catch (e) {
                    this.pageStatus = 'back failed';
                  }
                })
              Button('Forward')
                .fontSize(13)
                .margin({ left: 8 })
                .onClick(() => {
                  try {
                    if (this.webController.accessForward()) {
                      this.webController.forward();
                    }
                  } catch (e) {
                    this.pageStatus = 'forward failed';
                  }
                })
              Button('Reload')
                .fontSize(13)
                .margin({ left: 8 })
                .onClick(() => {
                  try {
                    this.webController.refresh();
                  } catch (e) {
                    this.pageStatus = 'reload failed';
                  }
                })
              Text(this.pageStatus)
                .fontSize(12)
                .fontColor('#6b7280')
                .margin({ left: 12 })
                .maxLines(1)
                .textOverflow({ overflow: TextOverflow.Ellipsis })
                .layoutWeight(1)
            }
            .width('100%')
            .padding(12)
          }
          .width('100%')
          .height('100%')
        }
        .tabBar(this.tabBar('WebView', 0))

        // ---------------- Tab 2: JsBridge ----------------
        TabContent() {
          Column() {
            Text('status: ' + this.bridgeStatus)
              .fontSize(13)
              .fontColor('#2563eb')
              .width('100%')
              .padding(8)
            Web({ controller: this.bridgeController, src: $rawfile('greet.html') })
              .javaScriptAccess(true)
              .domStorageAccess(true)
              .fileAccess(true)
              .javaScriptProxy({
                object: this.proxy,
                name: 'kmpNativeBridge',
                methodList: ['call'],
                controller: this.bridgeController,
              })
              .onPageEnd(() => {
                this.injectBridge();
              })
              .layoutWeight(1)
            Scroll() {
              Text(this.bridgeLog.length > 0 ? this.bridgeLog : '(native dispatch log appears here)')
                .fontSize(11)
                .fontColor('#374151')
                .textAlign(TextAlign.Start)
                .width('100%')
            }
            .align(Alignment.TopStart)
            .height(180)
            .width('100%')
            .padding(8)
            .backgroundColor('#f3f4f6')
          }
          .width('100%')
          .height('100%')
        }
        .tabBar(this.tabBar('JsBridge', 1))

        // ---------------- Tab 3: Engine ----------------
        TabContent() {
          Scroll() {
            Column() {
              Text('Kotlin/Native engine (libohoswebview.so)')
                .fontSize(15)
                .fontWeight(FontWeight.Bold)
                .margin({ bottom: 8 })
              Text(this.versionText)
                .fontSize(13)
                .width('100%')
                .padding(8)
                .backgroundColor('#eff6ff')
                .borderRadius(6)
              Text(this.scriptInfo)
                .fontSize(13)
                .width('100%')
                .padding(8)
                .margin({ top: 8 })
                .backgroundColor('#eff6ff')
                .borderRadius(6)

              Button('Format cookie expiration (+24h)')
                .fontSize(13)
                .width('100%')
                .margin({ top: 16 })
                .onClick(() => {
                  this.refreshCookieDate();
                })
              Text(this.cookieDateText.length > 0 ? this.cookieDateText : '(not run yet)')
                .fontSize(13)
                .width('100%')
                .padding(8)
                .margin({ top: 8 })
                .backgroundColor('#f0fdf4')
                .borderRadius(6)

              Button('Build Cookie.toString()')
                .fontSize(13)
                .width('100%')
                .margin({ top: 16 })
                .onClick(() => {
                  this.refreshCookie();
                })
              Text(this.cookieText.length > 0 ? this.cookieText : '(not run yet)')
                .fontSize(13)
                .width('100%')
                .padding(8)
                .margin({ top: 8 })
                .backgroundColor('#f0fdf4')
                .borderRadius(6)

              Text('Note: ArkTS only imports libwebview_napi.so; the Kotlin engine is ' +
                'reached through the @CName C ABI. Never import libohoswebview.so from ArkTS.')
                .fontSize(11)
                .fontColor('#6b7280')
                .margin({ top: 16 })
            }
            .alignItems(HorizontalAlign.Start)
            .width('100%')
            .padding(16)
          }
          .width('100%')
          .height('100%')
        }
        .tabBar(this.tabBar('Engine', 2))
      }
      .scrollable(true)
      .barMode(BarMode.Fixed)
      .onChange((index: number) => {
        this.currentIndex = index;
      })
      .width('100%')
      .layoutWeight(1)
    }
    .width('100%')
    .height('100%')
  }
}

# VSCodium for HarmonyOS PC (hos_vscodium-opensource)

把 **VSCodium 1.126.0**(VS Code 开源发行版)完整移植到 HarmonyOS PC(2in1,aarch64-unknown-linux-ohos)。

基于 openharmony-sig/electron 的 `electron-v37.2.0-openharmony` 运行时(Chromium 138 + Node 22.17.0),**全程无 hnp**,二进制通过 `module.json5` 的 `executableBinaryPaths` 随 HAP 分发(参考[官方 hap-bin 文档](https://developer.huawei.com/consumer/cn/doc/doccenter-getting-started/hap-bin))。

## 功能现状(真机 MateBook Pro 验证)

- ✅ Workbench 完整渲染(窗口/菜单/编辑器/文件树/欢迎页)
- ✅ 集成终端:`forkpty` + `/bin/sh`(自研 ohpty 模块),可选 bash/zsh profile
- ✅ 扩展市场:Open VSX 搜索/安装可用
- ✅ 文件关联:32 种 UTD「用 VSCodium 打开」
- ✅ 全文搜索(内置 OHOS 版 ripgrep)
- ⚠️ 编辑器键盘输入:依赖系统 IME 文本提交,个别场景输入异常(上游 fork 已知问题)
- ⚠️ 退出应用时偶发 dlclose 崩溃(继承自 ZCode/fork 的已知问题)
- ⚠️ `@vscode/spdlog` 已移除(ObjectWrap 在该运行时构造即崩):日志退化为 console;`@vscode/sqlite3` 已由自研纯 C N-API 重实现替代(见下)
- ⚠️ `media(hover: none)` 导致少量 UI 元素不显示(继承自 fork)

## 安装说明

### 前提

- 鸿蒙 PC(2in1 或平板,arm64),已开启开发者模式
- PC 上装好 DevEco command-line-tools(自带 `hdc`),`hdc list targets` 能看到设备

### 方式一:直接安装已签名 HAP(推荐)

从 [Releases](https://gitcode.com/nanqube/hos_vscodium-opensource/releases) 下载对应包名的 HAP,然后:

```bash
hdc install -r vscodium-ohos-nanqu-debug.hap
```

两个包名按需取一(**勿同时安装**):

| 包 | bundleName | 说明 |
|---|---|---|
| `vscodium-ohos-codearts-*.hap` | `com.huawei.codearts.agent` | 蹭系统能力白名单,限制最少;**会覆盖华为 CodeArts 应用**(如有请先确认);覆盖安装若报 9568332,先 `hdc shell bm uninstall -n com.huawei.codearts.agent` 再装 |
| `vscodium-ohos-nanqu-*.hap` | `org.nanqu.hos.code` | 独立身份,可与其他应用共存 |

### 方式二:用自己的签名材料重签

如果你手里是**未签名**的 HAP(例如自己 `devecocli build` 出来但没配签名),可以用仓库根目录的 `sign.py` + `sign.js` 离线重签:

```bash
# 前提:
#  - 已用 devecocli signature generate 生成过自己的签名材料
#    (~/.ohos/config/ 下有对应证书的 cer/p7b/p12 和 material/ 目录)
#  - 项目根目录的 build-profile.json5 的 signingConfigs 指向你的材料
#  - java 和 node 在 PATH 里
export TOOL_HOME=$HOME/Library/Huawei/command-line-tools
python3 sign.py <未签名.hap> <输出-已签名.hap>
```

原理:`sign.js` 是 hvigor `DecipherUtil` 的独立版,负责用 `material/` 里的密钥解出 signingConfigs 中加密的 `keyPassword`/`storePassword`;`sign.py` 再用明文密码调 SDK 的 `hap-sign-tool.jar` 完成本地签名。

## 权限说明

声明于 `web_engine/src/main/module.json5`(随 entry 合并),全部来自引擎层(Electron/Chromium 在鸿蒙上的正常运行所需),应用自身没有额外声明。

### kernel 级(需 ACL 签名,签名时需声明 restricted ACL)

| 权限 | 用途 |
|------|------|
| `ohos.permission.kernel.ALLOW_WRITABLE_CODE_MEMORY` | V8 JIT 需要可写可执行内存页(没有这个 Chromium 起不来) |
| `ohos.permission.kernel.LOAD_INDEPENDENT_LIBRARY` | 加载未随应用注册的独立动态库(libelectron.so / 各 .node 原生模块) |

### system 级(受限)

| 权限 | 用途 |
|------|------|
| `ohos.permission.ACCESS_USER_FULL_DISK` | 编辑器读写用户目录任意文件(IDE 核心能力);不要可自行移除 |
| `ohos.permission.READ_WRITE_USER_FILE` / `READ_WRITE_DOWNLOAD_DIRECTORY` / `READ_WRITE_DOCUMENTS_DIRECTORY` / `READ_WRITE_DESKTOP_DIRECTORY` | 文件打开/保存对话框 |
| `ohos.permission.FILE_ACCESS_PERSIST` | 文件管理器「打开方式」授权持久化 |
| `ohos.permission.CUSTOM_SANDBOX` | 自定义沙箱路径(Chromium 运行时文件布局) |
| `ohos.permission.ALLOW_EXTERNAL_NATIVE_CODE` | 允许加载外部原生代码(.node 模块) |
| `ohos.permission.SYSTEM_FLOAT_WINDOW` / `WINDOW_TOPMOST` | 悬浮窗/置顶(调试面板、画中画类窗口) |
| `ohos.permission.PRIVACY_WINDOW` | 防截屏泄露(隐私窗口) |
| `ohos.permission.PREPARE_APP_TERMINATE` | 退出前清理(杀死终端子进程等) |
| `ohos.permission.ACCESS_CERT_MANAGER` | HTTPS 客户端证书(企业网络/自签场景) |
| `ohos.permission.PRINT` | 打印 |
| `ohos.permission.WEB_NATIVE_MESSAGING` | Web Native Messaging 桥 |
| `ohos.permission.READ_PASTEBOARD` | 读剪贴板(粘贴) |

### normal 级

`INTERNET`(扩展市场/远程)、`GET_NETWORK_INFO`(网络状态监听)、`RUNNING_LOCK`(长时间构建/调试防休眠)、`ACCESS_BIOMETRIC`(WebAuthn)、`GET_FILE_ICON`(文件图标)、`SET_ABILITY_INSTANCE_INFO`(多实例窗口管理)、`LOCK_WINDOW_CURSOR`(沉浸式锁标,definePermissions 声明)

### 用户授权型(运行时按需弹窗)

定位(`LOCATION`/`APPROXIMATELY_LOCATION`/`LOCATION_IN_BACKGROUND`)、麦克风(`MICROPHONE`)、相机(`CAMERA`)、蓝牙(`ACCESS_BLUETOOTH`)、传感器(`ACCELEROMETER`/`GYROSCOPE`)、自定义截屏(`CUSTOM_SCREEN_CAPTURE`)——全部由 Chromium 的 Web 权限模型驱动(网页/扩展请求摄像头麦克风等 Web API 时才会用到),纯编辑场景不会触发。

> 如果你不需要全盘访问,删掉 `ACCESS_USER_FULL_DISK` 后应用仍可工作(文件对话框走系统 picker)。

## 目录结构

```
native/            # 自研原生模块源码与构建脚本(ohpty/napi-dyn/keymapping stub/crash-hook)
```

```
AppScope/            # 应用级配置(bundleName 在此,双包名切换点)
electron/            # entry HAP 模块
  libs/arm64-v8a/    # electron/node 启动器、ohpty(pty.node/.so)、keymapping、watcher 等
  src/main/resources/resfile/
    electron                       # Chromium 子进程 re-exec 启动器
    resources/app/                 # VSCodium 1.126.0(解包目录形态)
      main-shim.mjs                # 启动 shim(见下)
      extensions/ohos-terminal/    # 终端 profile + 默认环境变量
web_engine/          # Chromium/Electron 引擎 HAR(源自 ZCode,43 adapter)
  libs/arm64-v8a/    # libelectron.so / libadapter.so / libffmpeg.so
  src/main/resources/resfile/      # pak/icudtl/snapshot/locales
scripts/build-variants.sh          # 双包名签名构建脚本
```

## 关键移植技术点

### 启动 shim(`resources/app/main-shim.mjs`)

Electron 加载 `resources/app/package.json` 的 `main` → 我们的 shim → 再 import `out/main.js`。shim 做的事:

1. `process.platform` mock 为 `linux`——fork 报 `openharmony`,VS Code 的 `initServices` switch 漏注册 `IUpdateService` 直接自杀
2. `process.title` mock(OHOS 无 setproctitle)
3. `HOME=/storage/Users/currentUser` + `XDG_*` + `VSCODE_PORTABLE`(沙箱内 argv.json:`disable-chromium-sandbox` 等)
4. powerMonitor 监听器守卫(fork 缺 `setListeningForShutdown`)
5. BrowserWindow WCO 方法打桩(`setTitleBarOverlay` 等,另有 `out/main.js` 调用点补丁)
6. 主进程原生模块**预加载**:fork 在窗口/renderer 起来后会销毁其 JS VM,之后再首次加载 .node 会 abort(`ecma_vm destructed`),所以趁早 require 进缓存

### 原生模块(napi-dyn 转发层)

fork 的 libelectron.so 以 RTLD_LOCAL 加载,napi 符号不在全局作用域;直接链接(`-lelectron`)又会和 fork 的 ArkTS VM 转发集混用而崩溃。解法照抄 OHcode 预编译模块的思路:**模块内自带转发层,constructor 里 `dlopen("libelectron.so", RTLD_NOLOAD)` + dlsym 出全部 napi 函数指针**,零未定义 napi 符号。

- 源码与构建脚本:`native/`(ohpty.c、vscode-sqlite3.c、napi-dyn 转发层、keymapping stub、crash-hook 等;依赖 node headers 需自行准备,见 build 脚本头部注释),产物已随 HAP 进 libs
- 现有产物:ohpty(自研)、vscode-sqlite3(自研,见下)、keymapping(OHOS stub,返回 US layout)、watcher/watchdog/iselevated/policy-watcher(OHcode 预编)+ 各自的 `.so` 别名

### sqlite(vscode-sqlite3)

官方 @vscode/sqlite3 的 node-addon-api/ObjectWrap 在该运行时构造即崩。`native/src/vscode-sqlite3.c` 用纯 C N-API + napi-dyn 重实现了 VS Code 存储层用到的 node-sqlite3 子集(Database: exec/close/serialize/configure;Statement: bind/run/get/all/each/reset/finalize;错误对象带 `SQLITE_*` code 属性),静态链接 NqApi 的 OHOS 版 libsqlite3.a。sqlite 调用同步执行保证语句顺序,回调一律经 setImmediate 推迟(对齐上游异步语义,避免 `o = { db: new Database(p, cb) }` 的 TDZ)。注意类方法描述符必须给 `napi_default_method`(可写),否则 `lib/sqlite3.js` 的 prototype 覆写(normalizeMethod)会静默失败。构建:`native/build-sqlite.sh`。

### .so 别名

安装器只给 `*.so` 注册签名,`.node` 会 XPM 校验失败。fork 的 `node_binding.cc` 会在 dlopen 失败时回退同名 `.so`(见 Harmony_VS_Code 的 `patches/electron-node-binding.patch`),所以每个 `.node` 在 libs 里都有一个同内容 `.so`。

### 终端(ohpty)

官方 node-pty 在此运行时上崩溃(其 `ptsname(master)` 在 OHOS 返回 NULL、异常清理路径踩 libc),遂按 VS Code `lib/unixTerminal.js` 实际用到的接口(fork/resize/process)写了个极简实现(纯 C N-API + napi-dyn):`forkpty` + 子进程 `execl("/bin/sh", ...)`(参考 DevPython-OH `terminal_context::Fork`),exit 用 pthread + napi threadsafe function 回抛。

## 构建

```bash
export DEVECO_CLI_CLT_PATH=$HOME/Library/Huawei/command-line-tools

# 签名(两个包名各需一次,设备需在线):
#   1. 把 AppScope/app.json5 的 bundleName 改成目标包名
#   2. devecocli signature generate --product default
#   3. cp ~/.ohos/config/default_HarmonyCode_* dist/signing/<codearts|nanqu>.<ext>
#      (dist/signing/ 还需有 ~/.ohos/config/material 的拷贝)
#   已对 codearts/nanqu 各做一次的机器上直接:

./scripts/build-variants.sh           # debug, 两个包名
./scripts/build-variants.sh release   # release
```

产物:`dist/vscodium-ohos-codearts-{debug,release}.hap`、`dist/vscodium-ohos-nanqu-*.hap`。

- `com.huawei.codearts.agent`:蹭系统对该包名的能力白名单(ZCode 验证过的路径,forkpty/原生加载等限制最少;会覆盖华为 CodeArts 应用)
- `org.nanqu.hos.code`:独立身份,可与之共存

## 调试

```bash
devecocli log --bundle-name <bundle>          # hilog
# 应用内日志(设备上):
#   /data/app/el2/100/base/<bundle>/files/shim-log.txt     主进程 shim
#   .../files/crash-hook.txt                               原生崩溃栈(自研 crash-hook.node)
# Chromium DevTools: electron/libs/arm64-v8a/dev_config.json 里 remote-debugging: true(9333 端口)
#   hdc fport tcp:9333 tcp:9333 → http://127.0.0.1:9333
```

## 致谢与参考

- [ZCode](https://gitcode.com/z-ai/ZCode) — web_engine 引擎层、无 hnp 方案、bin 二进制(bash/zsh/rg)
- [OHcode](https://gitcode.com/nanqube/OHcode) — VS Code 1.85.2 移植先驱,原生模块预编译产物
- [Harmony_VS_Code](https://gitcode.com/nanqube/Harmony_VS_Code)— Code-OSS 1.134 源码级移植补丁、终端桥、签名/部署脚本思路
- [openharmony-sig/electron](https://gitcode.com/openharmony-sig/electron) `electron-v37.2.0-openharmony` 运行时
- [VSCodium](https://github.com/VSCodium/vscodium) 1.126.04524

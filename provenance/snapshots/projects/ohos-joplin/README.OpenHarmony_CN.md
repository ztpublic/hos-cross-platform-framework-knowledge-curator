# Joplin OpenHarmony 版编译与运行指南

本文档介绍开发者如何把本项目的 **OpenHarmony / HarmonyOS** 版本编译并运行起来。

Joplin 是一款开源的笔记与待办应用。本仓库在原有移动端（React Native）的基础上，通过
**RNOH（React Native OpenHarmony）** 将其移植到了 OpenHarmony / HarmonyOS 平台。鸿蒙宿主工程
位于 [packages/app-mobile/harmony](packages/app-mobile/harmony)，React Native 的 JS 入口为
[packages/app-mobile/index.harmony.js](packages/app-mobile/index.harmony.js)。

运行时由两部分组成：

- **JS 侧**：React Native 业务代码，由 Metro 打包成 `bundle.harmony.js`（或开发期由 Metro 服务实时提供）。
- **原生侧**：鸿蒙宿主工程（ArkTS / C++），由 DevEco Studio + hvigor 构建成 HAP 并安装到设备/模拟器。

---

## 一、Clone 即用（只改签名即可运行）

本仓库已经把依赖全部内置了（`node_modules`、鸿蒙 `oh_modules`、RNOH 的 C++ 源码、离线 JS bundle、`.har` 包）。
正常情况下 clone 完不用再跑 `yarn install` / `ohpm install`，唯一要做的就是换成你自己的签名。

```sh
# 1. 必须先装 git-lfs：HAR(约 156MB)和 bundle.harmony.js(约 26MB)等大文件走 LFS 存储，
#    不装 git-lfs 会被 clone 成几十字节的“指针文件”，导致构建失败。
git lfs install

# 2. 克隆（git-lfs 会自动拉取真实大文件）
git clone https://atomgit.com/OpenHarmonyPCDeveloper/ohos_joplin.git
#    若克隆时未启用 LFS，可在仓库内补拉： git lfs pull

# 3. 用 DevEco Studio 打开 packages/app-mobile/harmony 目录
# 4. 改成你自己的签名：File → Project Structure → Signing Configs（见下文「六」签名说明）
# 5. 选 entry 模块 + HarmonyOS PC 目标，点 Run
```

> 想确认大文件是真内容而不是 LFS 指针，可以看一下：`ls -la packages/app-mobile/harmony/local-har/*.har`
> 大小应该是上百 MB；如果只有几十字节，说明 LFS 没拉下来，补一句 `git lfs pull` 即可。
>
> 依赖完整性的更多说明，以及万一碰到 native 编译报错时的兜底修复，见[「九、常见问题」](#九常见问题)。

---

## 二、环境要求

| 工具 | 版本要求 | 说明 |
| --- | --- | --- |
| Node.js | **22**（仓库要求 `>=18`，鸿蒙工程建议用 22） | 见 [packages/app-mobile/.node-version](packages/app-mobile/.node-version) |
| Yarn | **4.12.0** | 仓库通过 `packageManager` 锁定，建议用 corepack 启用 |
| Git | 最新稳定版 | 拉取代码 |
| Git LFS | 最新稳定版 | **必装**。`.har` / `bundle.harmony.js` 等大文件走 LFS，详见[「一」](#一clone-即用只改签名即可运行) |
| DevEco Studio | 支持 HarmonyOS SDK API 21 的版本 | 构建鸿蒙原生工程、安装运行 HAP |
| HarmonyOS SDK | `compatibleSdkVersion 5.0.5(17)`，`targetSdkVersion 6.0.1(21)` | 见 [build-profile.json5](packages/app-mobile/harmony/build-profile.json5) |

几点补充：

- 项目路径里不要带空格，否则构建可能会失败。
- 启用 Yarn 4：

  ```sh
  corepack enable
  corepack prepare yarn@4.12.0 --activate
  ```

- 运行目标用 HarmonyOS PC 模拟器或真机都行。

---

## 三、安装依赖

在仓库根目录执行（这是个 Yarn workspaces 单仓库，必须从根目录装）：

```sh
yarn install
```

该命令会安装所有 workspace 包的依赖，并完成必要的 postinstall 步骤。鸿蒙原生侧的 `.har`
依赖通过 [oh-package.json5](packages/app-mobile/harmony/oh-package.json5) 以本地文件方式引用，其中
RNOH 运行时 `react-native-openharmony-0.82.29.har` 位于
[packages/app-mobile/harmony/local-har](packages/app-mobile/harmony/local-har)，DevEco 打开工程时会自动解析。

> 有个坑建议先看一眼：ohpm 从 RNOH 的 `.har` 安装时，有时会漏解压 React Native 的 C++ 核心源码目录
> `third-party/rn/ReactCommon/`（yoga / jsi / cxxreact 等）。一旦少了它，native 编译会在 CMake 阶段
> 报一长串 `add_subdirectory ... not an existing directory`。好在这些源码在 HAR 里是全的，一条命令就能补回来，
> 具体见[「九、常见问题」](#九常见问题)的第 1 条。

---

## 四、构建 JS（TypeScript / 注入脚本）

Joplin 由 TypeScript 编写，运行前需要把改动的 `.ts/.tsx` 编译为 `.js`。在根目录执行：

```sh
# 一次性编译
yarn tsc

# 或持续监听变更
yarn watch
```

如果修改了笔记编辑器 / 预览器等 WebView 内运行的脚本，需要在 `packages/app-mobile` 下额外构建注入脚本：

```sh
yarn workspace @joplin/app-mobile buildInjectedJs
# 开发期可持续监听：
yarn workspace @joplin/app-mobile watchInjectedJs
```

---

## 五、运行方式

有两种运行模式，二选一即可。

### 模式 A：开发模式（Metro + DevEco，推荐日常开发）

1. 在仓库根目录启动 Metro（鸿蒙平台）：

   ```sh
   yarn workspace @joplin/app-mobile start:harmony
   ```

   这个脚本会设置 `JOPLIN_TARGET_PLATFORM=harmony` 并以 reset-cache 方式启动 Metro，启动后让它一直开着。

2. 用 DevEco Studio 打开 [packages/app-mobile/harmony](packages/app-mobile/harmony) 目录。

3. 选择 `entry` 模块和一个 HarmonyOS 目标（PC 模拟器或真机），点击运行 / 调试。

4. 应用启动时，[entry/src/main/ets/pages/Index.ets](packages/app-mobile/harmony/entry/src/main/ets/pages/Index.ets)
   会按以下顺序依次尝试加载 JS bundle：

   1. 打包内置的 `hermes_bundle.hbc`
   2. 打包内置的 `bundle.harmony.js`
   3. 设备沙箱路径 `/data/storage/el2/base/files/bundle.harmony.js`
   4. Metro 服务 `http://localhost:8081/packages/app-mobile/index.bundle?platform=harmony&dev=true&minify=false`

   开发模式下走的就是第 4 项，所以 Metro 得一直开着。

> localhost 连不上时：PC 模拟器或真机不一定能把 `localhost` 解析到开发机。把
> [Index.ets](packages/app-mobile/harmony/entry/src/main/ets/pages/Index.ets) 里的 Metro 地址改成开发机的
> 局域网 IP（比如 `http://192.168.x.x:8081/...`），再确认 `8081` 端口能访问就行。

### 模式 B：离线打包模式（生成内置 bundle，不依赖 Metro）

若希望产出一个不依赖 Metro 的可安装包，先生成离线 JS bundle：

```sh
yarn workspace @joplin/app-mobile bundle:harmony
```

该命令（见 [tools/buildHarmonyBundle.js](packages/app-mobile/tools/buildHarmonyBundle.js)）会：

- 生成 `packages/app-mobile/harmony/entry/src/main/resources/rawfile/bundle.harmony.js`
- 把 React Native 资源复制到同目录下的 `assets/`
- 把图标字体复制到 `rawfile/fonts/`

完成后回到 DevEco Studio 直接构建 / 运行，应用会加载内置的 `bundle.harmony.js`（上面顺序中的第 2 项），无需启动 Metro。

---

## 六、在 DevEco Studio 中构建并运行 HAP

1. 用 DevEco Studio **打开 [packages/app-mobile/harmony](packages/app-mobile/harmony) 目录**（不是仓库根目录）。
2. 等待 DevEco 解析 `oh-package.json5` 并完成 Sync。
3. 选择 `entry` 模块和目标设备（PC 模拟器 / 真机）。
4. 点击运行（Run）或调试（Debug），DevEco 会用 hvigor 构建 HAP 并安装启动。

首次构建时 DevEco 会用 CMake + ninja 编译 RNOH 与 React Native 的 C++ 源码（yoga / jsi /
cxxreact / folly / boost / hermes 等），生成 `librnoh_*.so` 等原生库，再连同 JS bundle、ArkTS、
资源一起打进 HAP。native 编译产物在 `entry/.cxx/` 和 `entry/build/`，这两个目录删了也会自动重建。

> 可选：在命令行单独编译 native（用来在 DevEco 之外验证 C++ 能编过）。命令行环境得先导出
> `DEVECO_SDK_HOME`，不然会报 `Invalid value of 'DEVECO_SDK_HOME'`：
>
> ```sh
> export DEVECO_SDK_HOME=/Applications/DevEco-Studio.app/Contents/sdk   # 改成你的 DevEco SDK 路径
> cd packages/app-mobile/harmony
> /Applications/DevEco-Studio.app/Contents/tools/node/bin/node \
>   /Applications/DevEco-Studio.app/Contents/tools/hvigor/bin/hvigorw.js \
>   --mode module -p module=entry -p product=default compileNative --no-daemon
> ```

应用信息（见 [AppScope/app.json5](packages/app-mobile/harmony/AppScope/app.json5)）：

- bundleName：`net.cozic.joplin.harmony`
- versionName：`3.7.0`

### 签名说明

[build-profile.json5](packages/app-mobile/harmony/build-profile.json5) 中已内置一套 `default` 调试签名配置，
指向 `~/.ohos/config/` 下的证书 / profile。如果在你的机器上这些文件不存在，请在 DevEco Studio 中通过
**File → Project Structure → Signing Configs** 用自己的账号重新生成自动签名，或替换为你自己的证书路径。

---

## 七、类型检查与校验

鸿蒙平台有独立的 TypeScript 配置 [tsconfig.harmony.json](packages/app-mobile/tsconfig.harmony.json)，
提交前建议执行类型检查：

```sh
yarn workspace @joplin/app-mobile tsc:harmony
```

应用应能通过 [index.harmony.js](packages/app-mobile/index.harmony.js) 正常启动，并完成笔记 / 笔记本的增删改查。

> 首次只显示笔记编辑器时，请先通过侧边栏 / 新建笔记本流程创建或选择一个真实笔记本。

---

## 八、原生桥接约定

JS 侧期望宿主工程注册两个原生能力：

- `JoplinHarmony`：原生模块，契约见 [utils/harmony/HarmonyNative.ts](packages/app-mobile/utils/harmony/HarmonyNative.ts)
  与 [native-contract/JoplinHarmonyModule.d.ts](packages/app-mobile/harmony/native-contract/JoplinHarmonyModule.d.ts)
- `JoplinHarmonyWebView`：原生组件，供
  [components/ExtendedWebView/index.harmony.tsx](packages/app-mobile/components/ExtendedWebView/index.harmony.tsx) 使用，
  契约见 [native-contract/JoplinHarmonyWebView.md](packages/app-mobile/harmony/native-contract/JoplinHarmonyWebView.md)

`entryability` 需在初始化 RNOH 时以组件名 `Joplin` 注册暴露上述模块 / 组件的 package。

---

## 九、常见问题

### 1. native 编译报错 `... third-party/rn/ReactCommon/* not an existing directory`

**现象**：DevEco 构建（或 `compileNative`）在 `BuildNativeWithCmake` 阶段报一长串：

```text
CMake Error at .../third-party/rn/CMakeLists.txt:25 (add_subdirectory):
  add_subdirectory given source ".../third-party/rn/ReactCommon/yoga"
  which is not an existing directory.
... (jsi / cxxreact / callinvoker / cmake-utils 等同样报错)
```

**原因**：ohpm 从 RNOH 的 `.har` 安装时漏解压了 `third-party/rn/ReactCommon/`（RN 的 C++ 核心源码，
约 1300+ 文件）。这个目录是 native 编译必需的，同级的 boost / folly / glog / hermes 一般都正常解压，偏偏
ReactCommon 没出来。又因为它根本不在磁盘上，`git add` 也提交不进去，所以重新 clone 一遍还是会缺。

**修复**：源码在 `local-har/react-native-openharmony-0.82.29.har` 里是全的，从里面解压回安装目录就行：

```sh
cd packages/app-mobile/harmony
HAR="$PWD/local-har/react-native-openharmony-0.82.29.har"
# 自动定位 ohpm 实际安装目录(路径含哈希)
RN_DIR=$(find oh_modules/.ohpm -type d -path '*@rnoh/react-native-openharmony/src/main/cpp/third-party/rn' | head -1)
# 把 ReactCommon 解压回该目录(strip 掉 HAR 里的 package/src/main/cpp/third-party/rn/ 前缀)
tar xzf "$HAR" -C "$RN_DIR" --strip-components=6 package/src/main/cpp/third-party/rn/ReactCommon
# 校验:应能看到 yoga / jsi / cxxreact 等子目录
ls "$RN_DIR/ReactCommon" | head
```

补回后重新构建就能过。顺手把还原出来的 `ReactCommon/` 一起提交进 git，别人 clone 后就不用再踩一遍了。

### 2. `Invalid value of 'DEVECO_SDK_HOME'`（仅命令行构建）

在 DevEco Studio 外用命令行跑 hvigor 时出现。导出 `DEVECO_SDK_HOME` 指向 DevEco 的 SDK 目录即可，
见[「六」](#六在-deveco-studio-中构建并运行-hap)的可选命令。DevEco Studio 内部点 Run 不需要手动设置。

### 3. 其它

- 设备连不上 Metro：见上文「localhost 连不上时」，改用局域网 IP。
- RNOH 与 RN 版本：`@react-native-oh/react-native-harmony@0.82.29` 声明的 peer 依赖是
  `react-native@0.82.1`，而本应用现在用的是 `react-native@0.81.6`。在把 RN 升到 `0.82.1` 或换成兼容
  `0.81.x` 的 RNOH 构建之前，先别把它写死进 `package.json`。
- 构建目录可删了重建：`entry/.cxx/`、`entry/build/` 是 native 构建产物，里面带的是绝对路径，不能跨目录、跨机复用；
  碰到莫名其妙的链接 / 缓存问题，整个目录删掉让 DevEco 重新生成就好。
- 更多构建排错：参考 [readme/dev/build_troubleshooting.md](readme/dev/build_troubleshooting.md)。

---

## 十、功能支持情况

鸿蒙版各功能的适配情况按下面三档能力分组来看。每条都是对着代码核过的，主要参考原生模块
[JoplinHarmonyModule.ets](packages/app-mobile/harmony/entry/src/main/ets/rn/JoplinHarmonyModule.ets)
和各 `*.harmony.ts(x)` 适配层，更细的可以翻
[harmony/README.md](packages/app-mobile/harmony/README.md) 和
[harmony/PORTING_STATUS.md](packages/app-mobile/harmony/PORTING_STATUS.md)。

每张表的「状态」列只有两个值，含义如下：

- **能用**：在鸿蒙真机上实际用过、正常。哪怕带降级或限制（比如 OneDrive 要手动粘贴回 URL、大图不压缩），只要这条链路真能走通，就算能用，受限点写在说明列里。
- **未适配**：桌面端 / 移动原生特有、或平台没有对应概念的能力，原生侧是占位实现（抛错 / 返回空 / no-op）或这一版没纳入。这是按平台特点主动换了做法或没做，是取舍，原因写在说明列里。

下面按 T0 / T1 / T2 三档能力分组来列：T0 是核心链路（启动、本地笔记、加密、主流同步），T1 是带降级或限制但仍能走通的能力，T2 是系统级集成 / 外设这一类多数还没接的能力。

### T0 核心能力（启动、本地笔记、加密、主流同步）

| 功能 | 状态 | 说明 |
| --- | --- | --- |
| 应用启动 / RNOH 运行时加载 JS bundle | 能用 | 见 [pages/Index.ets](packages/app-mobile/harmony/entry/src/main/ets/pages/Index.ets) |
| 笔记查看 / Markdown 编辑器 / 富文本 / 绘图 / 插件面板（WebView） | 能用 | RNOH 社区 WebView（`@react-native-ohos/react-native-webview`），注入 JS / postMessage 走真实实现，见 [ExtendedWebView/index.harmony.tsx](packages/app-mobile/components/ExtendedWebView/index.harmony.tsx) |
| 本地数据库：笔记 / 笔记本 / 标签 增删改查、事务、搜索 | 能用 | `sqliteOpen/Exec/SelectAll`（原生 `relationalStore`） |
| 本地文件存储、profile 初始化、附件落盘 | 能用 | `fsWriteFile/ReadFile/Stat/...`（原生 `@ohos.file.fs`） |
| 随机数 / 摘要（MD5、SHA-1/256/384/512） | 能用 | `randomBytes` / `cryptoDigest`（原生 `cryptoFramework`） |
| 端到端加密：AES-GCM 加解密 + PBKDF2 派生 | 能用 | `cryptoEncrypt/cryptoDecrypt`，JS 侧 [crypto.harmony.ts](packages/app-mobile/services/e2ee/crypto.harmony.ts) |
| 端到端加密：RSA 生成 / 加 / 解密 | 能用 | `rsaGenerateKeyPair/rsaEncrypt/rsaDecrypt`，JS 侧 [RSA.react-native.harmony.ts](packages/app-mobile/services/e2ee/RSA.react-native.harmony.ts) |
| 同步用的二进制资源 HTTP 下载 / 上传 | 能用 | `fetchBlob` / `uploadBlob`（原生 `@kit.NetworkKit`） |
| 同步：Joplin Cloud / WebDAV / Nextcloud / 本地文件系统（账号密码 / Token 方式） | 能用 | 核心 HTTP 链路与 E2EE 加密打通，账号密码 / Token 配置可用 |
| 选择文件 / 选择图片视频（系统选择器） | 能用 | `pickDocument`（`DocumentViewPicker` / `PhotoViewPicker`） |
| 把笔记 / 附件分享到其他应用（分享导出，Share-out） | 能用 | `shareFile`（`startAbility` sendData） |
| 打开外部链接 | 能用 | `openUrl` |
| 原生文本输入弹窗（如重命名 / 新建） | 能用 | `promptForText`，[TextPromptService](packages/app-mobile/harmony/entry/src/main/ets/rn/JoplinHarmonyTextPromptService.ets) |

### T1 带降级或限制的能力

| 功能 | 状态 | 说明 |
| --- | --- | --- |
| 同步：OneDrive | 能用 | 要把登录后的重定向 URL 或 `code` 手动粘贴回登录页（见 [onedrive-login.harmony.tsx](packages/app-mobile/components/screens/onedrive-login.harmony.tsx)）。原生的 OAuth 重定向捕获（`openAuthUrl`）没实现，调用会抛错，所以走手动粘贴。 |
| 语音输入（系统语音转文字） | 能用 | 原生用 CoreSpeechKit 实现了 `speechToText`（见 [SpeechToTextBanner.harmony.tsx](packages/app-mobile/components/voiceTyping/SpeechToTextBanner.harmony.tsx)），依赖在线识别和麦克风权限；Whisper 离线模型是关掉的。 |
| 拍照添加附件 | 能用 | `takePicture` 现在是打开图库选择，没调相机实拍。 |
| 大图附件 | 能用 | 原生缩放 `resizeImage` 没实现（抛错），会回退成直接附带原图，能用但不压缩。 |
| 网络状态 | 能用 | 能探测通不通，但区分不了是不是计费 / 移动数据网络，所以「移动数据流量提醒」不准。 |

### T2 系统级集成 / 外设

| 功能 | 状态 | 说明 |
| --- | --- | --- |
| 提醒 / 闹钟 / 本地通知 | 未适配 | `alarmSchedule/Clear` 是 no-op，`alarmHasPermissions/RequestPermissions` 一律返回 `false`，没适配。 |
| 生物识别解锁（指纹 / 人脸） | 未适配 | `biometricAuthenticate` 返回 `not_available`，`biometricSensorInfo` 报无硬件，没适配。 |
| 录音添加音频附件 | 未适配 | `audioStartRecording/StopRecording` 抛错，没适配（这跟上面的语音转文字是两回事）。 |
| 系统分享导入（Share-in，从别的应用分享进 Joplin） | 未适配 | `shareData()` 返回空对象、`shareClose()` 是 no-op，还没接 Want 交接。 |
| 导入 / 导出 JEX、加密压缩包、tar/zip 解包打包 | 未适配 | `fsTarExtract/fsTarCreate/fsZipExtract` 都抛错，没适配。 |
| 快捷操作（Quick actions / 桌面快捷方式） | 未适配 | [setupQuickActions.harmony.ts](packages/app-mobile/setupQuickActions.harmony.ts) 直接返回 `null`，没适配。 |
| Whisper 离线语音输入 | 未适配 | [whisper.harmony.ts](packages/app-mobile/services/voiceTyping/whisper.harmony.ts) 里 `supported() === false`、`build()` 抛错，已禁用。 |
| 应用内重启 | 未适配 | `restartApp` 是 no-op。 |

**小结**：能用的核心链路是本地笔记的创建 / 编辑 / 管理 / 搜索、端到端加密、账号密码类同步、附件选择与分享导出，以及 OneDrive、语音输入、拍照、大图附件这些带降级或限制但仍能走通的能力。提醒通知、生物识别、录音、系统分享导入、JEX / 压缩包导入导出、快捷操作这些系统级集成是按平台情况未适配。

---

## 相关文档

- 鸿蒙宿主工程说明：[packages/app-mobile/harmony/README.md](packages/app-mobile/harmony/README.md)
- 移植进度：[packages/app-mobile/harmony/PORTING_STATUS.md](packages/app-mobile/harmony/PORTING_STATUS.md)
- 通用构建文档：[readme/dev/BUILD.md](readme/dev/BUILD.md)

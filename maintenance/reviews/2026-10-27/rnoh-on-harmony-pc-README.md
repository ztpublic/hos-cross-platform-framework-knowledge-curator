# rnoh-on-harmony-pc

在一台 **HarmonyOS PC** 上，用 **RNOH**（React Native for OpenHarmony）从零跑通一个 React Native 应用的完整工程。

本仓库是以下这篇实战文章的配套代码，文章记录了整个环境搭建过程的每一步排查：

> **[《鸿蒙 PC 上用 RNOH 开发开源鸿蒙应用》](docs/鸿蒙PC上用RNOH开发开源鸿蒙应用.md)**

应用在 HUAWEI MateBook Pro（HAD-W32，HarmonyOS 7.0）上的实际运行效果：

![运行效果](docs/images/screenshot-harmony-pc.jpeg)

---

## 版本配套

| 组件 | 版本 |
| --- | --- |
| React Native | 0.84.1 |
| @react-native-oh/react-native-harmony | 0.84.4 |
| @react-native-oh/react-native-harmony-cli | 0.84.4 |
| OpenHarmony SDK | API 26（26.0.0.105） |
| ohpm | 6.0.1 |
| hvigorw | 7.26.8 |
| hdc | 3.2.0f |
| Node.js | v26.12.0（要求 ≥ 22.11.0） |

> RNOH 是**多版本线并行**的项目。本仓库对应官方 `0.84-stable` 分支，**不要混用其他版本线的文档与包**。

## 环境要求

- **Node.js ≥ 22.11.0**
- **OpenHarmony SDK**（命令行工具链由 HNP 包 `hmos-clt` 提供，含 `ohpm` / `hvigorw` / `hdc` / SDK；装 DevEco Studio 亦可）
- **git**
- 构建 HAP **不需要 JDK**（SDK 的 `hap-sign-tool` 与 `ohos_packing_tool` 均为原生可执行文件）

需要配置的环境变量：

```bash
export DEVECO_SDK_HOME="<工具链根目录>/sdk"
export RNOH_C_API_ARCH=1        # RNOH 走 CAPI 架构，必须为 1
export HDC_SERVER_PORT=7035     # hdc 服务端口
```

---

## 目录结构

```
rnoh-on-harmony-pc/
├── App.tsx                     # RN 入口页面（已移除 safe-area 依赖）
├── metro.config.js             # 已接入 createHarmonyMetroConfig
├── package.json                # 含 "dev": "react-native bundle-harmony --dev"
├── index.js
├── android/  ios/              # RN 模板自带，本仓库未使用
├── docs/
│   ├── 鸿蒙PC上用RNOH开发开源鸿蒙应用.md    # 配套实战文章
│   ├── RNOH-环境说明.md                    # 环境信息与踩坑速查
│   └── images/
└── harmony/                    # 鸿蒙工程
    ├── build-profile.template.json5        # 配置模板（需派生为 build-profile.json5）
    ├── hvigor/hvigor-config.json5          # 引入 @rnoh/hvigor-plugin
    ├── AppScope/app.json5                  # bundleName: com.nutpi.myapplication
    └── entry/
        └── src/main/
            ├── cpp/                        # CMakeLists.txt + PackageProvider.cpp
            ├── ets/pages/Index.ets         # 含 bundle.harmony.js 回落加载
            └── resources/rawfile/          # bundle 产物落在这里（不入库）
```

---

## 快速开始

### 1. 安装 JS 依赖

```bash
npm install
npm install @react-native-oh/react-native-harmony@0.84.4 \
            @react-native-oh/react-native-harmony-cli@0.84.4
```

> ⚠️ **在 Node 22+ 上，`react-native-harmony-cli` 需要打一个兼容补丁**，否则 `init-harmony` 等命令会崩
> （它用了已被移除的 `fs.Dirent.path`）。详见[文章 7.3–7.4 节](docs/鸿蒙PC上用RNOH开发开源鸿蒙应用.md)。
> 修改 `node_modules/@react-native-oh/react-native-harmony-cli/dist/io/RealFS.js`：
>
> ```diff
> -return new core_1.AbsolutePath(this.nodeFSDirent.path).copyWithNewSegment(this.nodeFSDirent.name);
> +const parentPath = this.nodeFSDirent.parentPath ?? this.nodeFSDirent.path;
> +return new core_1.AbsolutePath(parentPath).copyWithNewSegment(this.nodeFSDirent.name);
> ```
>
> 该补丁位于 `node_modules`，每次 `npm install` 后需重新应用（建议用 `patch-package` 固化）。

### 2. 安装鸿蒙侧依赖

```bash
cd harmony
ohpm install --all
```

### 3. 配置签名（必读）

`harmony/build-profile.json5` **不在仓库中**——它包含开发者独有的签名凭据，被 `.gitignore` 排除。

请由模板派生：

```bash
cp harmony/build-profile.template.json5 harmony/build-profile.json5
```

然后填入你自己的 `signingConfigs`。鸿蒙签名需要华为签发的 Profile（`.p7b`），通常通过
DevEco Studio 的 `File > Project Structure > Signing Configs` + 登录华为账号自动生成。
具体做法见[环境说明第 8 章](docs/RNOH-环境说明.md)。

> 若沿用与本仓库相同的 `bundleName`（`com.nutpi.myapplication`），请确认你的签名 Profile
> 也绑定了该 bundle name，否则签名会失败。

### 4. 生成 JS bundle

```bash
cd ..          # 回到仓库根目录
npm run dev
```

产物：`harmony/entry/src/main/resources/rawfile/bundle.harmony.js`（该产物不入库，需自行生成）

### 5. 构建 HAP

```bash
cd harmony
hvigorw assembleHap --no-daemon
```

产物：`harmony/entry/build/default/outputs/default/entry-default-signed.hap`

> **首次构建需要全量编译 C++**（RN + RNOH + folly + hermes + boost 等），约 **25~30 分钟**。
> 之后不动 `cpp/` 就是增量，很快。

### 6. 安装到设备并运行

```bash
hdc list targets
hdc install -r harmony/entry/build/default/outputs/default/entry-default-signed.hap
hdc shell aa start -a EntryAbility -b com.nutpi.myapplication
```

### 7. 开发调试：用 Metro 热加载

`Index.ets` 的加载顺序是 **Metro → hermes_bundle.hbc → bundle.harmony.js**，所以起着 Metro 就能热加载：

```bash
npm start          # 监听 8081
```

---

## 几个容易踩的点

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| `init-harmony` 崩：`paths[0] must be of type string` | CLI 用了 Node 22+ 已移除的 `fs.Dirent.path` | 打上面的 RealFS.js 补丁 |
| 报 `00303027 Configuration Error: modelVersion` | `hvigor-config.json5` 与 `oh-package.json5` 的 `modelVersion` 不一致 | 两处保持一致 |
| 编译报 `cannot open file ...: Interrupted system call` | 高并发编译下文件系统偶发 EINTR，**非代码问题** | 直接重跑 `hvigorw assembleHap`，ninja 会增量推进 |
| 链接时报找不到 `.o` / `libfolly.a` | 构建**期间**改动了源码，ninja 日志与实际文件脱节 | 构建期间别改代码；必要时 `rm -rf entry/.cxx entry/build` 重建 |
| 装上了、启动了、但白屏 | `Index.ets` 只找 `hermes_bundle.hbc`，而 `npm run dev` 产出的是 `bundle.harmony.js` | 本仓库已修复（加了回落加载） |
| 鸿蒙上白屏 | RN 0.84 模板的 `App.tsx` 依赖 `react-native-safe-area-context` | 本仓库已改为显式传 `safeAreaInsets` |

完整排查过程见 [docs/](docs/) 下的两篇文档。

---

## 相关链接

- RNOH 官方仓库：[CPF-RN/ohos_react_native](https://atomgit.com/CPF-RN/ohos_react_native)
- 官方《环境搭建》：[0.84-stable 分支](https://atomgit.com/CPF-RN/ohos_react_native/blob/0.84-stable/docs/zh-cn/02-开发/01-环境与工具/环境搭建.md)
- 官方《版本说明》：[dist-tag 规则与安装方式](https://atomgit.com/CPF-RN/ohos_react_native/blob/0.84-stable/docs/zh-cn/05-运维/版本说明.md)
- React Native：[reactnative.dev](https://reactnative.dev)
- OpenHarmony：[openharmony.cn](https://www.openharmony.cn)

## License

本仓库为示例工程，随 RNOH 生态使用。RNOH 相关代码版权归 Huawei Technologies Co., Ltd. 所有（MIT）。

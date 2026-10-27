# Kamel · OpenHarmony 适配说明

> 这是对上游 Kamel-Media/Kamel `1.0.9`（Apache-2.0）的 **Compose Multiplatform** 鸿蒙适配。Kamel 是 CMP 生态里的异步媒体（图片）加载库：load / cache / decode / display 四段管线，网络抓取走 Ktor，UI 侧卖的是 `KamelImage` composable 和 `asyncPainterResource`。
>
> 这次适配**不是**用 ArkTS 重写一个图片加载器，也**不是**把 Compose 的 `Painter` 硬接到鸿蒙的 Compose 移植线上。它走的是 CPF-KMP-CMP 社区已经验证过的那条路：**加载与缓存核心（纯 Kotlin）编进 ohos so，Compose UI 与 Skia 解码部分物理隔离不编译，ArkTS 侧用系统 image kit 解码显示**。
>
> 仓库首页：[README.md](README.md)。英文摘要：[README.OpenHarmony.md](README.OpenHarmony.md)。上游文档留档：[README.upstream.md](README.upstream.md)。
>
> 本 fork 仓库：[https://atomgit.com/oh-tpc/Kamel](https://atomgit.com/oh-tpc/Kamel)，版本 `1.0.9-ohos.1`。本文不放任何第三方镜像站链接。
>
> 欢迎加入 KMP&CMP 鸿蒙社区：[https://atomgit.com/CPF-KMP-CMP](https://atomgit.com/CPF-KMP-CMP)

本文按「从 0 到 1」把这条适配链路写清楚：为什么选 CMP 库、为什么不在鸿蒙上直接编 Compose 源码、六阶段怎么走、Gradle / Kotlin 版本怎么钉死、C ABI 怎么落到 ArkTS、example 怎么分层验收、踩过哪些坑、目前还有哪些限制。环境安装不展开长文：JDK 21 用 Eclipse Temurin（`gradle.properties` 里 `org.gradle.java.home` 已钉死，但 `gradlew` 启动前仍需设置 `JAVA_HOME` 环境变量）；HarmonyOS 工程用 DevEco Studio 打开 `example/harmonyApp`；Kotlin/OHOS 编译器走社区 Maven `https://maven.eazytec-cloud.com/nexus/repository/maven-public/`，版本钉 `2.4.20-0.1.0-07`。

---

## 1. 这是什么，以及它不是什么

Kamel 的上游形态很典型：业务代码里一个 `KamelImage(url)` 或 `asyncPainterResource(url)`，库负责把 URL 变成 `Resource<Painter>`——抓取（Fetcher）、缓存（内存 LRU + 磁盘）、解码（Skia `ImageBitmapDecoder`、`SvgDecoder`、`AnimatedImageDecoder`）、显示（Compose `Image`）一条龙。它在 CMP 应用里扮演的角色相当于 Android 世界的 Coil。

对鸿蒙适配来说，这个库值钱的地方分三层，而三层的价值不一样：

1. **加载与缓存核心是纯 Kotlin。** `kamel-core` 的 `Cache` / `LruCache` / `DiskCacheStorage` / `DiskLruCache` / `HttpFetcher` / `Mapper` 体系只依赖 Kotlin、coroutines、Ktor、okio，不碰 Compose。给鸿蒙加 `ohosArm64()` / `ohosX64()`，这部分可以直接编进 so。
2. **解码与显示强绑 Compose / Skia。** `ImageBitmapDecoder` 依赖 Skia，`KamelImage` 依赖 `androidx.compose.foundation`，上游 1.0.9 用的是 Compose Multiplatform 1.10.0 / Kotlin 2.3.0。鸿蒙工具链的 Compose 移植线版本与其不齐，直接编 Compose 源码在当前不可行。
3. **鸿蒙设备上本来就有系统解码器。** 系统 image kit 能统一处理 PNG / JPEG / WebP / GIF / SVG。适配的正确姿势不是在 Kotlin/Native 里再养一套 Skia，而是把「抓取 + 缓存」的产物（原始字节）交给 ArkTS，解码显示用系统 kit。

四条常见路线为什么都不选：

| 路线 | 为什么不选 |
|---|---|
| 在 ArkTS 里重写图片加载库 | 白做。上游已经有完整、可测的缓存/抓取/映射核心。适配的意义是复用，不是翻译。 |
| 把 `KamelImage` / `asyncPainterResource` 原样编进 ohos | 上游 CMP 1.10.0 / Kotlin 2.3.0；鸿蒙 Compose 移植线代际不齐。强行对齐等于再打一套 Compose 发行版。 |
| ohos actual 直接产 `Painter` 塞给 ArkUI | ArkUI 的 `Image` 组件吃 `PixelMap` / 资源串，不吃 Compose `Painter`。硬接 Painter 需要整条 Compose runtime，回到上一行的问题。 |
| KN 里链 Skia 自己解码 | 系统 image kit 就在设备上，PNG/JPEG/WebP/GIF/SVG 全覆盖。在 so 里再塞一份解码器只会让 18MB 的产物更胖。 |

一句话：**让 Kamel 的加载/缓存核心在 HarmonyOS 上直接跑，把原始字节过桥交给 ArkTS，用系统 image kit 解码成 `PixelMap` 交给 ArkUI `Image` 显示。**

业务里真实会碰到的场景，几乎都能落到这条管线上：

- **网络图列表：** 内存 LRU 命中 `source=memory`，滑回来看过的图不再走网络。
- **磁盘缓存省钱：** Ktor `HttpCache` 透明落盘（okio POSIX），杀进程重启后同样的 URL 不再重新下载完整响应。
- **SVG 图标 / 占位图：** `placehold.co` 这类 SVG 资源，系统 image kit 直接解析。
- **自定义请求头 / 超时：** 桥的 `headers` / `timeoutMs` 字段透传给 Ktor 请求构建器。

---

## 2. 状态与验证环境

| 项 | 值 |
|---|---|
| 上游版本 | Kamel 1.0.9（Apache-2.0） |
| 本 fork | 1.0.9-ohos.1 |
| Kotlin | HarmonyOS Kotlin `2.4.20-0.1.0-07`（eazytec 社区 Maven） |
| Gradle | 8.14.1（Wrapper 钉死） |
| JDK | Temurin 21.0.12（`org.gradle.java.home` 已写进 `gradle.properties`；`gradlew` 启动前需 `JAVA_HOME`） |
| 核心依赖 | kotlinx-coroutines `1.11.0-0.1.0-07`、okio `3.17.0-0.1.0-07`、ktor `3.5.1-0.1.0-01`（含 ohosArm64/ohosX64 klib；HTTP 引擎 `ktor-client-curl` 同版本） |
| 上游原工具链 | Kotlin 2.3.0 / CMP 1.10.0 / ktor 3.3.3（本 fork 因鸿蒙移植线代际全部更换，见第 7 节） |
| 参与构建模块 | 仅 `:kamel-core`（jvm + ohosArm64 + ohosX64）与 `:example:nativeApp`（桥，仅 ohos 目标） |
| 鸿蒙产物 | `libohoskamel.so`（arm64 / x86_64）+ 匹配 `libc++_shared.so`，由 `copyKnSoArm64/X64` 自动拷入 `example/harmonyApp/entry/libs/`（该目录不入库） |
| Demo | `example/harmonyApp`，bundle `org.terminator.ohos.kamel`，versionName `1.0.9-ohos.1`，compatibleSdkVersion `5.1.0(18)`，申请 `ohos.permission.INTERNET` |
| 桥接符号 | `@CName` × 3 → C ABI：`KamelInit` / `KamelLoad` / `KamelCacheInfo` |
| ArkTS NAPI | CMake 包装库 `libkamel_napi.so`（ArkTS 只 import 它，绝不直接 import KN so） |
| 许可证 | Apache-2.0 |
| 宿主签名 | `build-profile.json5` 的 `signingConfigs` 为空数组 + `"signingConfig": "default"`，DevEco 本机自动签名，不提交 p12 |

`gradle.properties` 相对上游新增两行，都是鸿蒙目标必需：

```properties
kotlin.native.ignoreDisabledTargets=true
kotlin.native.distribution.downloadFromMaven=true
```

前者让 Gradle 跳过本 fork 未启用的 android/apple/js 目标，后者允许 KN 发行版从 Maven 拉取。

本机已跑过并通过的命令：

```text
.\gradlew.bat :kamel-core:compileKotlinJvm :kamel-core:compileTestKotlinJvm
.\gradlew.bat :kamel-core:compileKotlinOhosArm64 :kamel-core:compileKotlinOhosX64
.\gradlew.bat :example:nativeApp:linkDebugSharedOhosArm64
.\gradlew.bat :example:nativeApp:linkDebugSharedOhosX64
```

结果：

- `:kamel-core` 的 JVM 主代码与测试代码编译通过（JVM 目标保留就是为了交叉验证）。
- 两个 ohos 目标的 klib 编译通过。Compose 文件已物理隔离进 `src/composeMain`，ohos 编译器看不到 `androidx.compose`。
- `linkDebugSharedOhosArm64` 与 `linkDebugSharedOhosX64` 均成功（合计约 1m28s），`copyKnSoArm64/X64` 作为 `finalizedBy` 自动把 `libohoskamel.so` 与对应 triple 的 `libc++_shared.so` 拷进 `example/harmonyApp/entry/libs/{arm64-v8a,x86_64}`。
- `llvm-nm` 符号验收通过：arm64 与 x64 两份 `libohoskamel.so` 均导出 `T KamelInit` / `T KamelLoad` / `T KamelCacheInfo` / `T libohoskamel_symbols`（全局 T 符号）。

### 实测产物清单

| 产物 | 位置 | 大小 | 说明 |
|---|---|---|---|
| `libohoskamel.so` (arm64) | `entry/libs/arm64-v8a/` | ≈18.9 MB | debug 未裁剪，含 KN 运行时 + Ktor + curl |
| `libohoskamel.so` (x86_64) | `entry/libs/x86_64/` | ≈18.2 MB | 模拟器用 |
| `libc++_shared.so` (arm64) | `entry/libs/arm64-v8a/` | ≈1.85 MB | 与 KN so 同 triple，`dlopen` 必需 |
| `libc++_shared.so` (x86_64) | `entry/libs/x86_64/` | ≈1.73 MB | 同上 |
| `libkamel_napi.so` ×2 ABI | CMake 产物 | 很小 | NAPI 包装层，只做转发与内存管理 |

**设备层验收已通过**（DevEco 模拟器，API 5.1.0(18)）：hvigor 编译 / `entry-default-unsigned.hap` 打包（26.2 MB，含双 ABI 六个 so 与 `rawfile/cacert.pem`）、`hdc install` 安装、页面运行。功能实测：

| 验收点 | 实测结果 |
|---|---|
| HTTPS 首次加载 | `加载成功 source=network byteCount=7859 elapsedMs=3236`，图片显示 |
| 同一 URL 二次加载 | `加载成功 source=memory byteCount=7859 elapsedMs=11`（内存 LRU 命中） |
| SVG 预设 | `加载成功 source=network byteCount=2268`，系统 image kit 直接解码显示 |
| TLS 证书 | curl 引擎经 `caInfo`（内置 Mozilla CA bundle）验证通过，见第 7 节第 9 条 |

真机（物理设备）复测与断网磁盘缓存验证见第 12 节与第 25 节。

---

## 3. 六阶段路线图

### 阶段 0：查重

CPF-KMP-CMP 社区已落地的库列表里没有 Kamel。图片加载是 CMP 应用的刚需件（Vico、Reorderable、cmp_qrose 之外再补一块基础设施），不与既有适配重复。

### 阶段 1：钉工具链

`ohosArm64()` / `ohosX64()` 只存在于 HarmonyOS Kotlin 插件。`settings.gradle.kts` 的 `pluginManagement` 与 `dependencyResolutionManagement` 两个仓库块顶部各加 eazytec 的 `maven-public` 与 `content/groups/harmonyos` 两个 Maven 源，再接 `gradlePluginPortal()` / `mavenCentral()` / `google()`。

版本钉法不是随意挑的，是被代际逼出来的：ktor 的 ohos 移植版只有 `3.5.1-0.1.0-01` 一代，它及其依赖是 Kotlin 2.4.20 代编译的；coroutines `1.11.0-0.1.0-07` 的元数据版本是 2.4.0，旧编译器读不了。所以 Kotlin 钉 `2.4.20-0.1.0-07`，coroutines / okio 用 `-0.1.0-07`，ktor 用 `3.5.1-0.1.0-01`。Gradle Wrapper 钉 8.14.1，JDK 钉 Temurin 21。上游 1.0.9 原本的 Kotlin 2.3.0 / CMP 1.10.0 / ktor 3.3.3 那套在鸿蒙上直接 sync 会死在配置期或元数据解析期。

### 阶段 2：拆源码集

把所有 `import androidx.compose.*` 的文件 `git mv` 到 `kamel-core/src/composeMain/kotlin/...`。`composeMain` 不是 Kotlin 源集，不参与任何 target 编译。留下来是为了对照上游 API，也方便社区 CMP 与上游 1.10.0 对齐后接回。同时给 `ResourceConfig` 瘦身（删两个 Compose 字段）、重写 `LruCache`（cache4k 拉不到）、新增非 Compose 公共入口 `Kamel.kt`。明细见第 14 节。

### 阶段 3：加鸿蒙目标

`kamel-core` 只声明 `jvm()` + `ohosArm64()` + `ohosX64()`（jvm 用于交叉验证）。**库模块不开 `binaries.sharedLib`**——sharedLib 只放 `example/nativeApp`，否则每个模块都吐一份带 KN 运行时的 so，体积和符号都会乱。其余模块（kamel-samples、kamel-image、kamel-image-default、kamel-decoder/*、kamel-fetcher/*、kamel-mapper/*）全部移出 `settings.gradle.kts` 的 include，文件留在仓库但不参与 sync。

### 阶段 4：桥

`example/nativeApp` 里用 `@CName` 导出三个 C 符号（`KamelInit` / `KamelLoad` / `KamelCacheInfo`），配一个手写 MiniJson 做顶层扁平 JSON 解析。`KamelLoad` 内部 `runBlocking` + `withTimeoutOrNull`，同步阻塞返回；HTTP 引擎用 `ktor-client-curl` 3.5.1-0.1.0-01。再由 CMake 编一层 `libkamel_napi.so`，ArkTS 只 import 这层。协议明细见第 5 节。

### 阶段 5：HAP Demo

`example/harmonyApp` 是 DevEco Stage 工程：`module.json5` 申请 `ohos.permission.INTERNET`（cmp_qrose 无网络不需要，Kamel 是网络加载库，这个权限是硬前提）；`abiFilters` 同时带 `arm64-v8a` + `x86_64`；加载走 `worker.ThreadWorker`（`entry/ets/workers/KamelWorker.ets`），解码用 image kit，页面状态行做验收输出。

---

## 4. 目录与职责

```
kamel-core/src/commonMain/           加载/缓存核心：Cache、LruCache、DiskCacheStorage、DiskLruCache、
                                     HttpFetcher、Mapper 体系、ResourceConfig、新 Kamel 入口
kamel-core/src/composeMain/          ImageLoading.kt、AnimatedImage.kt、KamelConfig、KamelConfigBuilder、
                                     ResourceConfigBuilder、ConfigUtils（不参与任何 target 编译）
kamel-core/src/composeMain/test/     KamelConfigBuilderTest 等 5 个依赖 compose resources 的测试
kamel-core/src/ohosMain/             5 个 actual：Platform / FileFetcher / FileUrlFetcher / Mappers / httpCache
kamel-core/src/jvmMain/              httpCache.desktop.kt（自 desktopMain 迁入，os.name 判定）
example/nativeApp/src/ohosMain/      Bridge.kt：@CName × 3 + MiniJson + runBlocking/withTimeoutOrNull
example/harmonyApp/                  DevEco Stage 工程：CMake NAPI + KamelWorker + Index.ets
README.md / README.upstream.md /     README 四件套：fork 主 README、上游留档、英文摘要、
README.OpenHarmony.md / _CN.md       本文（中文主文档）
```

其余目录是上游工程遗留，**没有**进 `settings.gradle.kts`：`kamel-samples`、`kamel-image`、`kamel-image-default`、`kamel-decoder/*`、`kamel-fetcher/*`、`kamel-mapper/*`、`kamel-sample-ios`，以及 `kamel-core` 里的 androidMain / appleMain / jsMain / wasmJsMain 源码集。不要用它们在鸿蒙上 sync。

---

## 5. 桥接协议

KN so `libohoskamel.so` 导出三个 C 符号。`llvm-nm -D` 已验证双 ABI 均为 T 全局符号：

```text
T KamelInit
T KamelLoad
T KamelCacheInfo
T libohoskamel_symbols
```

ArkTS 侧一律 import `libkamel_napi.so`（CMake NAPI 包装层），由它转发到上述符号。三个函数的协议：

### 5.1 KamelInit —— 初始化

```c
const char* KamelInit(const char* config);
```

请求：

```json
{"cacheDir":"/path/to/app/cache","memoryCacheSize":100,"diskCacheBytes":10485760,"caBundlePath":"/path/to/app/files/cacert.pem"}
```

响应：

```json
{"ok":true,"cacheDir":"/path/to/app/cache"}
{"ok":false,"error":"..."}
```

- **必须在首次加载前调用一次**，落点在 EntryAbility 或页面 `aboutToAppear`（demo 在 `Index.ets` 的 `aboutToAppear` 里做，`cacheDir` 取应用 `cacheDir`）。
- `memoryCacheSize` 缺省 100，桥内收敛到 [0, 1000]；`diskCacheBytes` 缺省 10 MiB，收敛到 [0, 512 MiB]。
- `caBundlePath` 可选但 HTTPS 必需：PEM 格式 CA 根证书文件路径（CURLOPT_CAINFO）。demo 用 `getRawFileContentSync` 读 `rawfile/cacert.pem` 写到 `filesDir` 后传入；不传或传空时 HTTPS 会以 `CURL_PEER_FAILED_VERIFICATION` 失败（见第 7 节第 9 条）。
- 可以在主线程直调（瞬时操作）。

### 5.2 KamelLoad —— 加载

```c
const char* KamelLoad(const char* request);
```

请求：

```json
{"url":"https://picsum.photos/300/200","headers":"a:1;b:2","timeoutMs":15000}
```

响应（成功 / 失败）：

```json
{"ok":true,"url":"...","source":"memory","byteCount":12345,"elapsedMs":45,"base64":"..."}
{"ok":false,"url":"...","error":"...","elapsedMs":45}
```

- `headers` 是 `name:value` 以 `;` 连接的**扁平字符串**。不做成嵌套 JSON 对象，是因为桥内 JSON 解析器 MiniJson 只做顶层扁平解析（与 cmp_qrose 相同的自写 MiniJson 策略，避免为一个头字段拉 kotlinx.serialization）。
- `timeoutMs` 缺省 15000，桥内收敛到 [1000, 120000]；超时返回 `ok:false` 且 `error` 为 `timed out after ...ms`。
- 返回的 `base64` 是**原始图片字节**（PNG / JPEG / WebP / GIF / SVG 原文），不做任何解码。解码是 ArkTS 侧 image kit 的事。
- `source` 取值 `memory | disk | network`（枚举上还有 `none`，正常流程不出现）：内存 LRU 命中即 `memory`；其余走 `HttpUrlFetcher` + Ktor `HttpCache` 磁盘缓存。
- **同步阻塞调用**。内部 `runBlocking` + `withTimeoutOrNull`，网络 IO 可能数秒，ArkTS 必须放 Worker 线程（demo 用 `worker.ThreadWorker` + `entry/ets/workers/KamelWorker.ets`，主线程 `postMessage` 请求字符串、Worker 回传响应字符串）。在主线程直调会卡 UI。

### 5.3 KamelCacheInfo —— 缓存状态

```c
const char* KamelCacheInfo(void);
```

响应：

```json
{"ok":true,"memoryEntries":3,"memoryMax":100,"diskCacheDir":"/path/to/app/cache"}
```

主线程直调，瞬时，demo 的「缓存信息」按钮用它刷新状态行。

### 5.4 内存释放规则

与 cmp_qrose 完全相同的模式：KN 返回的字符串是 Kotlin/Native 堆上的内存，**必须**经 `libohoskamel_symbols()->DisposeString` 释放，不能 `free`，也不能让 ArkTS 去管。`napi_init.cpp` 的 `JsToUtf8`（两段式取长再开 `std::vector<char>` 拷贝）与 `DisposeKnString` 照搬 qrose 的实现：先 `napi_create_string_utf8` 把结果拷进 JS 字符串，再立刻 `DisposeString` 原始指针。

---

## 6. 关键决策

| 决策 | 选择 | 理由 |
|---|---|---|
| 适配面 | load / cache 核心进 so，decode / display 交系统 image kit | Compose 1.10.0 与鸿蒙 CMP 移植线不齐；系统解码器现成 |
| 工具链 | HarmonyOS Kotlin 2.4.20-0.1.0-07 | ktor ohos 移植版只有 3.5.1-0.1.0-01 一代，其代际是 2.4.20 |
| LRU | 手写 LinkedHashMap 线程安全 LRU | cache4k 在鸿蒙镜像仓有元数据无 artifact，实际拉不到 |
| 桥 | `@CName` × 3 + CMake NAPI 包装 | CAdapter 不把业务函数挂到 ArkTS 模块 |
| JSON | 自写 MiniJson，顶层扁平 | 避免为几个字段拉 kotlinx.serialization 并对齐其版本 |
| 线程 | 同步 C ABI + ArkTS Worker | `runBlocking` 不能上主线程；demo 用 ThreadWorker |
| 过桥载荷 | base64 原始字节 | PNG/JPEG/WebP/GIF/SVG 全交 image kit，一套通路通吃 |
| ABI | arm64 + x64 都出 | 模拟器是 x86_64，缺 x64 so 装不上 |
| HTTP 引擎 | ktor-client-curl 3.5.1-0.1.0-01 | ohos 移植线可用的引擎 |

---

## 7. 踩坑表

九个坑全部真实发生，按出现顺序：

| 现象 | 原因 | 处理 |
|---|---|---|
| coroutines 1.11.0-0.1.0-07 报 `metadata version is 2.4.0, but the compiler version 2.2.0 can read up to 2.3.0` | 鸿蒙移植库分代际：ktor 3.5.1-0.1.0-01 及其依赖是 Kotlin 2.4.20 代；2.2.x 编译器读不了其 2.4.0 元数据 | Kotlin 钉 `2.4.20-0.1.0-07`；coroutines / okio 用 `-0.1.0-07` |
| cache4k 0.14.0 `Could not find` | eazytec 镜像有 maven-metadata 列表但选择性存 artifact | 去掉依赖，`LruCache` 手写 |
| `compileKotlinOhosArm64` 报 `ClassNotFoundException: org.jetbrains.kotlin.cli.utilities.MainKt` | 2.4.20-0.1.0-**08** 的 KN 预构建发行版不完整 | 换回 2.4.20-0.1.0-**07**（本机 `~/.konan` 已有其发行版） |
| `kotlin.concurrent.SynchronizedObject` Unresolved / 不可继承 | 该 ohos Kotlin 构建未暴露 `kotlin.concurrent` 版本；且它是工厂函数不是 open class | 与上游 `DiskLruCache` 一致，用 `kotlinx.coroutines.internal.SynchronizedObject` + `@OptIn(InternalCoroutinesApi::class)` |
| `LinkedHashMap(16, 0.75f, true)` 在 Native 报 candidates not applicable | `accessOrder` 三参构造是 JVM 独有 | `get` 时 `remove` + re-`put` 手动维护 LRU 访问顺序 |
| desktopMain 的 httpCache actual 报 Unresolved `org.jetbrains.skiko.hostOs` | skiko 是 Compose 传递依赖，隔离 composeMain 后不再存在 | 文件移到 `jvmMain`，改用 `System.getProperty("os.name")` 判定 |
| `httpCacheStorage` expect 无 JVM actual | 上游 desktop 目标名叫 `desktop`，本 fork 目标名叫 `jvm` | `git mv` desktopMain → jvmMain |
| `gradlew` 报 `JAVA_HOME not set` | `org.gradle.java.home` 只作用于 fork 出来的 JVM，wrapper 启动本身要 `JAVA_HOME` | 命令前 `export JAVA_HOME`（PowerShell 用 `$env:JAVA_HOME=...`） |
| 设备上 HTTPS 全部失败 `请求失败 CURL_PEER_FAILED_VERIFICATION - TLS peer certificate verification failed` | libcurl 在鸿蒙上读不到系统信任库（`/system/etc/security` 对应用不可见），TLS 验证无根证书可用 | ktor 3.5 curl 引擎原生支持 `engine { caInfo = ... }`（CURLOPT_CAINFO）：`Kamel.configure` 增加 `caBundlePath`，demo 打包 Mozilla CA bundle（`rawfile/cacert.pem`，121 根证书）启动时拷进沙箱传入。实测 HTTPS 恢复（见第 2 节设备层验收表） |

前两条是「代际对齐」的学费：鸿蒙移植库不是按上游版本号齐步走的，而是按 Kotlin 编译器代际成套出现。挑依赖先看它是哪一代编译的，再看版本号。第九条是 curl 类引擎上鸿蒙的通病：**网络栈能通，TLS 信任链要自带**——任何用 ktor curl 引擎的鸿蒙适配都要处理这一条。

---

## 8. 分层验收

1. **编译层：** `:kamel-core:compileKotlinJvm`、`:kamel-core:compileTestKotlinJvm`、`compileKotlinOhosArm64`、`compileKotlinOhosX64` 全部通过。Compose 文件不在编译路径上。
2. **链接层：** `linkDebugSharedOhosArm64/X64` 通过（约 1m28s），`copyKnSoArm64/X64` 把 `libohoskamel.so` 与匹配 `libc++_shared.so` 拷入 `entry/libs/{arm64-v8a,x86_64}`。
3. **符号层：** `llvm-nm -D` 双 ABI 均见 `T KamelInit / T KamelLoad / T KamelCacheInfo / T libohoskamel_symbols`；ArkTS 只 import `libkamel_napi.so`。
4. **功能层：** `KamelInit` 配置缓存目录、`KamelLoad` 返回 base64 与 source、`KamelCacheInfo` 报内存缓存条目——这三条在 Demo 状态行上各有输出位（见第 16 节）。
5. **设备层：** **待真机/模拟器测试**。hvigor 编译、HAP 打包、签名、安装、运行与功能验收都还没做，操作步骤与验收点清单见第 12 节。

前三层是「机器已证明」的，第四层的调用链在代码里就绪，第五层要人工在 DevEco 里走完。

---

## 9. 已知限制

1. **鸿蒙侧无 Compose UI。** `KamelImage` / `asyncPainterResource` / `Painter` 体系在 `composeMain` 不编译；鸿蒙业务侧显示用 ArkUI `Image`。接回条件：社区 CMP 版本与上游 1.10.0 API 对齐后。
2. **同步 C ABI，无逐字节进度流。** 上游的 `onLoading` 进度回调没有等价物；`elapsedMs` / `byteCount` 是事后给的。大图 base64 传输有内存峰值（base64 本身膨胀约 1/3）。
3. **内存 LRU 缓存的是原始字节，不是解码位图。** 解码缓存交给系统 image kit 的 `PixelMap` 生命周期管理。
4. **GIF 只保证首帧**（系统 image kit 行为），动画帧由系统处理；SVG 由系统 kit 解析，与上游 SVGDOM 渲染路径不同，复杂 SVG 的效果可能有差异。
5. **返回值无 `contentType`。** image kit 按内容嗅探解码，不需要这个字段。
6. **HTTPS 证书已用内置 CA bundle 解决**（curl 引擎读不到鸿蒙系统信任库）：`Kamel.configure(caBundlePath = ...)` 经 ktor curl 的 `caInfo`（CURLOPT_CAINFO）注入 Mozilla 根证书，demo 打包 `rawfile/cacert.pem`（121 根证书，188 KB）并在启动时拷进沙箱。注意：自签/企业内网证书需自行替换该 bundle；`http` 明文流量仍未配置，demo 预设全是 https。
7. **上游 example 各平台 App 未接入**鸿蒙工程。`composeMain` 里的上游装配（`KamelConfig.Core` 那套 fetcher/mapper）留待接回，桥直接用新的 `Kamel` 入口。
8. **jvmTest 有 2 个上游遗留失败**（`StringMapperTest.testAbsoluteFilePath` / `testRelativeFilePaths`）：这两个用例在上游就标注 "This is currently broken"（上游 PR #78 跟踪），ktor 从 3.3.3 换到鸿蒙移植线 3.5.1-0.1.0-01 后，`Url("/path/…")` 被规范化为 `http://localhost/path/…`，断言差异暴露得更明显。`StringMapper` 本身未被改动，与本适配无关；其余 15 个测试全部通过。

---

## 10. commonMain 里保留了什么（加载/缓存核心清单）

「物理隔离 Compose」不等于「把库掏空」。commonMain 里整套抓取与缓存体系原样保留，一行算法没动：

| 体系 | 文件 | 说明 |
|---|---|---|
| 缓存接口与 LRU | `cache/Cache.kt`、`cache/LruCache.kt` | 接口原样；LruCache 因 cache4k 缺位重写为实现类 |
| 磁盘缓存 | `cache/DiskCacheStorage.kt`、`cache/DiskLruCache.kt` | okio POSIX 落盘，ohos 上原样编译 |
| HTTP 磁盘缓存 actual | `cache/httpCache.kt`（expect）+ 各平台 actual | ohosMain 的 actual 用 `DiskCacheStorage(FileSystem.SYSTEM, cacheDir/http-cache, maxSize)` |
| 抓取器 | `fetcher/Fetcher.kt`、`fetcher/HttpFetcher.kt` 等 | HttpUrlFetcher 产出 `Resource<ByteReadChannel>`，流式语义保留 |
| 映射器 | `mapper/Mapper.kt`、`mapper/StringMapper.kt` 等 | URL/URI 映射链 |
| 资源模型 | `Resource.kt`、`DataSource.kt`、`config/ResourceConfig.kt` | ResourceConfig 已瘦身（删两个 Compose 字段，保留 `requestData` + `coroutineContext`） |

在此之上新增了**非 Compose 公共入口** `io.kamel.core.Kamel`（单例对象），这是鸿蒙侧唯一入口：

```kotlin
// 初始化：首次加载前调用一次
Kamel.configure(diskCacheDir = ctx.cacheDir, memoryCacheSize = 100, httpCacheSize = 10L * 1024 * 1024)

// 加载：内存 LRU 命中 → Resource.Success(bytes, DataSource.Memory)
//       否则走 HttpUrlFetcher + Ktor HttpCache 磁盘缓存
suspend fun load(): Resource<ByteArray> = Kamel.loadBytes("https://...") {
    // HttpRequestBuilder.() -> Unit，可加 header 等
}

// 状态与管理
Kamel.clearMemoryCache()
Kamel.memoryCacheEntries      // 当前条目数
Kamel.memoryCacheMaxSize      // 配置上限
Kamel.configuredDiskCacheDir  // 已配置的磁盘缓存目录（未配置为 null）
```

上游 `KamelConfig` 那套 fetcher/mapper 装配留在 composeMain（未来接回）；桥不碰它，直接用新入口。这对应 cmp_qrose 适配里「新增 `Qrose.encodeQr` 非 Compose 出口」的同一规则：**给纯核心开一个不依赖 UI 框架的门，桥只走这扇门。**

---

## 11. 接口对照（上游 Compose API → 鸿蒙侧等价物）

不是逐函数翻译，而是按「加载/缓存/解码/显示」四段责任对齐。这张表是 Demo 的验收清单，也是以后写使用篇的接口目录。

| 上游 Compose API | 鸿蒙侧等价物 |
|---|---|
| `asyncPainterResource(url)` 返回 `Resource<Painter>` | `kamelLoad(url)` 返回 `{base64 原始字节}` → ArkTS `image.createImageSource().createPixelMap()` → ArkUI `Image(PixelMap)` |
| `KamelImage` composable | ArkUI `Image` 组件 + Worker 线程加载 |
| `KamelConfig(imageBitmapCacheSize 等四缓存) | `Kamel.configure(diskCacheDir, memoryCacheSize, httpCacheSize)`（内存缓存只存原始字节，解码缓存交给系统 image kit 的 PixelMap 生命周期） |
| `onLoading` 进度回调 | 同步 C ABI 无流式回调；`elapsedMs`/`byteCount` 在结果里（已知限制） |
| 解码 `ImageBitmapDecoder`(Skia) / `SvgDecoder`(SVGDOM) / `AnimatedImageDecoder`(Codec) | 系统 image kit 统一解码（PNG/JPEG/WebP/GIF/SVG；GIF 显示首帧，动画帧由系统处理） |
| Ktor engine（Android=OkHttp / desktop=CIO / apple=Darwin） | `ktor-client-curl` 3.5.1-0.1.0-01（ohos 移植线） |
| 磁盘缓存 Ktor `HttpCache` + `DiskCacheStorage` | 同一套代码原样编进 ohos（okio POSIX），目录 `cacheDir/http-cache` |

读法举例：上游 `asyncPainterResource` 的三态（Loading/Success/Failure）在鸿蒙侧变成——Loading 由 Worker 派发后的 UI 状态承担，Success 是 JSON 里 `ok:true` 加 base64，Failure 是 `ok:false` 加 `error`。语义没有丢，只是载体从 `Resource<Painter>` 换成了 `Resource<ByteArray>` 过桥。

---

## 12. HAP 怎么打、真机/模拟器怎么装（设备层验收步骤）

这一节是**待执行**的操作手册。Kotlin 侧只负责 so，HAP 仍要 DevEco / hvigor：

1. **先链 so。** 跑 `:example:nativeApp:linkDebugSharedOhosArm64` 和 `:example:nativeApp:linkDebugSharedOhosX64`。`copyKnSo*` 是 `finalizedBy`，会把 `libohoskamel.so` 和对应 ABI 的 `libc++_shared.so` 拷进 `example/harmonyApp/entry/libs/<abi>/`。不要手工从别的工程借 so。
2. **用 DevEco Studio 打开 `example/harmonyApp` 子目录**，不要打开仓库根目录。根目录是 Gradle KMP 工程，DevEco 认的是 hvigor 工程。
3. **自动签名。** File > Project Structure > Signing Configs，勾选 Automatically generate signature（需登录华为账号）。`build-profile.json5` 已写 `"signingConfig": "default"` 且 `signingConfigs` 为空数组，证书落在本机，不把任何 p12 路径提交进仓库。
4. **打包。** Build > Build Hap(s)/APP(s) > Build Hap(s)；或命令行（在 `example/harmonyApp` 下）：

   ```text
   hvigorw assembleHap -p product=default -p buildMode=debug --no-daemon
   ```

5. **安装与启动：**

   ```text
   hdc install entry/build/default/outputs/default/entry-default-signed.hap
   hdc shell aa start -a EntryAbility -b org.terminator.ohos.kamel
   ```

6. **ABI 注意。** DevEco 模拟器是 x86_64 镜像，HAP 的 `libs/x86_64/` 必须三件套齐全（`libkamel_napi.so`、`libohoskamel.so`、`libc++_shared.so`），缺任何一个安装解析或运行期就失败；真机只看 `arm64-v8a`。两套 ABI 都打进同一份 HAP 最省事。`abiFilters` 已配置 `arm64-v8a` + `x86_64`。

**验收点清单**（每条对应页面状态行的一次输出）：

| 验收点 | 期望状态行 | 说明 |
|---|---|---|
| 启动初始化 | `初始化成功 cacheDir=...` | `KamelInit` 已在 `aboutToAppear` 调用 |
| 预设 URL 首次加载 | `加载成功 source=network byteCount=… elapsedMs=…` | 五个预设逐个点 |
| 同一 URL 二次加载 | `加载成功 source=memory …` | elapsedMs 应明显变小 |
| 缓存信息 | `memoryEntries=… memoryMax=100 diskCacheDir=…` | 「缓存信息」按钮 |
| SVG 预设 | 正常出图 | `placehold.co` 返回 SVG，系统 image kit 解析 |
| 杀进程重启再加载 | 仍能出图（source 可能仍报 network） | Ktor HttpCache 是透明磁盘缓存，命中时不改变 DataSource 语义 |
| 断网后加载已缓存 URL | 验证透明缓存能否直接命中；若不能出图如实记录 | 作为后续「磁盘缓存直读入口」改进依据 |

---

## 13. 以后怎么给上游提 issue / PR

库本身的问题（Ktor 配置、缓存逐出语义、fetcher 行为）应该提到上游 Kamel 仓库，而不是提到鸿蒙 fork。鸿蒙 fork 只收这些：

- `ohosArm64` / `ohosX64` 目标与 sharedLib 桥
- `composeMain` 隔离与 `ResourceConfig` 瘦身
- 新增 `Kamel` 非 Compose 入口、`LruCache` 重写
- `ohosMain` actual 与 `jvmMain` 迁移
- nativeApp 桥与 HAP Demo

提交 PR 时作者名用 **Everlasting**，组织走 **oh-tpc**。README 与提交说明里只放 AtomGit 链接（[oh-tpc/Kamel](https://atomgit.com/oh-tpc/Kamel)），不要写第三方镜像站品牌名。

如果哪天社区 CMP 版本与上游 1.10.0 API 对齐，优先做的事情是：把 `composeMain` 改回 `commonMain`，让 `KamelImage` / `asyncPainterResource` 在鸿蒙 Compose 上直接跑，新 `Kamel` 入口降级为非 Compose 场景的备用出口。当前这条「字节过桥 + 系统 image kit」路径是为了先把 CMP 库的核心资产交到鸿蒙上，不是终态架构。

---

## 14. 源码集手术明细（composeMain 隔离与新增入口）

这一刀是整个适配里最细的活，全部用 `git mv` 保留历史。

### 14.1 挪出编译路径

主代码（都 `import androidx.compose.*`）`git mv` 到 `kamel-core/src/composeMain/kotlin/...`：

```text
ImageLoading.kt
AnimatedImage.kt
config/KamelConfig.kt
config/KamelConfigBuilder.kt
config/ResourceConfigBuilder.kt
utils/ConfigUtils.kt
```

测试侧（用了 compose resources）`git mv` 到 `kamel-core/src/composeMain/test/...`：

```text
config/KamelConfigBuilderTest.kt
config/KamelConfigUtilsTest.kt
config/ResourceConfigBuilderTest.kt
fetcher/HttpFetcherTest.kt
tests/HttpMockEngine.kt
```

`composeMain` 不注册为任何 target 的源集，sync 期就被排除。**不要删除**——它们是对照上游 API 的活文档，也是接回时的原料。

### 14.2 类型抽取：ResourceConfig 瘦身

`config/ResourceConfig.kt` 留在 commonMain，但删掉两个 Compose 字段：`density: Density` 与 `maxBitmapDecodeSize: IntSize`，保留 `requestData` + `coroutineContext`。这与 cmp_qrose 把 `Code128Type` 从 Painter 文件抽回 commonMain 是同一条规则：**接口定义跟 UI 文件绑在一起、而核心又要用的时候，把接口洗成非 Compose 形态留在 commonMain。** 删字段而不是挪整个文件，因为 `HttpUrlFetcher` 等核心需要 `ResourceConfig` 携带请求构建器。

### 14.3 新增非 Compose 入口 Kamel.kt

`kamel-core/src/commonMain/kotlin/io/kamel/core/Kamel.kt`，内容见第 10 节。要点：`configure` 幂等（目录变了关旧 HttpClient、容量变了清旧 LruCache）；`loadBytes` 先查内存 LRU，未命中走 `HttpUrlFetcher` 把 `ByteReadChannel` 收成 `ByteArray` 回填缓存。

### 14.4 LruCache 重写

上游把 LRU 委托给 cache4k——鸿蒙镜像仓里它有 maven-metadata 但没有 artifact，实际拉不到。重写为 `LinkedHashMap` 手写线程安全 LRU：`get` 命中时 `remove` + re-`put` 维护访问顺序（因为 Native common 没有 `accessOrder` 三参构造），超限从头逐出。锁用 `kotlinx.coroutines.internal.SynchronizedObject`（见第 7 节第 4 条坑）。

### 14.5 ohosMain 新增 5 个 actual

```text
utils/Platform.kt        Dispatchers.Kamel = IO；File/URL/URI 最小实现（File 用 okio POSIX 读文件）
fetcher/FileFetcher.kt   本地文件抓取
fetcher/FileUrlFetcher.kt file:// 转本地文件读
mapper/Mappers.kt        URLMapper / URIMapper
cache/httpCache.kt       DiskCacheStorage(FileSystem.SYSTEM, cacheDir/http-cache, maxSize)
```

ohos 上没有 `java.io.File` 也没有 `NSFileHandle`，`expect class File` 的 actual 就是一层 okio `FileSystem.SYSTEM` 的薄封装——这也是整个适配里少数需要真正新写平台代码的地方。

### 14.6 desktopMain → jvmMain

上游 desktop 目标的 `httpCache.desktop.kt` 依赖 `org.jetbrains.skiko.hostOs` 判定目录；skiko 是 Compose 的传递依赖，隔离后不存在了。处理：文件移到 `jvmMain`，改用 `System.getProperty("os.name")`。顺带解决 expect 无 JVM actual 的问题（上游目标名 `desktop`，本 fork 目标名 `jvm`）。

---

## 15. MiniJson 为什么不上 kotlinx.serialization

桥的入参是两份扁平 JSON：`KamelInit` 的 `{cacheDir, memoryCacheSize, diskCacheBytes}`，`KamelLoad` 的 `{url, headers, timeoutMs}`。没有嵌套对象，没有数组（`headers` 故意做成 `;` 连接的扁平字符串）。为这点东西拉 `kotlinx-serialization` 会多出一个 KN 依赖，还要和 HarmonyOS Kotlin 插件的序列化版本做代际对齐——第 7 节的学费不想再交一次。

`Bridge.kt` 里的 `MiniJson` 只做三件事：按引号状态机切顶层逗号、剥引号与转义、把值按需转 Int/Long。字符串里的逗号靠 inStr 状态跳过，URL 带逗号也不会切坏。

不要把 MiniJson 提成公共 API。业务侧继续传 JSON 字符串，以后若字段变复杂，只换解析器，不换 `KamelLoad` 的 C 签名。C ABI 保持 `const char* -> const char*`，ArkTS 侧 `kamelLoad(string): string` 永远不用改。

布尔/数值字段按 JSON 标准来。ArkTS `JSON.stringify({...})` 产出的正是 MiniJson 认的形态。

---

## 16. Demo 页面怎么验收

`example/harmonyApp/entry/src/main/ets/pages/Index.ets` 是单页 Demo：URL 输入框、五个预设按钮、加载/缓存信息两个按钮、图片区、状态行。

预设列表（真实代码）：

| 按钮 | URL | 用意 |
|---|---|---|
| Photo 1 | `https://picsum.photos/300/200` | 首次 network → 二次 memory |
| Photo 2 | `https://picsum.photos/seed/kamel/300/200` | 种子图，字节稳定，适合 JVM 对账 |
| Photo 3 | `https://picsum.photos/seed/ohos/300/200` | 同上 |
| Photo 4 | `https://picsum.photos/400/260` | 非整数比例 |
| SVG | `https://placehold.co/300x200` | SVG 解码路径 |

状态行是唯一验收输出位，格式固定：

```text
初始化成功 cacheDir=...
加载成功 source=memory byteCount=24832 elapsedMs=3
加载失败: timed out after 15000ms elapsedMs=15012
memoryEntries=2 memoryMax=100 diskCacheDir=/data/storage/el2/base/cache
```

页面自带一条提示文案：「再次点击同一预设图验证内存缓存命中（source=memory）；杀进程重启后可能显示 network（Ktor HttpCache 透明磁盘缓存）」——这句是把第 12 节验收表里两条最容易误判成 bug 的行为提前讲清楚：透明磁盘缓存命中时 `source` 语义不变，不要因为重启后看到 `network` 就以为磁盘缓存没生效，要看有没有真的发起网络传输（流量/耗时）。

所有加载走 Worker：主线程 `postMessage` 请求 JSON，Worker 里直调 `kamelLoad`，回传响应 JSON，主线程再解码刷 UI。图片解码用 image kit，旧的 `PixelMap` 在替换前 `release()`。

---

## 17. 故障排查

前两行是构建期已实际遇到的（处理见第 7 节），其余是设备层预判项——按社区同类工程（cmp_qrose / Vico / Reorderable）的共性经验列出，真机阶段按表排查：

| 现象 | 真正原因 | 处理 |
|---|---|---|
| `JAVA_HOME not set` | wrapper 启动需要 JAVA_HOME | 命令前 `export JAVA_HOME`（PowerShell `$env:JAVA_HOME=...`） |
| metadata version / Could not find 报错 | 依赖代际或镜像缺 artifact | 按第 7 节表逐条对版本 |
| 安装失败码 `9568347` | HAP 缺当前 ABI 的 so | 模拟器必须有 `libs/x86_64/` 三件套（napi/KN/libc++） |
| 能安装，一点「加载」就崩 | `kamelLoad` 未注册或 so 没 dlopen 成功 | 核对 CMake `nm_modname=kamel_napi`、ArkTS `import 'libkamel_napi.so'`、`index.d.ts` 导出名三者一致 |
| `does not provide an export named 'kamelLoad'` | HAP 缺当前 ABI 的 `libkamel_napi.so` | `abiFilters` 必须含 `arm64-v8a` 和 `x86_64` |
| 页面卡死 / 主线程无响应 | `KamelLoad` 在主线程直调（同步阻塞） | 放 Worker 线程（demo 的 `KamelWorker.ets` 已做） |
| 状态行 `初始化失败: cacheDir is required` | `KamelInit` 没拿到应用 cacheDir | 检查 `getUIContext().getHostContext()` 时机，太早拿不到 host context |
| `加载失败: timed out` | 网络不通或超时过短 | 默认 15s，可在请求 JSON 调 `timeoutMs`（上限 120s） |
| HTTPS 证书相关报错 | curl 证书链 / 系统证书库 / 系统时间 | 优先检查系统证书库与时间；必要时评估给 curl 配自定义 CA 路径 |
| 网络请求全部失败 | 缺 INTERNET 权限 | `module.json5` 的 `ohos.permission.INTERNET` 是硬前提（cmp_qrose 不需要，Kamel 必需） |
| so 的 NEEDED 里出现 Windows 盘符路径 | CMake 把 IMPORTED 绝对路径写进 RUNPATH | `IMPORTED_NO_SONAME` + `$ORIGIN` rpath（本工程 CMake 已做） |
| 内存持续上涨 | 旧 PixelMap 未 release 或 KN 字符串未 Dispose | `replacePixelMap` 先 release；NAPI 层 `DisposeString`（已做，自查别处） |

查 so 是否导出符号：

```text
llvm-nm -D example/harmonyApp/entry/libs/arm64-v8a/libohoskamel.so | findstr Kamel
llvm-nm -D example/harmonyApp/entry/libs/x86_64/libohoskamel.so | findstr Kamel
```

四个 T 符号（`KamelInit` / `KamelLoad` / `KamelCacheInfo` / `libohoskamel_symbols`）缺一不可。`libkamel_napi.so` 的 NEEDED 里只应看到 `libohoskamel.so`、`libace_napi.z.so`、`libc++_shared.so`、`libhilog_ndk.z.so` 这类短名。

运行时日志：

```text
hdc shell hilog | findstr Kamel
```

NAPI 层的 `OH_LOG_INFO` 打在 `KamelNapi` tag 下；业务异常不走异常抛出跨语言，统一在 JSON `error` 字段里回给 ArkTS，页面状态行显示 `加载失败: …`。不要把 Kotlin 异常直接抛过 NAPI——KN 异常跨语言会变成无信息崩溃。

---

## 18. 相对上游改了哪些文件

| 路径 | 动作 | 原因 |
|---|---|---|
| `settings.gradle.kts` | 两个仓库块顶部加 eazytec `maven-public` 与 `content/groups/harmonyos`；include 只留 `:kamel-core`、`:example:nativeApp` | 其余模块移出构建 |
| `gradle/libs.versions.toml` | Kotlin `2.4.20-0.1.0-07`、coroutines/okio `-0.1.0-07`、ktor `3.5.1-0.1.0-01` | 代际对齐 |
| `gradle.properties` | JDK 21 钉死；新增 `kotlin.native.ignoreDisabledTargets=true`、`kotlin.native.distribution.downloadFromMaven=true` | 鸿蒙目标必需 |
| `gradle/wrapper` | 8.14.1 | 与 HarmonyOS Kotlin 插件兼容 |
| `kamel-core/build.gradle.kts` | `jvm()` + `ohosArm64()` + `ohosX64()`；依赖换成鸿蒙代际 | 库模块不开 sharedLib |
| 6 个 Compose 文件 | `git mv` commonMain → composeMain | `import androidx.compose.*` |
| 5 个测试文件 | `git mv` commonTest → composeMain/test | 用 compose resources |
| `config/ResourceConfig.kt` | 删 `density` 与 `maxBitmapDecodeSize` 两字段 | 类型抽取，核心要留在 commonMain |
| `cache/LruCache.kt` | 重写为手写 LRU | cache4k 拉不到 |
| `io/kamel/core/Kamel.kt` | 新增 | 非 Compose 公共入口 |
| `ohosMain` 5 个 actual | 新增 | Platform / FileFetcher / FileUrlFetcher / Mappers / httpCache |
| `desktopMain/httpCache.desktop.kt` | `git mv` 到 `jvmMain`，去 skiko | skiko 随 Compose 隔离而消失 |
| `example/nativeApp` | 新增 | sharedLib `ohoskamel` + `@CName` × 3 + MiniJson + copyKnSo 任务 |
| `example/harmonyApp` | 新增 | DevEco Stage 工程 + CMake NAPI + Worker + Demo 页 |
| `README.md` / `README.upstream.md` / `README.OpenHarmony.md` / `README.OpenHarmony_CN.md` | 新增/重写 | README 四件套；中文版是主文档 |

没有改抓取算法，没有改磁盘缓存协议，没有改 Mapper 语义。缓存与网络行为必须和 JVM 上的 Kamel 1.0.9 一致，这是 CMP 适配的验收底线。`composeMain` 里的 Compose 文件仍然在仓库里，不要删除，也不要在文档里写成「已经在鸿蒙上跑 Compose UI」——那不是事实。

---

## 19. 和 cmp_qrose / Vico / Reorderable 是同一条路

社区里已经落地的 CMP 库，没有一家是「把 Compose 渲染直接跑在 ArkUI 上」。共同结构：

1. `commonMain` 保住算法 / 数据模型 / 核心管线。
2. Compose UI（Painter、Modifier、composable）隔离到 `composeMain` 或直接拿掉。
3. `ohosArm64` / `ohosX64` 出 `sharedLib`。
4. `@CName` 提供 C ABI。
5. CMake 编一个 NAPI so，ArkTS 只 import 这个 so。
6. HAP Demo 用 ArkUI 把结果呈现出来。

差异在「过桥的是哪一层」：Vico 把图表数据交给 ArkTS 画 Canvas；Reorderable 把列表状态交给 ArkTS 列表；cmp_qrose 在 KN 里做光栅化出 PNG，因为条码样式模型重；**Kamel 过桥的是原始字节，解码责任整个移交系统 image kit**——因为鸿蒙设备上本来就有一个高质量解码器，在 18MB 的 so 里再养一份 Skia 没有意义。四种选择都是「上游值钱的部分在哪，就保哪」的同一逻辑。

同样地，新 `Kamel` 入口只活在 `kamel-core`（它本身就是库的公共 API 候选），桥细节只活在 `example/nativeApp`，不反向渗透进库模块。等 `compose.ui` 的 ohos 制品出现，`composeMain` 接回 `commonMain`，这条字节通路降级为非 Compose 场景的 fallback，拆除成本为零。

---

## 20. HAP 命令行打包注意

Kotlin 侧：

```text
gradlew :example:nativeApp:linkDebugSharedOhosArm64 :example:nativeApp:linkDebugSharedOhosX64
```

`copyKnSoArm64/X64` 是 `finalizedBy`，从 `build/bin/<target>/debugShared` 拷 `libohoskamel.so`，并从 `~/.konan/dependencies` 里找同 triple 的 `libc++_shared.so`（aarch64-linux-ohos / x86_64-linux-ohos）一起拷。so 没链出来之前别开 DevEco 打包，CMake 会直接 `FATAL_ERROR: Missing .../libohoskamel.so`。

HAP 侧必须用 DevEco 的 Node、ohpm、JBR：

```text
cd example/harmonyApp
hvigorw assembleHap -p product=default -p buildMode=debug --no-daemon
```

产物在 `entry/build/default/outputs/default/`。真机走自动签名后的 signed HAP；模拟器一般接受 debug 包。签名配置保持 `signingConfigs` 空数组 + `"signingConfig": "default"`，让 DevEco 本机生成，不要把 p12 路径写进仓库，也不要把这段改空。

安装与启动：

```text
hdc install <hap 路径>
hdc shell aa start -a EntryAbility -b org.terminator.ohos.kamel
```

包名以 `AppScope/app.json5` 为准（`org.terminator.ohos.kamel`，versionName `1.0.9-ohos.1`）。模拟器是 x86_64，必须带 `x86_64/libs`；截图用 `hdc shell snapshot_display -f /data/local/tmp/kamel.jpeg` 再 `hdc file recv` 拉回。

ArkTS 侧不要对 `libohoskamel.so` 写 `oh-package` 依赖。类型包只给 `libkamel_napi.so`。KN so 是 NAPI 的链接依赖，不是 ArkTS 的 import 依赖——这一点和 cmp_qrose、Vico、Reorderable 完全相同。

---

## 21. ArkTS 侧集成示例

Demo 的完整链路抽成可复制片段。三步：Worker 里调桥、主线程收结果、image kit 解码成 PixelMap。

页面侧（`Index.ets` 关键片段）：

```ts
import { kamelInit, kamelCacheInfo } from 'libkamel_napi.so';
import { image } from '@kit.ImageKit';
import { util, worker, ErrorEvent, MessageEvents } from '@kit.ArkTS';

@Entry
@Component
struct Index {
  @State pixelMap: image.PixelMap | undefined = undefined;
  @State status: string = '尚未加载';
  private workerInstance: worker.ThreadWorker | undefined = undefined;

  aboutToAppear(): void {
    // 1) 首次加载前初始化一次，cacheDir 用应用 cacheDir
    const ctx = this.getUIContext().getHostContext();
    const config: string = JSON.stringify({
      cacheDir: ctx !== undefined ? ctx.cacheDir : '',
      memoryCacheSize: 100,
      diskCacheBytes: 10485760
    });
    const raw: string = kamelInit(config);          // 瞬时，可主线程直调
    this.status = JSON.parse(raw).ok ? '初始化成功' : '初始化失败';

    // 2) Worker：KamelLoad 是同步阻塞调用，绝不能上主线程
    const w: worker.ThreadWorker =
      new worker.ThreadWorker('entry/ets/workers/KamelWorker.ets');
    w.onmessage = (e: MessageEvents): void => {
      this.onLoadResult(e.data as string);
    };
    this.workerInstance = w;
  }

  private load(url: string): void {
    const request: string = JSON.stringify({
      url: url, headers: '', timeoutMs: 15000
    });
    this.workerInstance?.postMessage(request);
  }

  // 3) 收结果 → base64 解码 → image kit 出 PixelMap
  private onLoadResult(raw: string): void {
    const parsed = JSON.parse(raw);
    if (!parsed.ok || parsed.base64 === '') {
      this.status = `加载失败: ${parsed.error ?? 'empty'}`;
      return;
    }
    const bytes: Uint8Array = new util.Base64Helper().decodeSync(parsed.base64);
    // 关键：Uint8Array 视图要 slice 出整段底层 buffer，否则解码报错
    const sliced = bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength);
    const src = image.createImageSource(sliced);
    src.createPixelMap().then((pm: image.PixelMap) => {
      if (this.pixelMap !== undefined) {
        this.pixelMap.release();          // 旧图先释放，防内存上涨
      }
      this.pixelMap = pm;
      this.status = `加载成功 source=${parsed.source} byteCount=${parsed.byteCount} elapsedMs=${parsed.elapsedMs}`;
    });
  }

  build() {
    Column() {
      if (this.pixelMap) {
        Image(this.pixelMap).objectFit(ImageFit.Contain)
      }
      Text(this.status)
    }
  }
}
```

Worker 侧（`entry/ets/workers/KamelWorker.ets` 全文）：

```ts
import { kamelLoad } from 'libkamel_napi.so';
import { worker, MessageEvents } from '@kit.ArkTS';

// Worker 脚本内的正规入口是 worker.workerPort（ThreadWorkerGlobalScope）。
// 注意：本 SDK（API 18）的 @kit.ArkTS 不具名导出 ThreadWorker 类，
// 且 ThreadWorker 构造器必传 scriptURL（主线程专用），Worker 侧不要 new。
const port = worker.workerPort;

port.onmessage = (e: MessageEvents): void => {
  const request: string = e.data as string;
  const raw: string = kamelLoad(request);   // 同步阻塞，跑在 Worker 线程
  port.postMessage(raw);
};
```

四个注意点：

1. **`KamelLoad` 是同步 C 调用**，网络 IO 数秒级，Worker 线程是硬要求；`KamelInit` / `KamelCacheInfo` 瞬时，主线程可直调。
2. **`buffer.slice` 那一步不能省。** `decodeSync` 返回的 `Uint8Array` 可能是带 byteOffset 的视图，直接把视图丢给 `createImageSource` 会解码失败。
3. **`PixelMap` 要换新前先 `release` 旧的**，滚动列表里不释放会涨内存。
4. **错误走 `error` 字段**，桥不抛异常，把失败显示在状态行上比静默好排查。

想嵌入自己的工程，拷 `Index.ets` 的 `onLoadResult` + Worker 文件即可，不依赖 Demo 的其它状态。

---

## 22. JVM 交叉验证：字节一致性对账

`kamel-core` 保留 `jvm()` 目标，就是为了证明「鸿蒙侧跑的核心没有漂」。对账思路分三层：

1. **编译层（已通过）。** `compileKotlinJvm` / `compileTestKotlinJvm` 过了，说明 `Kamel` 入口、缓存、抓取这套代码在 JVM 语义下成立。
2. **字节层（真机阶段做）。** 同一 URL 在 JVM 与 ohos 两侧各跑一次 `Kamel.loadBytes`，比对 `byteCount` 与字节摘要（如 SHA-256）。JVM 侧运行时对账需要给 jvmTest 补一个 JVM 引擎依赖（ktor 在 JVM 上不默认带引擎），这属于待办，不在已跑通清单里。
3. **行为层（真机阶段做）。** 预设里的种子图（`picsum.photos/seed/kamel/...`）对同一 seed 返回稳定字节，适合做两端对账样本；比对结果应「同 URL 同字节」，有差异先查缓存目录是否串了，再怀疑桥。

三层都过了，才能说「加载/缓存核心 100% 复用」这句话成立。只在模拟器上看到一张能显示的图不算验收——能显示只证明管线通，不证明和上游行为一致。

---

## 23. 线程模型与内存生命周期

整条链路上有三个线程域、两种堆，边界要划清楚：

```text
ArkTS 主线程                 ArkTS Worker 线程            Kotlin/Native 域
────────────                ──────────────               ─────────────
aboutToAppear:
  kamelInit(config)   ──►   (无需 Worker，瞬时)
  postMessage(request) ──►  kamelLoad(request)  ──►      runBlocking {
                              同步阻塞等结果               withTimeoutOrNull(15s) {
onmessage(response)  ◄──   postMessage(response) ◄──       Kamel.loadBytes(url)
解码/刷 UI                                              }
                                                      }
```

- **`runBlocking` 只存在于桥内、只跑在 Worker 线程。** 库本体 `Kamel.loadBytes` 是 suspend 函数，语义干净；阻塞式 C ABI 是桥的适配选择，不是库的选择。
- **KN 字符串的生命周期归 NAPI 层管。** `KamelLoad` 返回的 `const char*` 指向 Kotlin/Native 堆，`napi_init.cpp` 拷进 JS 字符串后立即 `DisposeString`。`JsToUtf8` 用两段式（先量长度、再开 `std::vector<char>` 拷贝）规避 API 26 头文件里缓冲区非 const 的坑。
- **`PixelMap` 的生命周期归 ArkTS 页面管。** demo 在 `replacePixelMap` 里先 `release` 旧图、`aboutToDisappear` 里 `terminate` Worker 并 `release` 当前图。
- **大图注意 base64 峰值。** 原始字节 → base64 字符串 → JS 字符串 → `Uint8Array` → `PixelMap`，同一份图片数据在链路上会同时存在两到三份形态。超大图场景这是已知限制（第 9 节），列表场景建议控制预设尺寸或后续评估共享内存方案。

---

## 24. 常见疑问补充

**Q：为什么过桥的是 base64 字节，不是解码后的像素或文件路径？**
C ABI 上最稳的载荷是字符串。像素要约定宽高/格式/行距一套结构体协议；文件路径会引入目录权限与清理问题。base64 换来的是「image kit 一行解码」和「JVM 对照可比」——两端拿到的是同一份字节。代价是约 1/3 膨胀与内存峰值，已列进已知限制。

**Q：`headers` 为什么是 `"a:1;b:2"` 扁平字符串？**
MiniJson 只做顶层扁平解析，不支持下嵌套对象（与 cmp_qrose 相同策略）。业务侧要传多个头就拼 `;`。真有复杂结构需求时换解析器，C 签名不动。

**Q：为什么用 ThreadWorker 而不是 taskpool？**
两者都满足「离开主线程」。demo 主线程侧用 `worker.ThreadWorker` 是因为生命周期可控（页面 `aboutToDisappear` 里 `terminate`）；Worker 脚本侧则用 `worker.workerPort` 收发消息（API 18 的 SDK 不具名导出 `ThreadWorker` 类，且其构造器必传 scriptURL，Worker 侧不能无参 new）。桥对宿主线程模型无要求，taskpool 也能用。

**Q：内存缓存为什么存字节不存解码位图？**
上游四缓存里有解码位图缓存，那是因为 Compose 侧反复要 `ImageBitmap`。鸿蒙侧解码产物是系统的 `PixelMap`，其生命周期由 ArkTS 持有，库再缓存一份等于双倍内存。字节级 LRU + 系统 kit 按需解码，职责更干净。

**Q：`file://` 能加载吗？**
能。ohosMain 有 `FileUrlFetcher` 与 `File` 的 okio POSIX 实现，`Kamel.loadBytes` 的映射链支持 `file://` 转本地文件读。demo 预设全是 https，本地文件路径留给自己试。

**Q：上游的 decoder 模块（kamel-decoder/*）去哪了？**
没有删，但移出了构建。解码责任已移交系统 image kit，这些模块在鸿蒙侧没有编译价值；等 composeMain 接回时一并评估。

**Q：curl 引擎可靠吗？**
编译与链接已验证；HTTPS 证书链行为必须真机确认。若证书报错，处理思路在第 17 节：先查系统证书库与时间，再评估自定义 CA 路径，不要先怀疑桥。

---

## 25. 下一步

1. **真机验收（最优先）。** 按第 12 节步骤走完：DevEco 打开 `example/harmonyApp` → 自动签名 → assembleHap → 安装运行 → 逐条过验收点清单。重点记录：每个预设 URL 首次 `source=network`、二次 `source=memory`、「缓存信息」输出、杀进程重启后的透明磁盘缓存行为、断网后磁盘缓存命中验证、SVG 预设解码效果、curl HTTPS 证书链表现。模拟器（x86_64）与真机（arm64）各跑一遍。
2. **JVM 字节对账。** 给 jvmTest 补 JVM 引擎，用种子图做同 URL 字节一致性比对（第 22 节）。
3. **断网与缓存语义确认。** 若透明 HttpCache 在断网时不能直接出图，评估给桥加「磁盘缓存直读入口」（只读 `cacheDir/http-cache`），作为 `KamelLoad` 的旁路。
4. **接回评估。** 社区 CMP 版本与上游 1.10.0 API 对齐后，把 `composeMain` 接回 `commonMain`，恢复 `KamelImage` / `asyncPainterResource` 在鸿蒙 Compose 上的原生路径。
5. **提交与归档。** 仓库 [oh-tpc/Kamel](https://atomgit.com/oh-tpc/Kamel)，作者名 Everlasting，组织 oh-tpc。文档和提交说明里不要写第三方镜像站链接，仓库地址只保留 AtomGit。

环境搭建请直接看社区入口：[KMP&CMP 鸿蒙社区](https://atomgit.com/CPF-KMP-CMP)、[HarmonyOS 应用开发导读](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/application-dev-guide)。本仓库不展开 SDK、DevEco、模拟器安装步骤，避免和官方文档打架。真机和模拟器的操作入口以官方文档为准。

# QjackCtl 的 OpenHarmony 适配记录

这份文档记录我们把 **QjackCtl**（JACK Audio Connection Kit 的 Qt 图形控制台，能配置音频路由、查看端口连接、管理 patchbay 预设——长期是 Linux 桌面音频领域的常用工具）搬到 OpenHarmony / HarmonyOS PC 上的过程：鸿蒙工程怎么组织、环境怎么搭、HAP 怎么编出来、业务库怎么交叉编译，以及最后哪些功能能用、哪些受平台限制用不了。原项目是一个标准的 Qt 桌面应用，上游主页：<https://qjackctl.sourceforge.io/>（GPL-2.0+）。

这次走的是 Qt-for-OpenHarmony 这条路：用一个 ArkTS 壳工程起 `UIAbility`，铺 `XComponent` 承载 Qt 窗口，通过 `libqohos.so`（Qt-OHOS QPA 平台插件）加载外部交叉编译好的 `libqjackctl.so`。QjackCtl 的 C++/Qt 源码编译成独立共享库（17 MB），加上 12 个 `.so` 依赖，由 QPA 插件 `dlopen` 后调用其 `main` 入口。

> 需要先说清楚一个关键问题：**QjackCtl 是一个音频路由控制工具，但鸿蒙 PC 上没有 JACK 音频后端。** QjackCtl 本身只是控制台 UI——真正的音频路由、端口连接、传输控制都需要和 `jackd`（JACK 音频服务器）通信。鸿蒙 PC 上没有 `jackd`，所以 Connections 面板是空白的，音频路由配置、端口连接/断开、传输控制全用不了。但主窗口、菜单、工具栏、设置面板、Patchbay 预设编辑器这些纯 Qt UI 都正常。下面会详细讲原因。

---

## 一、工程结构 / 鸿蒙工程在哪

鸿蒙工程就是仓库根目录，主模块为 `entry/`。用 DevEco Studio 打开仓库根目录即可。

```text
ohos_QjackCtl-main/
├── AppScope/                              # 应用级配置（包名、图标、版本）
│   ├── app.json5                          # bundleName = com.example.qjackctlpc
│   └── resources/base/
│       ├── element/string.json            # 应用名 "QjackCtl"
│       └── media/                         # app_icon / startIcon
│
├── entry/                                 # 主模块（HAP 业务壳）
│   ├── libs/arm64-v8a/                    # 预编译 native 库 ⭐
│   │   ├── libqjackctl.so                 # QjackCtl 业务库（17 MB，导出 main 符号）
│   │   ├── libqohos.so                    # Qt-OHOS QPA 桥接库
│   │   ├── libQt5*.so                     # Qt 运行时
│   │   ├── libc++_shared.so               # OHOS NDK C++ 运行时
│   │   ├── platforms/libqohos.so          # QPA 平台插件
│   │   ├── styles/libqohosstyle.so        # QStyle 插件
│   │   └── imageformats/*.so              # 图片格式插件
│   ├── src/main/
│   │   ├── module.json5                   # Ability / 权限 / 设备类型 / 文件关联
│   │   ├── cpp/                           # Native 构建入口（示例 libentry.so）
│   │   ├── ets/
│   │   │   ├── qabilitystage/
│   │   │   │   └── QAbilityStage.ets      # 应用入口（初始化 Qt、加载 libqjackctl.so）
│   │   │   ├── qability/
│   │   │   │   ├── QAbility.ets           # 主 UIAbility（生命周期委托 libqohos）
│   │   │   │   ├── OhosExportModules.ts   # 导出鸿蒙系统 API 给 Qt
│   │   │   │   └── QEmbeddedUiExtensionAbility.ets
│   │   │   ├── pages/
│   │   │   │   ├── MainWindowNativeNode.ets   # 主窗口 XComponent
│   │   │   │   ├── SubWindowNativeNode.ets    # 子窗口
│   │   │   │   ├── FloatWindowNativeNode.ets  # 浮动窗口
│   │   │   │   └── UiExtensionNativeNode.ets  # UI 扩展窗口
│   │   │   └── common/
│   │   │       └── QtAppConstants.ets         # APP_LIBRARY_NAME = libqjackctl.so
│   │   └── resources/
│   │       └── base/profile/main_pages.json   # 页面路由
│   ├── build-profile.json5                # CMake + arm64-v8a ABI
│   └── oh-package.json5
│
├── build-profile.json5                    # 顶层构建配置
├── oh-package.json5                       # 工程级 ohpm 依赖
└── README.md
```

关键一点：**鸿蒙工程不包含 QjackCtl 业务源码**。业务逻辑全部在外部 `libqjackctl.so` 中，由 `libqohos.so` 通过 `dlopen` / `dlsym("main")` 加载。`entry/src/main/cpp/CMakeLists.txt` 只编译一个示例性质的 `libentry.so`，不涉及 QjackCtl 本体。

---

## 二、环境要求

| 项目 | 要求 | 备注 |
| --- | --- | --- |
| DevEco Studio | 支持 HarmonyOS PC（Stage 模型）的版本 | 命令行构建用 `hvigorw` / `ohpm` |
| HarmonyOS SDK | `compatibleSdkVersion` / `targetSdkVersion` = `6.1.0(23)` | 见 `build-profile.json5` |
| Qt for OpenHarmony | `5.12.12`（arm64-v8a） | ArkTS 侧通过 `libqohos.so` 接入 |
| 目标设备 | `2in1` / `tablet`（arm64-v8a） | 见 `module.json5` 的 `deviceTypes`；不含手机（phone 模式有窗口恢复 bug，见 `module.json5` 注释） |
| 启动业务库 | `libqjackctl.so` | 见 `QtAppConstants.ets` |
| 签名 | 用自己的调试 / 发布证书 | 仓库里 `signingConfigs` 是本机配置，换机器要重配 |

应用信息（取自 `AppScope/app.json5`）：

- `bundleName`：`com.example.qjackctlpc`
- 显示名：QjackCtl
- 版本：`versionName 1.0.0` / `versionCode 1000000`

---

## 三、编译与运行

### 前置准备：放入 Qt 依赖库

`entry/libs/arm64-v8a/` 目录需要放入以下交叉编译产物（业务库 17 MB + 12 个 `.so` 依赖）：

```text
entry/libs/arm64-v8a/
├── libqjackctl.so             # QjackCtl 业务库
├── libqohos.so                # Qt-OHOS QPA 桥接库
├── libQt5Core.so / libQt5Gui.so / libQt5Widgets.so / ...
├── libc++_shared.so
├── platforms/libqohos.so
├── imageformats/
└── styles/
```

### 构建 HAP

```bash
# 1. 安装 ohpm 依赖
ohpm install

# 2. 用 DevEco Studio 打开仓库根目录
#    File → Project Structure → Signing Configs 配好签名

# 3. 构建
#    Build → Build Hap(s)/APP(s) → Build Hap(s)

# 产物路径
# entry/build/default/outputs/default/entry-default-signed.hap
```

命令行构建：

```bash
hvigorw --mode module -p module=entry@default -p product=default assembleHap
```

### 推送到设备

```bash
hdc install entry/build/default/outputs/default/entry-default-signed.hap
hdc shell aa start -a QAbility -b com.example.qjackctlpc
```

---

## 四、这个项目是怎么适配的

适配思路一句话：**ArkTS 壳层负责生命周期和窗口承载，QjackCtl 业务逻辑不动，由 Qt-OHOS 桥接层把两者连起来；JACK API 用 Shim 接口隔离层替代，让编译通过、UI 能跑。**

### 1. 启动链路

```text
HarmonyOS 拉起 QjackCtl（QAbility）
  → QAbilityStage.onCreate() 初始化 Qt-OHOS QPA
  → qpa.setupQtApplication({ appName: 'libqjackctl.so' })
  → QAbility.onWindowStageCreate() 加载 MainWindowNativeNode
  → MainWindowNativeNode 起 XComponent（libraryname: 'qohos'）
  → QPA 插件在 XComponent 上建 surface，dlopen("libqjackctl.so")
  → 调用 main(argc, argv)
  → QjackCtl Qt 主窗口渲染到 XComponent
```

### 2. JACK API 的 Shim 接口隔离层

这是这次适配里工程难度最高的一环。QjackCtl 上游强依赖 JACK 开发库（`libjack`），所有音频路由、端口连接、传输控制都通过 JACK API 调用。鸿蒙 PC 上没有 `libjack`，也没有 `jackd`（JACK 音频服务器）。

这次的方案是做了一个 **Shim 接口隔离层**——7 个 ABI 兼容头文件 + `static inline` 桩实现，把所有 JACK API 调用拦截下来，返回标准错误码（不是崩溃）。比如：

```text
// 原版
jack_client_open("qjackctl")  → 返回 jack_client_t* → 拿到端口列表 → 显示在 UI 上

// 鸿蒙 Shim 版
jack_client_open("qjackctl")  → static inline 返回 NULL
→ QjackCtl 拿到 NULL → "没有 server"
→ Connections 面板显示空白
```

这意味着所有依赖 JACK server 响应的功能——Connections 面板、音频路由配置、端口连接/断开、传输控制——在 UI 上表现为空白或不可操作，但不会崩溃。UI 本身是好的，纯 Qt 逻辑的菜单、设置面板、Patchbay 预设编辑器都能正常操作。

### 3. 编译层面的工程亮点

这次适配做了几个有特色的工程处理：

| 工程处理 | 说明 |
| --- | --- |
| **Shim 接口隔离层** | 7 个 ABI 兼容头文件 + `static inline` 桩实现，让 QjackCtl 不改源码就能编译通过 |
| **moc ABI 5.15→5.12 降级脚本** | 200 行 perl 脚本，把 moc 5.15 生成的 `QMetaObject::SuperData::link<>()` 改回 5.12 的 `&T::staticMetaObject` 语法 |
| **Q_INIT_RESOURCE 链接性传染** | `Q_INIT_RESOURCE` 宏会传染到所有链接该库的目标，需要特别处理 |
| **cdrv_main 双入口符号设计** | 业务库需要同时导出 `main` 和 `cdrv_main` 两个入口符号 |

编译产物：17 MB 业务库 + 12 个 `.so` 依赖，全部编译通过。

### 4. 权限声明（`module.json5`）

这个项目的权限比其他项目多一些，因为它是音频工具：

| 权限 | 用途 |
| --- | --- |
| `ohos.permission.SYSTEM_FLOAT_WINDOW` | 浮动窗口显示 |
| `ohos.permission.FILE_ACCESS_PERSIST` | Patchbay 配置文件读写 |
| `ohos.permission.PRINT` | 打印功能 |
| `ohos.permission.PREPARE_APP_TERMINATE` | 应用终止前保存状态 |
| `ohos.permission.STORE_PERSISTENT_DATA` | 持久化存储 |
| `ohos.permission.INTERNET` | JACK 网络音频传输 |
| `ohos.permission.READ_PASTEBOARD` | 读取剪贴板 |
| `ohos.permission.MICROPHONE` | 音频输入设备访问 |
| `ohos.permission.CAMERA` | 摄像头访问 |
| `ohos.permission.CUSTOM_SCREEN_CAPTURE` | 自定义屏幕捕获 |
| `ohos.permission.ACCELEROMETER` | 加速度计 |
| `ohos.permission.GYROSCOPE` | 陀螺仪 |
| `ohos.permission.APPROXIMATELY_LOCATION` | 大致位置 |
| `ohos.permission.LOCATION` | 精确位置 |
| `ohos.permission.GET_WIFI_INFO` | 网络状态检测 |
| `ohos.permission.ACCESS_BLUETOOTH` | 蓝牙访问 |
| `ohos.permission.GET_FILE_ICON` | 获取文件图标 |

### 5. 文件关联（`module.json5`）

`QAbility` 注册了 `file://` + `general.text` / `general.png` / `general.jpeg` 的技能声明，`maxFileSupported: 10`。从文件管理器打开文本和图片文件有接入路径。

---

## 五、功能支持情况（T0 / T1 / T2）

先把口径说清楚：

- 分档：T0 是「装、起、最低限度能用」，T1 是「纯 Qt UI 功能，不依赖 JACK server」，T2 是「音频路由、端口连接、传输控制这些需要 jackd 响应的核心功能」。
- 状态只有两个：**能用** 指功能正常；**未适配** 指桌面端特有或平台没有对应概念的能力，按平台特点主动换了做法或没做，是取舍不是没做完。

### T0 — 基础能力

| 能力 | 状态 | 说明 |
| --- | --- | --- |
| 编译打包成 HAP | 能用 | `hvigorw assembleHap` / DevEco 都能构建；需先放入 native 库 |
| 安装 / 启动 / 进主界面 | 能用 | ArkTS 起 `UIAbility` + `XComponent`，QPA 插件加载 `libqjackctl.so`，拉起 QjackCtl 主窗口 |
| 主窗口渲染 | 能用 | Qt Widgets 经 XComponent 宿主渲染，主窗口界面正常显示 |
| 从系统「打开方式」接收文件 | 能用 | `module.json5` 注册了 `file://` + `general.text` / `general.png` / `general.jpeg` 文件关联 |
| 进程隔离 | 能用 | `isolationProcess: true`，业务崩溃不影响壳 |

小结：装、起、进界面这条主链路是通的。Shim 接口隔离层让编译通过、UI 能跑、不崩溃。编译层面的工程难度极高——Shim 隔离层、moc ABI 降级脚本、Q_INIT_RESOURCE 链接性传染、cdrv_main 双入口符号，全部跑通。

### T1 — 纯 Qt UI 功能

| 能力 | 状态 | 说明 |
| --- | --- | --- |
| 主窗口 / 菜单 / 工具栏 | 能用 | 纯 Qt UI，正常显示 |
| 设置面板 | 能用 | 采样率、缓冲区、驱动参数等配置界面能操作 |
| Patchbay 预设编辑器 | 能用 | 纯 Qt 逻辑，创建/编辑/保存/加载路由预设的界面能操作 |
| 状态栏显示 | 能用 | 界面正常显示 |

小结：QjackCtl 的纯 Qt UI——主窗口、菜单、工具栏、设置面板、Patchbay 预设编辑器——都是好的，所有菜单都能点。问题不在这里，在下面 T2 的 JACK server 依赖上。

### T2 — JACK 音频功能（核心价值，需要 jackd）

| 能力 | 状态 | 说明 |
| --- | --- | --- |
| Connections 连接面板 | 未适配 | 鸿蒙 PC 没有 `jackd` 在运行，`jack_client_open` 返回 NULL，面板一片空白 |
| 音频路由配置 | 未适配 | 没有 JACK server，无法配置音频路由 |
| 端口连接 / 断开 | 未适配 | 需要 `jackd` 响应端口操作 |
| 传输控制（播放 / 暂停 / 录音） | 未适配 | 需要 JACK transport |
| Session 管理 | 未适配 | 需要 D-Bus + `jackd` |
| 状态监控（DSP 负载 / XRUN 计数） | 未适配 | 没有 `jackd`，拿不到实时性能数据 |
| 复杂子窗口 / 模态对话框 | 未适配 | Qt-OHOS 平台插件的子窗口 / 模态对话框实现不完整 |
| 打印 | 未适配 | 声明了 `PRINT` 权限，但鸿蒙 PC 没有 CUPS 打印子系统 |
| Embedded UI | 未适配 | 桥接代码已预留，但宿主页仍是模板内容，不是成熟功能 |
| `x86_64` 模拟器 | 未适配 | 构建配置只开了 `arm64-v8a` |
| 手机设备 | 未适配 | `module.json5` 的 `deviceTypes` 不含 `phone`；注释写明 phone 模式有窗口恢复 bug（QTFOROH-1076） |

小结：T2 这批里，最核心的问题是——**这是一个音频路由工具，但没有音频后端。** 就像一台调音台能亮灯、能拧旋钮、能看面板——但接不上任何音频信号。Shim 拦截层做了一个技术让步：所有 JACK API 调用返回标准错误码（`jack_client_open` 返回 NULL），不是崩溃。所以 Connections 面板是空白的，不是报错。UI 完全可用，但它的核心价值——音频路由和端口连接——在鸿蒙 PC 上用不了，因为鸿蒙 PC 没有 `jackd`。

---

## 六、整体到什么程度

把三档合起来看：

- **T0 基础能力**：宿主工程结构、启动链路、主窗口承载、文件关联都到位，能编能装能起。编译层面的工程难度极高——Shim 接口隔离层、moc ABI 降级脚本、Q_INIT_RESOURCE 链接性传染、cdrv_main 双入口符号，全部跑通，17 MB 业务库 + 12 个 `.so`。
- **T1 纯 Qt UI 功能**：主窗口、菜单、工具栏、设置面板、Patchbay 预设编辑器都正常——这部分没问题。
- **T2 JACK 音频功能**：Connections 面板空白、音频路由配置、端口连接/断开、传输控制、Session 管理——这些都需要 `jackd`（JACK 音频服务器），鸿蒙 PC 上没有，全用不了。Shim 拦截层返回标准错误码而不是崩溃，所以 UI 不会崩，只是面板空白。

一句话总结：这次把 QjackCtl 完整搬到了鸿蒙 PC 上——编译难度极高，做了 Shim 接口隔离层、moc ABI 降级脚本、cdrv_main 双入口符号等工程处理，17 MB 业务库 + 12 个 `.so` 全部编译通过。主窗口、菜单、工具栏、设置面板、Patchbay 预设编辑器这些纯 Qt UI 都正常。但这是一个音频路由工具，鸿蒙 PC 上没有 `jackd`（JACK 音频服务器），所以 Connections 面板一片空白、音频路由配置和端口连接全用不了——Shim 层返回 NULL 而不是崩溃，UI 不会崩，只是没有音频后端可连。这不是 bug，是平台没有 JACK 音频后端。
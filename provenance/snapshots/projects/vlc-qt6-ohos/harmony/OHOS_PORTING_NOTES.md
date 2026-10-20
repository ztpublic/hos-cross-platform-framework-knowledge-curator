# VLC 3.0.21 Qt GUI for HarmonyOS：移植记录与踩坑总结

本文记录将 VLC 3.0.21 原版 Qt Widgets GUI 移植到 HarmonyOS 的实现、调试过程、已解决问题和失败尝试。文档按实际排障时间保留历史，因此第 4、5、10、11 节中的“当前问题”和包信息只代表当时阶段；最终可用基线及结论以第 0、9、12--16 节为准。后续修改前应先阅读本文，避免重复已经证伪的方案。

## 0. 当前可用基线

截至 2026-08-02，实机已确认以下功能可用：

- Qt Widgets 主界面能够在 HarmonyOS 启动和操作，应用显示名为 `VLC`，使用 VLC 锥形图标。
- 默认 `hardware-texture` 后端可以播放视频，窗口缩放不再依赖有问题的解码 Surface 直接铺放。
- SRT 文本字幕、外挂 ASS 字幕和 MKV 内嵌 ASS 字幕均能显示。
- 文件选择器返回的 URI 会持久化并激活读权限；重新播放和添加外挂字幕不会因临时授权过期而失败。
- Want 文件关联能够把外部“用 VLC 打开”的音视频 URI 转交给现有 Qt/VLC 播放列表。
- AVSession 已接入播放状态、媒体标题、时长、进度、播放/暂停/停止、上一首/下一首、绝对与相对跳转。
- 后台音频播放声明和 `KEEP_BACKGROUND_RUNNING` 权限已加入生成包。
- 字幕延迟无需另加控件；原界面已有 `工具 -> 轨道同步`，最终写入 VLC 的 `spu-delay`。

当前综合未签名包由构建脚本输出到：

```text
harmony/build/harmony-package/entry/build/default/outputs/default/entry-default-unsigned.hap
```

根目录下的便捷 HAP 副本、构建目录、复制进来的 QtBase/node-addon-api 和所有第三方二进制均为本机产物，已由 `.gitignore` 排除，不应提交。后续验证以直接部署和设备日志为准，不再花时间做哈希复核。

## 1. 工程与环境

- VLC 工程：`E:\source\vlcplayer`
- 可参考的最小 VLC HarmonyOS 实现：`E:\source\vlc`
- VLC 库侧 WSL 源码：`/home/huzheng/tpc_c_cplusplus-current/thirdparty/vlc/vlc-ohos-3.0.21`
- Windows 侧 VLC SDK：`E:\source\vlc\vlc-ohos-3.0.21-arm64-v8a-dev-win\usr\vlc\arm64-v8a`
- FFmpeg 6 OHOS 源码：`/home/huzheng/tpc_c_cplusplus-current/community/FFmpeg-surface-dev/FFmpeg`
- VLC 版本：3.0.21
- Qt：6.12.0 for HarmonyOS arm64-v8a
- Qt 安装目录：`E:\Qt\6.12.0`
- Qt Maintenance Tool 安装的精确配套源码：`E:\Qt\6.12.0\Src\qtbase`
- 配套源码提交标签：`97575d35c0cecdc0fb4e12fc3575afaa9fd9d3f1`
- 复制到工程内的配套 Qt 源码：`harmony\qtbase-sdk-src`
- 配套 Qt 构建目录：`harmony\build\qtbase-sdk-ohos`
- 旧的、不匹配的 Git 分支源码：`harmony\qtbase-6.12.0`，仅供对照，不能用于打包
- VLC/HAP 主构建目录：`harmony\build`
- 未签名 HAP：`harmony\build\harmony-package\entry\build\default\outputs\default\entry-default-unsigned.hap`
- hdc：`E:\Huawei\DevEco_Studio\sdk\default\openharmony\toolchains\hdc.exe`
- Bundle：`org.videolan.vlc.qt`
- Ability：`QAbility`

用户负责签名和部署。不要自行签名或覆盖用户的签名配置，也不要再为交付包做哈希复核；重新构建后直接部署验证，必要时结合文件大小、修改时间和 `hilog` 判断是否用了新包。

主构建命令：

```powershell
cd E:\source\vlcplayer\harmony
.\build-harmonyos.ps1 -Configuration Debug -Bundle
```

Qt 平台插件增量构建命令：

```powershell
cd E:\source\vlcplayer\harmony\build\qtbase-sdk-ohos
cmake --build . --target QOhosPlatformIntegrationPlugin --parallel 16
```

机器为 Ryzen 9 9950X3D，Qt 构建使用 `--parallel 16`，不要再使用 `--parallel 1`。此前工具超时只杀掉外层 PowerShell，曾遗留三条后台 `ninja -j 1`；重新启动构建前可检查是否存在遗留 Ninja。

## 2. 已完成的基础移植

### 2.1 Qt 6 API 迁移

原版 VLC 3.0.21 Qt GUI 使用大量 Qt 5 API。工程内已经批量迁移，包括但不限于：

- `QDesktopWidget` 改为 `QScreen` / `QGuiApplication::screens()`。
- `QLinkedList` 改为 `QList`。
- `qSort` 改为 `std::sort`，同类弃用 API 应全工程搜索后批量处理，不要碰到一个改一个。
- `QLayout::setMargin` 改为 `setContentsMargins`。
- Qt 6 鼠标事件位置 API、`nativeEvent` 参数、随机数 API 等迁移。
- 多个 Qt Widgets、EPG、播放列表、插件和设置界面的 Qt 6 编译修复。

这部分当前可以成功编译，不应回退。

### 2.2 Qt GUI 必须运行在 HarmonyOS 主线程

VLC 原版会在自己的线程中启动 Qt GUI，而 HarmonyOS Qt 平台要求 UI 主线程。当前实现通过 `vlc_qt_harmony_run_main_thread()` 和条件变量把 VLC Qt GUI 启动桥接到 HarmonyOS 主线程。

相关文件：

- `modules/gui/qt/qt.cpp`
- HarmonyOS 启动侧代码位于 `harmony` 工程中。

此修复解决了早期窗口白屏、UI 不工作等问题，应保留。

### 2.3 Privacy and Network Access Policy 空白弹窗

原版首次启动的隐私/网络访问策略对话框在 HarmonyOS 上内容空白，且关闭按钮无法工作，同时会导致主界面白屏。HarmonyOS 构建中已跳过该对话框。

### 2.4 Watchdog

HarmonyOS 下关闭了不适用的 VLC/Qt watchdog 路径，避免误判和退出。

### 2.5 `unidentified codec` 假错误

HarmonyOS codec/解码输出路径与桌面平台不同。视频实际能播放、有声音时，VLC 仍可能弹出：

`VLC could not identify the audio or the video codec`

HarmonyOS 下已经过滤这个误报。不要用该弹窗判断实际解码失败，应结合画面、声音和日志判断。

## 3. 视频 Surface：已解决的泛白问题

### 3.1 症状

早期版本播放视频时有声音，但画面严重泛白，只能隐约看到内容，同时会出现 unidentified codec 弹窗。

### 3.2 根因与有效方案

不能在 VLC 已经开始创建视频输出后才临时取得或替换 Surface。当前有效实现为：

1. `VideoWidget` 预先创建 `QWindow`。
2. 使用 `QWidget::createWindowContainer()` 嵌入该窗口。
3. 通过 Qt OHOS 平台函数 `setSurfaceConsumer` 获取该窗口对应的 `OHNativeWindow`。
4. 在播放前把指针注册到 VLC object tree 的 `drawable-ohos-nativewindow-ptr`。
5. `vout_window_t::handle.onativewindow` 使用该原生窗口。

相关文件：

- `modules/gui/qt/components/interface_widgets.cpp`
- `modules/gui/qt/components/interface_widgets.hpp`
- `include/vlc_vout_window.h`
- `modules/gui/qt/qt.cpp`

该方案解决了泛白问题，视频目前能够正确解码和显示，不能回退成播放期间重建 Surface。

## 4. 历史诊断：视频 NativeNode 几何不正确

本节记录已经结束的 native-surface 排障阶段。最终默认方案已切换到第 12 节的
`hardware-texture`；这里的“当前”均指当时诊断状态，不再是现版本遗留问题。

### 4.1 当前稳定复现

- 调整 VLC 窗口大小后，视频位置和大小会发生变化，说明某条几何更新链正在运行。
- 变化后的矩形仍与 Qt 视频区域不一致。
- 在全屏模式下直接打开视频，首次出现时尺寸就已经错误，所以问题不只是 resize 事件或 resize 时序。
- 2026-08-02 的全屏截图为 2560×1600，视频 Surface 只覆盖左上方约 1440×840，右侧和底部留有大块白色 Qt 背景。
- 截图保存在：`harmony\build\vlc-geometry-current.jpeg`。

这表明需要区分至少三层几何：

1. `VideoWidget` / `stable` 的 QWidget 几何。
2. `surfaceWindow` 的 QWindow 几何及其父 QWindow 坐标系。
3. Qt OHOS 插件中 ArkUI NativeNode 的实际 PX 位置、大小和 Surface 分辨率。

不能再只看其中一层就推断根因。

### 4.2 当前 Qt 平台补丁

配套 Qt 源码的：

`harmony\qtbase-sdk-src\src\plugins\platforms\ohos\qohosplatformwindow.cpp`

在 `QOhosPlatformWindow::setGeometry()` 后，对可见的 `EmbeddedWindow` 调用：

```cpp
view->forceGeometryUpdate();
```

原版 `setGeometry()` 只更新 `QPlatformWindow`，不会主动把新矩形推给 ArkUI NativeNode。`forceGeometryUpdate()` 会把位置和大小放入 `QOhosView` 的 pending update，再调用 NativeNode 的 `setPosition` / `setSize`。

该补丁可以编译并保持 ABI 一致，但目前单独使用并未修正视频矩形。

### 4.3 当前 VLC 侧几何尝试

`VideoWidget::resizeEvent()` 中目前会激活布局并更新 `surfaceWindow`。位置使用：

```cpp
stable->mapTo(stable->window(), QPoint(0, 0))
```

目的是将 QWidget 位置转换到顶层窗口坐标，而不是简单传 `stable->rect()` 的 `(0,0)`。实际部署后仍没有解决问题，因此该坐标是否与 `surfaceWindow->parent()` 的坐标系一致仍未被证实。

下一步应打印：

- `VideoWidget::rect()`、`geometry()`、`size()`、`devicePixelRatioF()`。
- `stable->rect()`、`geometry()`、映射到顶层窗口和映射到 `surfaceWindow->parent()` 的位置。
- `surfaceWindow->geometry()`、`frameGeometry()`、`parent()`、`devicePixelRatio()`。
- `QOhosPlatformWindow::geometry()`、`lastRequestedWindowFrameGeometry()`、`frameMargins()`。
- `QOhosView::updateWindowPosition()` / `updateWindowSize()` 的输入。
- `QNativeNode::geometry()` 与 Surface 回调报告的分辨率。

日志统一使用一个容易过滤的标识，例如 `VLC_OHOS_GEOMETRY`。

## 5. 已尝试但无效或有副作用的方案

### 5.1 只设置 NativeWindow scaling mode

尝试：

```cpp
OH_NativeWindow_NativeWindowSetScalingModeV2(
    nativeWindow, OH_SCALING_MODE_SCALE_FIT_V2);
```

结果：没有解决 Surface 的位置/大小问题。该 API 控制缓冲内容如何适配 Surface，不负责 ArkUI NativeNode 在 Qt 窗口中的几何。

### 5.2 只强制 QWidget/QWindow geometry

多次尝试激活布局、设置 `stable` 和 `surfaceWindow` geometry。结果最多让视频随窗口发生变化，但位置和大小仍不正确。

结论：问题跨越 Qt Widgets、QWindow 和 ArkUI NativeNode，单独强制某一层不足以解决。

### 5.3 resize 时重建 Surface 并重启 VIDEO_ES

尝试在窗口变化时销毁/重建 Surface，并重新启动 VLC 视频 ES。

结果：

- 视频开始跟着变化，但位置、大小仍不正确。
- 播放过程中会在视频、VLC 初始黑屏和带 Logo 的界面之间闪烁。
- 用户确认部分“会动”只是偶然卡顿，并非正确修复。

该方案已删除。不要重新引入 resize 时重建 Surface 或重启 VIDEO_ES。

### 5.4 父窗口 eventFilter 强制几何

尝试通过父窗口 eventFilter 反复推送 geometry。无效且增加更新竞争，已经删除。

### 5.5 把所有 EmbeddedWindow 统一乘 DPR

根据截图曾推测 ArkUI `ARKUI_LENGTH_METRIC_UNIT_PX` 与 Qt 逻辑像素之间缺少 DPR 换算，于是在 `QOhosView::updateWindowSize()` 和 `updateWindowPosition()` 中对所有没有 `m_ohosWindowProxy` 的窗口乘 `devicePixelRatio()`。

结果：其他 Qt 控件和嵌入窗口也发生错误，属于全局回归。用户要求立即回退，现已回退。

结论：即使视频 Surface 存在像素单位问题，也不能在通用 Qt 平台层对所有 EmbeddedWindow 统一缩放。若最终证实需要换算，必须只针对视频窗口，通过明确属性/窗口角色限定，且要分别验证 position、node size 和 Surface buffer size，不能同时盲乘。

### 5.6 参考 AloePlayerPC/mpv 的误区

`AloePlayerPC` 普通 mpv 播放使用 `QOpenGLWidget`，每帧绘制到当前 Qt FBO，并使用 `width() * devicePixelRatioF()`。它天然跟随 Qt 布局缩放，不等价于 VLC 当前使用独立 OHOS Native Surface 的方案。

Aloe 的 HDR overlay 才使用 Native Surface，可作为更接近的参考，但不能据普通 mpv 路径推断 Qt 平台插件无需处理 NativeNode 几何。

## 6. Qt 源码与 ABI 大坑

### 6.1 错误源码分支导致启动即闪退

最初从 Qt 官方 6.12 分支拉取源码到：

`harmony\qtbase-6.12.0`

该分支与本机 Maintenance Tool 安装的 6.12.0 二进制不是同一提交。虽然插件能编译，但打包后应用在 `QAbilityStage.onCreate()` 立即闪退。

FaultLogger 的 JS 错误：

```text
TypeError: Cannot read property handleAbilityStageOnCreate of undefined
at QAbilityStage.ets:41
```

真正的动态链接错误为：

```text
Error relocating libqohos.so:
registerQOhosOnOffMethodsBasedEventHandler(...): symbol not found
```

旧分支插件引用了未带正确版本或安装版 QtCore 不存在的私有符号。继续尝试链接安装版 Qt 库时还暴露出大量其他私有 ABI 差异。

### 6.2 正确做法

通过 Qt Maintenance Tool 安装源码，使用：

`E:\Qt\6.12.0\Src\qtbase`

其 `.tag` 为：

```text
97575d35c0cecdc0fb4e12fc3575afaa9fd9d3f1
```

复制到 `harmony\qtbase-sdk-src` 后重新配置和构建。新插件与安装版原插件各有 1507 个未定义外部符号，集合差异为 0，证明 ABI 对齐。

### 6.3 node-addon-api 也必须匹配

最初使用自行克隆的 `harmony\node-addon-api`，外部符号对比仍有一个差异：

- 一边使用 `napi_create_external`
- 另一边使用 `napi_add_finalizer`

改用 DevEco 随附的：

`E:\Huawei\additional-packages\include`

作为 `NodeAddonApi_INCLUDE_DIR` 后，1507 个外部符号完全一致。

不要再用自行克隆的新版 node-addon-api 构建配套 Qt 插件。

## 7. 调试命令

启动应用：

```powershell
$hdc = 'E:\Huawei\DevEco_Studio\sdk\default\openharmony\toolchains\hdc.exe'
& $hdc shell aa start -a QAbility -b org.videolan.vlc.qt
```

清空并读取日志：

```powershell
& $hdc shell hilog -r
& $hdc shell hilog -x -v time
```

截图：

```powershell
& $hdc shell 'snapshot_display -f /data/local/tmp/vlc.jpeg'
& $hdc file recv /data/local/tmp/vlc.jpeg E:\source\vlcplayer\harmony\build\vlc.jpeg
```

列出 FaultLogger：

```powershell
& $hdc shell "hidumper -s 1201 -a '-p Faultlogger'"
```

读取指定故障记录：

```powershell
& $hdc shell "hidumper -s 1201 -a '-p Faultlogger -f <fault-file-name>'"
```

## 8. 当时的后续建议（历史）

1. 不再做无日志的几何猜测；先加入三层矩形追踪。
2. 给视频 `surfaceWindow` 设置一个仅本项目使用的动态属性或平台窗口标记，便于 Qt 插件只识别视频窗口，避免影响其他 EmbeddedWindow。
3. 明确 `surfaceWindow->parent()` 是哪个 QWindow，使用与该父窗口一致的坐标系计算位置；不要默认顶层 QWidget 就是 NativeNode 父节点。
4. 分别测量逻辑尺寸、NativeNode PX 尺寸和 Surface buffer 分辨率。三者可能需要不同换算。
5. 在窗口化、拖动 resize、最大化、进入全屏、全屏下直接打开视频五种场景分别记录数据。
6. 保持同一个 Surface，不在 resize 时重建或重启视频 ES。
7. Qt 平台插件只能从 `qtbase-sdk-src` 构建，并在打包前继续做 ABI 外部符号集合对比。

## 9. 当前状态摘要

- Qt GUI、视频、音频、中文界面、文件持久授权、Want 文件关联和 AVSession 均已接通。
- 默认视频输出为 `hardware-texture`：OHOSAVCodec 输出到 NativeImage，再作为
  `GL_TEXTURE_EXTERNAL_OES` 由 Qt OpenGL 窗口绘制。native-surface 几何问题不再阻塞默认路径。
- SRT 走 OHOS Qt 文本 renderer；ASS/SSA 走 libass RGBA region，再通过公共 C ABI 桥接到
  OpenGL 字幕纹理。两类字幕有独立 owner，清屏互不覆盖。
- 外挂 ASS 和 MKV 内嵌 ASS 均已实机确认显示。内嵌 ASS 的最终根因及修复见第 14 节。
- `libass.so.9` 仍是动态库；FriBidi 等它所依赖的库被静态链接进 libass，不能误写成
  “libass 整体静态链接”。
- 音频默认补偿为 `-750 ms`，可用环境变量或原“轨道同步”界面调整；字幕延迟同样使用
  “工具 -> 轨道同步”。
- 不要再尝试全局 DPR 缩放、手动覆盖所有 EmbeddedWindow、resize 时重建 Surface、延迟
  NativeImage 消费，或把 MKV ASS 包送入 `BuildSsaFrame()`。

## 10. 2026-08-02：两轮几何修复尝试（均未完全解决）

对 2560×1600 截图和设备显示参数重新核对后，确认旧截图中的视频矩形约为
1828×1080 px。设备当前逻辑密度为 1.9125；该矩形换算后接近 960×540，恰好是
1920×1080 片源被 `devicePixelRatioF()` 换算后的 Qt 逻辑尺寸。

已确认存在两处问题，但修正后仍不足以解决最终 Native Surface 大小：

1. `MainInterface::getVideoSlot()` 先执行 `setVideoFullScreen(true)`，随后仍在
   `b_autoresize` 分支调用 `videoWidget->setSize(sourceWidth / DPR, sourceHeight / DPR)`。
   这会在进入全屏后把 `QStackedWidget` 页面重新压回片源逻辑尺寸，直到后续某次
   布局才可能改变。因此“全屏下直接打开视频首次尺寸就错误”并非 Surface buffer
   分辨率问题，而是 VLC 侧用片源尺寸覆盖了全屏页面几何。
2. `stable` 设置了 `WA_NativeWindow`，所以 `createWindowContainer()` 会把
   `surfaceWindow` 挂到 `stable` 的原生 `QWindow` 下，并以 `stable->rect()` 作为
   父相对几何。VLC 侧又把 `stable->mapTo(stable->window(), ...)` 的顶层坐标手动写入
   `surfaceWindow`，导致父窗口位置被重复叠加，同时与 `QWindowContainer` 自带的
   move/resize 同步竞争。

第一轮实施的修复：

- 全屏或最大化时不再按片源尺寸调用 `videoWidget->setSize()`，页面尺寸完全交给
  `QStackedWidget`。
- 不再从 VLC 侧调用 `surfaceWindow->setGeometry()`；`surfaceWindow` 的父相对位置和
  大小完全交给 `QWindowContainer`。
- 保留 Qt OHOS 插件中 `QOhosPlatformWindow::setGeometry()` 对 EmbeddedWindow 的
  `forceGeometryUpdate()`，使 Qt 已转换为 native pixels 的平台几何继续下发到 ArkUI
  NativeNode；不要额外乘 DPR。
- `build-harmonyos.ps1` 已改回 `--parallel 16`。

Debug 未签名 HAP 已成功生成，包内 `libqt_plugin.so` 和 `libqohos.so` 与各自构建产物
SHA-256 一致；新旧 `libqohos.so` 的 1506 个未定义动态符号集合差异为 0。签名部署后的
实机截图证明第一轮修复没有解决大小问题。

### 10.1 第一次修复后的实机结果与进一步修复

第一次修复部署后仍未完全解决。新截图 `harmony/build/vlc-current-failed.jpeg` 中，视频
左上角已经与目标区域对齐，说明重复写入顶层坐标的问题已解决；但视频 Surface 仍保持
约 1920×1080 px，窗口客户区变大后右侧和底部继续留白。这说明剩余问题是视频子
NativeNode 没有随其原生容器父节点铺满，而不是 position 或全局 DPR。

第二次修复：

- 给 VLC 的 `surfaceWindow` 设置私有动态属性 `_q_vlc_ohos_parent_fill`。
- Qt OHOS 插件只在看到该属性时，对 NativeNode 调用 `fillToParent()`，即 position 为
  `(0,0)`，宽高为父节点的 100%；其他 EmbeddedWindow 仍使用 Qt 的标准 point 几何。
- HarmonyOS 下 `VideoWidget::setSize()` 不再直接 resize 这个由 `QStackedWidget` 管理的
  页面，只发出顶层窗口尺寸请求并报告实际尺寸。即使 OHOS 窗口管理器异步处理或拒绝
  顶层 resize，视频页面也不会残留在片源尺寸。

还发现主 `build-harmonyos.ps1` 原先不会自动重编 `build/qtbase-sdk-ohos`，可能出现
“Qt 平台源码已改、HAP 仍打入旧 `libqohos.so`”的假部署。脚本现已在主工程构建前显式
构建 `QOhosPlatformIntegrationPlugin`。交付前应从最终 HAP 解出 `libqohos.so`，确认其中
包含 `_q_vlc_ohos_parent_fill` 字符串；不能只比较源码或未打包的构建产物。

### 10.2 第二次修复后的实机结果：仍失败

第二轮修复签名部署后仍无可见改善。当前基线截图为
`harmony/build/vlc-parent-fill-failed.jpeg`，分辨率 2560×1600。视频左上角基本对齐，
但 Surface 仍约为 1828×1080 px，右侧和底部存在大块白色区域；打开菜单不影响该结果。

已从最终 HAP 解包确认 `libqt_plugin.so` 和 `libqohos.so` 都包含
`_q_vlc_ohos_parent_fill`，因此这次不是“平台插件没有重编”或“打入旧库”的问题。
`build-harmonyos.ps1` 中显式构建 `QOhosPlatformIntegrationPlugin` 的步骤应保留。

当前只能得出以下结论，尚不能称为根因已解决：

- `fillToParent()` 很可能只是铺满了一个本身仍停留在片源尺寸的直接原生父节点。
  最可疑的是设置了 `WA_NativeWindow` 的 `stable` QWidget：`surfaceWindow` 被挂在它的
  NativeNode 下，如果 `stable` 的原生节点没有跟随 Qt 布局变化，子节点铺满父节点后仍是
  约 1828×1080 px。
- 另一种可能是带标记的更新分支执行时机过早，随后又被普通几何更新覆盖。没有运行时
  调用顺序和 NativeNode 实际矩形日志前，不能继续凭截图二选一。

以下方案已经证实无效或不足，不要原样重复：

- 全屏/最大化后跳过按片源尺寸直接 resize 页面。
- 删除 VLC 侧对 `surfaceWindow` 的手工 `setGeometry()`。
- 对 EmbeddedWindow 通用调用 `forceGeometryUpdate()`。
- 仅给视频子 NativeNode 调用 `fillToParent()`。
- 全局把 EmbeddedWindow 几何再次乘 DPR；该方案会破坏其他控件。
- resize 时重建 Surface 或重启视频 ES；该方案会闪烁并破坏播放连续性。

下次继续时按下面顺序做：

1. 暂停新的几何修补，先统一加入 `VLC_OHOS_GEOMETRY` 运行时日志。
2. 同时记录 `VideoWidget`、`stable`、`surfaceWindow` 的 Qt geometry、parent 和 DPR，
   `QOhosPlatformWindow` 的请求几何，以及 ArkUI 布局完成后 NativeNode 的实际屏幕矩形、
   父相对矩形和 Surface buffer 分辨率。仅记录请求值不够。
3. 明确打印 `_q_vlc_ohos_parent_fill` 分支是否执行、执行次数和执行后的直接父节点矩形，
   查清它是铺错父节点还是随后被覆盖。
4. 日志若确认 `stable` 的原生节点停在旧尺寸，优先做一个 OHOS 专用对照实验：移除
   `stable->setAttribute(Qt::WA_NativeWindow, true)`，让 `QWindowContainer` 把
   `surfaceWindow` 直接挂到顶层窗口，绕开嵌套的 `stable` NativeNode。这只是下一条假设，
   不是已确认修复。
5. 如果 `stable` 必须保持原生窗口，则应让它自己的 NativeNode 跟随 QWidget 布局，或给
   `stable` 单独建立受控的父填充路径；仍需先用日志确认，避免扩大插件影响范围。

当前源码保留了第二轮 `_q_vlc_ohos_parent_fill` 实验，方便下次加日志对照，但其状态必须
视为“未解决”，不能作为已完成方案交付。

### 10.3 额外实机线索：窗口宽高与视频宽度发生反常耦合

第二轮部署后又做了手工拖拽测试，观察到的不是普通“Surface 固定不动”：

- 横向拉长窗口时，视频反而横向变窄，同时视频左边缘向右移动，不再贴近内容区左边。
- 竖向拉长窗口时，视频横向变长。
- 上述过程中视频纵向高度始终没有变化。

当前状态截图为 `harmony/build/vlc-axis-coupling.jpeg`（2560×1600）。截图中主窗口约从
x=110 到 x=1855，视频约从 x=240 到 x=1480；左右白边明显不对称。这组响应不能用
普通等比例缩放、单纯 DPR 错误或“子节点保持固定尺寸”完整解释。它更像位置、尺寸、
父节点百分比布局和旧的点坐标缓存正在相互反馈；也可能同时混入 VLC 根据错误
`WindowResized` 尺寸调整画面视口的效果。

代码中还有一个需要优先验证的具体疑点：`QNativeNode::fillToParent()` 只把 ArkUI 节点的
position 设置为 `(0,0)`、宽高设置为父节点的 100%，却没有同步更新 `m_nodeGeometry`；而
`QNativeNode::geometry()` 仍直接返回这份旧的点坐标缓存。若后续视图/重挂父节点流程读取
该缓存，就可能把百分比布局后的实际矩形与旧请求矩形混用。此处目前只是与现象吻合的
候选机制，不能在没有日志时直接修改。

下次采样应增加两个受控拖拽序列，并逐帧记录增量关系：

1. 只改变顶层窗口宽度，记录顶层、`VideoWidget`、`stable`、`surfaceWindow`、两层
   NativeNode 和 Surface buffer 的 x/y/w/h。
2. 恢复初始状态后只改变顶层窗口高度，记录同一组值。
3. 同时打印 `m_nodeGeometry` 缓存和 ArkUI `GetLayoutPosition/GetLayoutSize` 实测值，确认
   反向变化产生在 Qt 布局、NativeNode 布局还是 VLC 画面视口层。

在完成这组对照前，不应把现象笼统归因于“宽高写反”；严格来说目前只确认视频宽度同时
受窗口宽度和高度反常影响，而视频高度被锁住。

### 10.4 10.3 诊断版本（等待签名部署采样）

已按 10.3 加入统一标记 `VLC_OHOS_GEOMETRY`，本版本只增加日志，不继续修改几何策略。

VLC QWidget 层在 Surface 注册、`setSize()` 请求和每次 `reportSize()` 时记录：

- 顶层 QWidget、`VideoWidget`、`stable` 的 geometry。
- `stable` 映射到顶层后的坐标及其原生 QWindow geometry。
- `surfaceWindow`、它的直接 QWindow parent 及双方 geometry。
- 报给 `WindowResized()` 的 physical size 和 DPR。

Qt OHOS 平台层在 `QOhosPlatformWindow::setGeometry()`、计划/执行 size/position 更新、
ArkUI area change 和 Surface status change 时记录：

- Qt 请求矩形、QWindow geometry 和 `m_lastRequestedWindowFrameGeometry`。
- `QNativeNode::m_nodeGeometry` 所代表的旧点坐标缓存。
- ArkUI `GetLayoutPosition/GetLayoutSize` 得到的父相对及屏幕实际矩形。
- Surface buffer 分辨率。
- 视频节点的直接父 QWindow、父节点缓存和父节点 ArkUI 实际矩形。

Debug 未签名诊断包：

`harmony/build/harmony-package/entry/build/default/outputs/default/entry-default-unsigned.hap`

- 生成时间：2026-08-02 10:21:01
- 大小：228002834 bytes
- 包内 `libqohos.so` SHA-256：
  `F0BEEE489065610D1610C0C727444A232196D805413E85A3408EC7BBEC07D80A`
- 包内 `libqt_plugin.so` SHA-256：
  `BD6006578BBA63407865EB46F5D22A7658633EAEDA53593C78602B5605215A26`
- 已从最终 HAP 解包确认两个库均包含 `VLC_OHOS_GEOMETRY`。

签名部署后按以下顺序采样，避免两种拖拽混在同一段日志里：

1. 启动 VLC、打开同一个视频，恢复到容易复现的窗口大小。
2. 清空 hilog，仅横向拖动右边缘使窗口变宽，停止后保持约 1 秒。
3. 立即导出包含 `VLC_OHOS_GEOMETRY` 的日志，命名为 `geometry-width.log`。
4. 恢复窗口初始大小，再次清空 hilog。
5. 仅竖向拖动下边缘使窗口变高，停止后保持约 1 秒。
6. 导出同一标记的日志，命名为 `geometry-height.log`。

PowerShell 采样命令：

```powershell
$hdc = 'E:\Huawei\DevEco_Studio\sdk\default\openharmony\toolchains\hdc.exe'
& $hdc shell hilog -r
# 完成一次单轴拖拽后执行：
& $hdc shell hilog -x -v time |
    Select-String 'VLC_OHOS_GEOMETRY' |
    Set-Content -Encoding utf8 E:\source\vlcplayer\harmony\build\geometry-width.log
```

第二组清空日志后将输出文件名改成 `geometry-height.log`。分析时首先比较每条链路中
`qwindow`、`cache`、`actualParent`、`parentActualScreen` 和 `surface` 的增量；只有这样才能
判断反常耦合最早出现在哪一层。

### 10.5 单轴采样结论：容器几何正确，问题转移到缓冲缩放层

已在视频暂停状态完成两次横向采样和一次竖向采样。最终有效样本保存在：

- `harmony/build/geometry-width.log`、`harmony/build/geometry-width.jpeg`
- `harmony/build/geometry-height.log`、`harmony/build/geometry-height.jpeg`

横向采样的最终目标 Surface 为 `2087x1081`，竖向采样的最终目标 Surface 为
`2087x1174`。两组日志中，Qt logical size 乘以 DPR 后都与下列实测值一致：

- 视频 ArkUI NativeNode 的 `actualParent` 矩形；
- 父 NativeNode 的实际矩形；
- OHNativeWindow Surface buffer 分辨率。

因此 Qt 布局、QWindowContainer、ArkUI NativeNode 和 Surface 的尺寸传递链路已经正确。
`fillToParent()` 后 `m_nodeGeometry` 仍保留旧点坐标确实是缓存不一致，但本次日志证明
ArkUI 实际布局和 Surface 已到达目标尺寸，它不是当前画面错位的直接原因。

截图中 Surface 覆盖了完整视频内容区，但暂停画面只占其中一部分，新增区域显示为白色。
这说明剩余问题发生在解码缓冲内容到 Surface 的缩放阶段，而不是 Surface 自身的位置或
大小。暂停测试也解释了“窗口高度变了但画面高度不变”：Surface 已变大，最后一帧缓冲
仍按旧的适配策略显示。

此前注册 Surface 时使用 `OH_SCALING_MODE_SCALE_FIT_V2`。该模式按缓冲比例完整显示内容，
窗口与缓冲比例不同时会用背景色填充未占用区域；这与当前白边、居中偏移以及画面尺寸
不完全跟随 Surface 的现象一致。为避免 NativeWindow 在 VLC 已完成视口计算后再做一次
独立的等比例适配，下一诊断包改为：

```cpp
OH_NativeWindow_NativeWindowSetScalingModeV2(
    nativeWindow, OH_SCALING_MODE_SCALE_TO_WINDOW_V2);
```

该模式会在两个方向把缓冲匹配到 Surface。部署后应分别验证播放中、暂停后拖动窗口以及
全屏切换；若画面填满但宽高比失真，则下一步应检查 OHOSAVCodec 生产端是否收到 VLC 的
显示视口，而不是退回修改已经确认正确的 Qt/ArkUI 几何链路。

本次 `SCALE_TO_WINDOW` Debug 未签名包仍位于：

`harmony/build/harmony-package/entry/build/default/outputs/default/entry-default-unsigned.hap`

- 生成时间：2026-08-02 10:35:52
- 大小：228003128 bytes
- HAP SHA-256：
  `29212175BFF167CEACBCD5477332793CC609F912539DAABD355725C1B5D5DDAB`
- 包内来源 `libqt_plugin.so` SHA-256：
  `33F663B609BB0CFC23D8E57087024943D2EB2CDB74A878EC0E448326299C0D99`
- 已直接检查 HAP 内两份 `libqt_plugin.so`，均包含 `scale-to-window` 新日志字符串。

另：此设备的 `hilog -d` 中 `-d` 表示 domain，不是非阻塞 dump；导出当前日志应使用
`hilog -x -v time`。

### 10.6 SCALE_TO_WINDOW 实机结果

2026-08-02 实机确认 `OH_SCALING_MODE_SCALE_TO_WINDOW_V2` 对当前画面问题完全无效。
因此停止继续修改 QWindowContainer、NativeNode geometry 和 NativeWindow scaling mode。
10.3--10.5 的日志仍然有价值：它们已经证明 Surface 容器尺寸正确，也把问题边界收敛到
OHOSAVCodec 直出缓冲与 Surface 的组合，但这条原生直出链路暂不再作为默认方案。

## 11. 替代方案：software-vmem 双后端版本

### 11.1 默认软件路径

新的默认路径完全绕开 OHNativeWindow 视频直出：

```text
VLC/FFmpeg 软件解码
  -> VLC vmem video output (RV32)
  -> VideoWidget 持有的 QImage
  -> QPainter 在 Qt 内容区内居中等比例绘制
```

`libvlc_new()` 只保留与后端无关的原有参数。`VideoWidget` 创建后在 VLC 根对象和 playlist
对象上设置以下运行期变量：

```text
vout=vmem
window=none
avcodec-hw=none
```

注意：不能把 LibVLC memory callback 内部设置的变量照搬成 `libvlc_new()` 命令行参数。
首包加入了 `--window=none`、`--vout=vmem` 和 `--avcodec-hw=none`，设备上的
`libvlc_new()` 直接返回空并使 Ability 主动退出。修正版从启动参数中移除了全部三个后端
参数，只在 VLC 对象树创建完成后设置运行期变量；其中 `window` 本来也不是已注册的命令行
配置项。

没有强制 `--avcodec-codec=h264`，因为该选项是全局 avcodec 选项，也可能影响由 avcodec
负责的音频流。`avcodec-hw=none` 与 LibVLC 官方 memory callback 路径保持一致，并允许
H.264、H.265 等软件解码器按媒体类型正常选择。

`VideoWidget` 在 VLC 根对象和 playlist 对象上同时注册 `vmem-setup`、`vmem-cleanup`、
`vmem-lock`、`vmem-unlock`、`vmem-display` 和 `vmem-data`，避免之前原生窗口变量遇到的
对象继承链差异。vmem 输出源分辨率的 `RV32` 帧；窗口 resize 不改变解码缓冲，只触发
Qt 用新的目标矩形重绘，因此画面位置和尺寸不再依赖 ArkUI Surface。

因为 vmem 不持有 Qt vout window，第一帧到达时由 `presentSoftwareFrame()` 主动把中央
`QStackedWidget` 切换到 `VideoWidget`，避免帧已解码但界面仍停留在背景页。

首版使用互斥锁保护 QImage，并在 GUI 线程用 `Qt::KeepAspectRatio` 绘制，未占用区域填黑。
这是正确性优先的实现，1080p 需要实机检查 CPU 占用和掉帧；4K、HDR 和高帧率不是首版
性能目标。后续可以把独立的软件帧后端替换成 NV12/I420 + OpenGL 纹理上传，减少 RV32
颜色转换和 QPainter 缩放成本。

### 11.2 保留硬件解码余地

原来的 `native-surface` 代码没有删除。启动进程前设置：

```text
VLC_OHOS_VIDEO_BACKEND=native-surface
```

即可恢复 `QWindowContainer -> OHNativeWindow -> OHOSAVCodec` 路径；环境变量为空或为
`software-vmem` 时走新的默认软件路径。后端判断集中在 `harmony/main.cpp` 和
`VideoWidget`，后续可以再增加 GPU texture/backend 枚举，而不必把硬解 Surface 逻辑
重新混入软件帧回调。

### 11.3 software-vmem 启动修正版

Debug 未签名包：

`harmony/build/harmony-package/entry/build/default/outputs/default/entry-default-unsigned.hap`

- 生成时间：2026-08-02 10:54:23
- 大小：228067376 bytes
- HAP SHA-256：
  `788511E6D5336BB04DE7399CEB53FD936BD916EB5137C3C3994B06A7271A72C7`
- 已检查 HAP 内 `libvlc-harmony-qt.so`，包含双后端选择字符串，且不包含三个后端命令行
  参数。
- 已检查 HAP 内 `libqt_plugin.so`，包含 `VLC_OHOS_VMEM` 回调日志标记和
  `presentSoftwareFrame`。
- 首包启动失败的 PID 级日志保存在
  `harmony/build/software-vmem-crash.log`；关键结论是
  `VLC_OHOS_VIDEO_BACKEND=software-vmem` 后紧接 `libvlc_new failed`。

首次部署先用原测试视频验证是否出现画面。若黑屏，不要再采集 geometry 日志，只导出：

```powershell
& $hdc shell hilog -x -v time |
    Select-String 'VLC_OHOS_VIDEO_BACKEND|VLC_OHOS_VMEM|vmem|avcodec|decoder|video output'
```

正常至少应看到 `VLC_OHOS_VMEM software video callbacks registered` 和
`VLC_OHOS_VMEM setup frame=<宽>x<高> pitch=<字节数>`。出现画面后再检查播放、暂停、
横向/竖向 resize、全屏、音频同步和 CPU 占用。

## 12. hardware-texture 后端与首次播放 warm-up

为兼容 OHOSAVCodec 硬件解码，同时绕开原生 Surface 无法正确跟随 Qt 窗口尺寸的问题，
默认后端现为：

```text
OHOSAVCodec
  -> OH_NativeImage / OHNativeWindow
  -> GL_TEXTURE_EXTERNAL_OES
  -> 嵌入 QWindowContainer 的 QOpenGLWindow
```

实现位于 `modules/gui/qt/components/ohos_video_texture_widget.*`。NativeImage 返回的矩阵
按 OpenGL column-major 数据直接上传，并使用 `OH_NativeImage_GetBufferMatrix()` 合并硬解
缓冲的 stride/crop。帧回调使用待消费计数而不是布尔值，避免多个 BufferQueue 通知合并后
没有释放全部输出缓冲而使播放卡死。视频原始宽高由 `MainInterface::getVideoSlot()` 在创建
NativeImage 前传给 `VideoWidget`，不再用窗口尺寸猜测首次缓冲大小。

### 12.1 已知问题：进程首次 codec session 发白/裁剪异常

实机结果：同一应用进程中，fresh NativeImage 上的第一次 OHOSAVCodec session 仍可能画面
发白并只显示局部；停止后在同一个 NativeImage/NativeWindow 上第二次打开相同文件，颜色和
裁剪均正常。方向和连续播放问题已经分别通过纹理坐标及逐帧消费修复。

当前 workaround 在第一次真正消费到纹理帧后，仅执行一次进程级
`playlist_ViewPlay(currentItem)`，让播放引擎对同一个 playlist item 重建 input/decoder。
不能使用 `playlist_Stop()` 后再裸调 `playlist_Play()`：OHOS 当前流程在 Stop 后会清空当前项，
Play 没有目标并直接回到未打开文件的页面。用户侧仍只需打开一次文件；后续播放不再重启。
设置以下环境变量可关闭该行为以便继续诊断：

```text
VLC_OHOS_TEXTURE_FIRST_OPEN_WARMUP=0
```

TODO：在可修改 OHOSAVCodec 适配层源码后，检查首次 `SetSurface` 前后的输出 format、色彩
空间/HDR metadata、crop 和 BufferQueue geometry 提交顺序，并在 codec/surface 边界完成
正确初始化后删除自动 stop/play workaround。

### 12.2 视频轻微领先音频

实机观察到多数视频画面比音频轻微领先。曾尝试在 NativeImage 消费端每次只取一帧并延迟
60 ms；该做法会填满硬解码器有限的输出 BufferQueue，实机直接卡住，已经撤回。纹理层必须
继续及时消费所有已通知缓冲，音画补偿放回 VLC input clock。

hardware-texture 后端现在默认使用 VLC 的 `audio-desync=-750`（单位毫秒），即让音频相对
提前 750 ms，不改变视频解码和呈现队列。可通过环境变量校准：

```text
VLC_OHOS_AUDIO_DESYNC_MS=-750
```

启动参数取值限制为 -2000--2000 ms；设为 0 可禁用补偿。负值让音频提前，正值让音频
延后。播放期间也可通过 `工具 -> 音轨同步 -> Audio track synchronization` 即时调整；
OHOS 版把用户值保存到 QSettings 的 `OHOS/audioDelayMs`，并在每个新 input 建立时重新
应用，因此不会在打开下一个视频后归零。界面单位为秒，默认显示为
`-0.750 s (Hastened)`。

### 12.3 简体中文界面

OHOS 使用的预编译 LibVLC 未启用 gettext/NLS，因此原来的 `vlc_gettext()` 总是返回英文，
即使仓库存在 `po/zh_CN.po` 也不会翻译。构建现将该 PO 转换为
`harmony/translations/vlc_zh_CN.qm`，通过 `harmony_translations.qrc` 内嵌到 Qt 插件；OHOS
下的 `qtr()` 和 UIC 的 `Q_()` 改由 `QTranslator` 查询空 context 的 VLC catalog。

启动时默认跟随 `QLocale::system()`；语言代码以 `zh` 开头时加载简体中文，否则回退英文。
若 Harmony 平台只能返回空值或 `C` locale，本移植版回退为 `zh_CN`，避免中文设备继续显示
全英文。
可设置 `VLC_OHOS_UI_LANGUAGE=zh_CN` 强制中文，或设置为 `en` 强制英文。当前只打包了简体
中文，后续增加其他语言时可用同样流程转换相应 `po/*.po` 并增加资源映射；尚未实现运行期
语言选择下拉框，切换语言需要重启应用。

### 12.4 hardware-texture 字幕叠加

`hardware-texture` 的视频帧是 `CDOH` opaque picture。OHOS surface display 模块没有提供
`subpicture_chromas`，也没有在 display 阶段合成 subpicture；Qt 的 OpenGL 窗口原先又只绘制
`GL_TEXTURE_EXTERNAL_OES` 视频纹理，因此选择字幕轨后虽然 SPU 在工作，字幕层并未进入最终
画面。

当前为该后端增加了 OHOS 专用 `text renderer` 子模块和第二层 RGBA OpenGL 纹理。文本字幕
先由 Qt 绘制到透明 `QImage`，再由 `OhosVideoTextureWindow` 按视频的 letterbox 区域、VLC
对齐方式和原始画面坐标进行定位，最后以 alpha blending 叠加到视频纹理上。实现文件为：

- `modules/gui/qt/components/ohos_subtitle_renderer.*`
- `modules/gui/qt/components/ohos_video_texture_widget.*`
- `modules/gui/qt/qt.cpp`

该路径解决 SRT 等 VLC `subsdec` 已能解析的纯文本字幕，不依赖 libass。ASS/SSA 的完整排版、
特效和字体附件仍需在 VLC SDK 中编译并打包 libass；位图字幕也不在这条文本渲染路径内。
`software-vmem` 和 `native-surface` 后端不会启用此 renderer，以免影响它们原有的字幕选择。

Debug HAP 已重新构建：

```text
harmony/build/harmony-package/entry/build/default/outputs/default/entry-default-unsigned.hap
SHA-256: 7F2B86D1BF79C9532C83B89B3952974DF6DCB2CA24C8B8FFAF204500F4A3A9E6
```

实机验证时选择 SRT 字幕轨，并从 hilog 检查以下标记：

```text
VLC_OHOS_SUBTITLE hardware texture renderer opened
VLC_OHOS_SUBTITLE frame
VLC_OHOS_SUBTITLE first overlay presented
```

前两项说明字幕已被解析并交给纹理窗口，最后一项说明 RGBA 字幕纹理已经参与 OpenGL 合成。

### 12.5 libass RGBA 字幕桥接 ABI

libass 在 VLC 3.0 中是 `spu decoder`，输出一个或多个 `VLC_CODEC_RGBA` region，并不经过
12.4 增加的 `text renderer`。为让 GUI 可以在不链接 libass、也不让 OHOS vout 链接 Qt 的
情况下接收这些 region，增加了公共 C ABI：

```text
include/vlc_ohos_subtitle_bridge.h
VLC variable: ohos-subtitle-bridge
ABI version: 1
```

`VideoWidget` 建立 hardware-texture surface 时，把
`vlc_ohos_subtitle_bridge_t` 的静态地址同时注册到 LibVLC 根对象和 playlist。回调可从任意
VLC vout 线程调用；`publish()` 在返回前复制所有像素，随后由 Qt GUI 线程上传纹理。当前约定：

- 一帧最多 64 个 region；
- region 仅接受 `VLC_CODEC_RGBA`、正 pitch；
- 坐标使用 libass/VLC 的原始画面坐标；
- Qt 对 region 做画面边界裁剪、SourceOver 合成和 subpicture 全局 alpha；
- `heartbeat()` 延长当前 bitmap 字幕，`clear()` 立即清除 bitmap 字幕；
- Text 与 Bitmap 使用独立 owner，vout 的 bitmap 清屏不会误删 SRT renderer 刚提交的字幕。

库侧 `modules/video_output/ohos/display.c` 已从 `vout_display_t` 调用
`var_InheritAddress(vd, VLC_OHOS_SUBTITLE_BRIDGE_VAR)` 获取该表，检查 `struct_size` 和
`abi_version` 后再调用。在 `Prepare()` 中把 `subpicture_t` 的 RGBA picture planes 填入
`vlc_ohos_subtitle_frame_t`；内容变化时调用 `publish()`，未变化时调用 `heartbeat()`，字幕
结束或切轨时调用 `clear()`。像素指针在 `publish()` 返回后即可失效。

当前 SDK 的 `libohos_surface_display_plugin.so` 已声明 `VLC_CODEC_RGBA` subpicture chroma 并包含
上述 bridge，SHA-256 为
`09ADDC340AEF3C330583B8E12B21D2735BB4C29FF461E94A9EA24B6416C41FA2`。

最终 Debug HAP 大小为 233907236 bytes；构建输出与 HAP staging 中未 strip 的 `libqt_plugin.so`
SHA-256 均为：

```text
EA189D0A2D874BC264CAD0A19B48E6ED9CC8CA7F46C9ED723A71142CA1753891
```

最终 HAP 会再次 strip 原生库，因此压缩包内 `libqt_plugin.so` 的 SHA-256 为
`8335C358EA5B136C0A192E50C25AFAB91271106B4ABDB2E78EED3244337A4480`，包内
`libohos_surface_display_plugin.so` 的 SHA-256 为
`2954ED3181495F0859AFD96AA388CFEF01AEE2FD58E2A66D1B63F6F2DF80FD6B`；两者仍包含下述
`VLC_OHOS_SUBTITLE` 诊断字符串。

2026-08-02 更新的 VLC 依赖已经重新打包。HAP 内已确认包含
`vlc/plugins/codec/liblibass_plugin.so`、`libass.so.9`、`libfontconfig.so.1`、
`libharfbuzz.so.0`、`libfribidi.so.0`、`liba52.so.0` 和 fontconfig 所需的
`libxml2.so`；staging 副本与新 SDK 源文件的 SHA-256 一致。

新版 `libass.so.9` 已将 FriBidi 等依赖静态链接进自身，仅保留 `libc.so` 动态依赖，且不再有
未解析的 `fribidi_*` 符号。此前 `harmony/main.cpp` 中无效的 `RTLD_GLOBAL` 预加载兼容代码
已经移除；源文件与 staging 中 `libass.so.9` 的 SHA-256 均为
`472579E664C26BB5D471483BAB359E6CBD10B8D0C2E8FDD6295DB4F0A6664ED1`。

OHOS 没有该静态 fontconfig 构建所期望的配置文件。为避免 ASS 轨可解码但
`ass_render_frame()` 因无可用字体而产生空图像，WSL VLC 的 `modules/codec/libass.c` 现将
`__OHOS__` 纳入 Android 系统字体分支，默认使用设备存在的
`/system/fonts/NotoSansCJK-Regular.ttc`。重新编译后的 `liblibass_plugin.so` 在 WSL 输出、
Windows SDK 和 HAP staging 中的 SHA-256 均为
`331B43AB083133E374B228911F15439F541650C6D199234323B296338F09ACE1`。

针对 ASS 轨道已能加入但仍无画面的情况，GUI 现在会在创建视频 Surface 前提前注册字幕桥；OHOS vout
也会在每次收到 subpicture 时重试取得 bridge，避免 vout 打开早于 Qt 注册所造成的永久空指针。两端新增
`VLC_OHOS_SUBTITLE` 诊断标记，可依次确认 bridge 注册、vout 获取、RGBA region、publish 和 OpenGL
overlay 呈现。最终 HAP SHA-256 为
`7F2B86D1BF79C9532C83B89B3952974DF6DCB2CA24C8B8FFAF204500F4A3A9E6`。

上述哈希只保留为当时排障记录，不再作为后续交付步骤。当前约定是重新构建后直接部署实测。

## 13. HarmonyOS 系统能力接入

### 13.1 文件 URI 持久化与激活

实现参考 `mpc-qt` 提交 `ce683eae4df1e46adea5bec53d09baa7fb6fcca3`，但适配到 VLC 3
现有的打开文件、播放列表和外挂字幕入口。核心文件为：

- `modules/gui/qt/components/ohos_file_permission.*`
- `modules/gui/qt/dialogs_provider.cpp`
- `modules/gui/qt/input_manager.cpp`
- `modules/gui/qt/main_interface.cpp`

文件选择器和 Want 提供的授权是临时的。URI 第一次进入 VLC 时调用
`OH_FileShare_PersistPermission()`，实际打开前调用 `OH_FileShare_ActivatePermission()`；新 input
建立时再激活一次，覆盖从播放列表、最近项目或恢复流程重新进入的情况。外挂字幕拖放和文件对话框
也走同一封装，不能只修主视频选择器。

FileShare 权限 API 与 VLC file access 对 URI 的要求不同：

- 权限管理使用带 authority 的 `file://docs/...`；`/Photos` 使用 `file://media/...`。
- VLC 播放时必须去掉 `docs`/`media` authority，恢复为真实本地 `file:///...` 路径。

因此 `OhosFilePermission::playbackUrl()`/`playbackMrl()` 与 persist/activate 必须成对保留。把
`file://docs/...` 原样交给 VLC 会造成“授权成功但文件仍打不开”的误判。

### 13.2 Want 文件关联

`harmony/patch-harmony-package.ps1` 在每次 `harmonydeployqt` 生成的新模板上补入媒体
`linkFeature: "FileOpen"`，覆盖 `video/*`、`audio/*`、`general.audio` 和通用文本类 URI。
不要直接编辑 `harmony/build/harmony-package` 中的 manifest，因为下一次部署会全部重建。

Qt 当前没有把 `Want.uri` 直接暴露给这版 VLC Qt GUI，因此使用一个小型跨层队列：

```text
QAbility.onCreate()/onNewWant()
  -> filesDir/vlc-open-wants.txt
  -> OhosWantFileBridge 每 250 ms 消费并清空
  -> persist + activate
  -> Open::openMRL()
```

这种方式同时处理冷启动 Want 和应用已运行时的 `onNewWant()`，并确保最终播放操作回到 Qt GUI
线程。日志标记为 `VLC_OHOS_WANT`。如果以后 Qt 官方提供稳定的 Want URI 原生接口，可替换文件
队列，但必须继续覆盖两个生命周期入口。

### 13.3 AVSession 与后台播放

核心文件为 `modules/gui/qt/components/ohos_avsession_bridge.*`，链接系统库 `ohavsession`。
当前创建 `SESSION_TYPE_AUDIO` 的 VLC 会话，并同步：

- 标题、资源 URI、时长；
- preparing/playing/paused/completed 状态；
- 每秒一次的播放位置；
- 播放、暂停、停止、上一项、下一项；
- 绝对 seek、快进和快退。

AVSession 回调不是 Qt GUI 线程，必须用 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`
回到对象线程后再操作 `MainInputManager`。析构时注销全部 callback、deactivate 并 destroy，避免
系统继续持有失效的 `this`。

打包脚本同时补入：

```text
backgroundModes: ["audioPlayback"]
ohos.permission.KEEP_BACKGROUND_RUNNING
```

以及所需的 BackgroundTasksKit/WantAgent ArkTS 注册。日志标记为 `VLC_OHOS_AVSESSION`。

### 13.4 应用名称与图标

生成包的 Ability label 和 AppScope `app_name` 都改为 `VLC`。Qt 模板的 `app_icon.png`、
`foreground.png`、`startIcon.png` 统一由仓库现有的 `share/vlc512x512.png` 覆盖。修改位于
`patch-harmony-package.ps1`，保证每次重新生成 Harmony 工程后仍然生效。

## 14. 字幕链路的最终结论

### 14.1 为什么“选择了字幕轨”不代表能看到字幕

在默认 `hardware-texture` 后端中，字幕至少经过四段：

```text
demux 识别轨道和 codec
  -> spu decoder（subsdec 或 libass）
  -> VLC subpicture/RGBA region
  -> OHOS bridge + Qt OpenGL overlay
```

轨道菜单能列出并选中，只能证明 demux 暴露了 ES；任何后续一段为空都会表现为“选择后没反应”。
排障时必须分别确认 codec、decoder、region、bridge publish 和 overlay present，不能直接归因于 GUI
菜单或 libass。

### 14.2 SRT 文本字幕

OHOS surface display 不会替 Qt 的 `GL_TEXTURE_EXTERNAL_OES` 视频合成 subpicture，所以最早曾出现
SRT 已解码但画面无字。现在使用 `ohos_subtitle_renderer.*` 把 VLC 文本 region 绘制为透明
`QImage`，再由 `OhosVideoTextureWindow` 上传第二层 RGBA texture 做 alpha blend。字幕坐标必须按
视频 letterbox 矩形换算，不能按整个 Qt 窗口直接定位。

### 14.3 外挂 ASS 与 libass 依赖

外挂 ASS 需要 `liblibass_plugin.so` 和 `libass.so.9`。最终库形态是：

- `libass.so.9` 仍为动态库；
- FriBidi 等 libass 依赖被静态链接进 `libass.so.9`；
- GUI/HAP 继续打包 libass 插件和该动态 libass。

早期日志：

```text
relocating failed: symbol not found ... libass.so.9 ... fribidi_get_bidi_types
```

即使先 `RTLD_GLOBAL` 预加载 `libfribidi.so.0` 仍失败，说明问题在实际 DSO 符号/命名空间或版本
组合，不是单纯的加载先后顺序。把依赖静态并入 libass 后消除了这类运行时重定位风险。以后描述
这一方案时不要简写成“静态编译了 libass”。

OHOS 又没有该构建所预期的 fontconfig 配置，因此 `modules/codec/libass.c` 将 `__OHOS__` 纳入
Android 字体选择分支，默认使用 `/system/fonts/NotoSansCJK-Regular.ttc`。否则 decoder 可以打开，
`ass_render_frame()` 仍可能因找不到字体而返回空图像。

libass 输出的是 `VLC_CODEC_RGBA` region，不经过文本 renderer。公共 C ABI 定义在
`include/vlc_ohos_subtitle_bridge.h`；GUI 在 Surface/vout 建立前把 bridge 同时注册到 LibVLC 根对象
和 playlist，OHOS display 收到 subpicture 时也会重试获取，避免初始化时序造成永久空指针。

### 14.4 MKV 内嵌 ASS：两个连续根因

外挂 ASS 可显示但内嵌 ASS 仍不显示时，关键是两者的 demux 路径不同：外挂文件走 VLC subtitle
demux，MKV 在当前包中走 FFmpeg 6 avformat，因为 SDK 没有打入 VLC 原生 Matroska demux 插件。

第一处根因是 VLC 3 的 `modules/codec/avcodec/fourcc.c` 只映射：

```c
{ VLC_CODEC_SSA, AV_CODEC_ID_SSA }
```

现代 FFmpeg 对 Matroska `S_TEXT/ASS` 返回 `AV_CODEC_ID_ASS`，结果 VLC fourcc 为 0。轨道仍会列出，
选择后却打印 `main decoder: could not identify codec`。最终增加：

```c
{ VLC_CODEC_SSA, AV_CODEC_ID_ASS }
```

第二处是一次失败修正带来的教训：不能把 `AV_CODEC_ID_ASS` 加进 avformat 的
`BuildSsaFrame()` 分支。该函数只接受带起止时间的 `Dialogue: ...` 行，再转换成 libass 的 chunk；
而 Matroska 内嵌 ASS packet 已经是下面这种 libass 原生 chunk：

```text
ReadOrder,Layer,Style,Name,MarginL,MarginR,MarginV,Effect,Text
```

送进 `BuildSsaFrame()` 会因为不以 `Dialogue:` 开头而返回 `NULL`，每个字幕包随即被静默丢弃。
最终正确状态是：

- `fourcc.c` 映射 `AV_CODEC_ID_ASS -> VLC_CODEC_SSA`；
- `modules/demux/avformat/demux.c` 的 `BuildSsaFrame()` 条件仍然只处理
  `AV_CODEC_ID_SSA`；
- `AV_CODEC_ID_ASS` 走普通 block copy，保留 Matroska chunk 原文交给 libass。

该组合已由用户实机确认内嵌 ASS 正常显示。以后升级 FFmpeg 或 VLC 时，应优先核对 codec ID 和
packet 格式，而不是因为显示层已经能显示外挂 ASS 就继续修改 OpenGL overlay。

### 14.5 字幕延迟

VLC 原 Qt 界面已经提供 `工具 -> 轨道同步`，字幕项直接读写 input 的 `spu-delay`。正值表示字幕
延后，负值表示字幕提前，媒体切换时 VLC 会按自身规则重置。不再添加重复的 OHOS 专用控件。

## 15. 构建、打包与版本库边界

完整构建命令：

```powershell
cd E:\source\vlcplayer\harmony
.\build-harmonyos.ps1 -Configuration Debug -Bundle
```

构建脚本的重要顺序：

1. 显式构建与安装版 Qt 精确匹配的 `QOhosPlatformIntegrationPlugin`；
2. 构建 VLC Qt 插件和 launcher；
3. 运行 `harmonydeployqt` 生成临时 Harmony 工程；
4. 运行 `patch-harmony-package.ps1` 补 manifest、Want、图标、VLC plugins 和全部版本化 `.so`；
5. 再运行 Hvigor 生成未签名 HAP。

库侧修改在 WSL VLC 树中交叉编译。字幕内嵌修复至少需要重建：

```bash
make -C modules -j4 libavformat_plugin.la libavcodec_plugin.la
```

然后把对应 `.so` 复制到 Windows VLC SDK 的正确插件子目录，再运行完整 HAP 打包。只改本仓库
源码、只改 WSL 源码或只重编插件但未更新 Windows SDK，都会产生“源码正确、设备仍是旧行为”的
假象。

### 15.1 WSL VLC 库侧补丁

WSL 编译树中的必要源码修改已导出为：

```text
harmony/ohos_vlc-ohos-3.0.21-vlc-qt-integration.patch
```

该补丁明确以 [CPF-ApplicationTPC/ohos_vlc](https://gitcode.com/CPF-ApplicationTPC/ohos_vlc)
的 `ohos-3.0.21` 分支为上游，基线提交为
`8138861d174e1a725f00e4ce41fbc0a01a3d4d12`。补丁只收录以下五个源文件，不包含 WSL 中的
`arm64-v8a-build`、`arm64-v8a-contrib` 等生成目录：

- `include/vlc_ohos_subtitle_bridge.h`；
- `lib/Makefile.am`；
- `modules/codec/avcodec/fourcc.c`；
- `modules/codec/libass.c`；
- `modules/video_output/ohos/display.c`。

在该上游分支的干净工作树中应用：

```bash
git apply --check /path/to/ohos_vlc-ohos-3.0.21-vlc-qt-integration.patch
git apply /path/to/ohos_vlc-ohos-3.0.21-vlc-qt-integration.patch
```

其中 `display.c` 的上游内容使用 CRLF，而当前修改段使用 LF。为了让普通 `git apply` 无需附加
空白字符选项即可精确应用，该文件在补丁中由 Git 编码为 binary patch；这只是行尾兼容处理，内容
仍是 C 源码。其余四个文件保持普通文本 diff，便于审阅。

版本库只保存自有源码、脚本、资源描述和本文。以下内容不提交：

- `harmony/build/` 和 CMake/Ninja/Hvigor 中间文件；
- `harmony/*.hap`；
- `harmony/qtbase-6.12.0/`、`harmony/qtbase-sdk-src/`；
- `harmony/node-addon-api/`；
- 生成的 `.qm`、复制的静态/动态库和版本化 `.so.*`。

`.gitignore` 已为这些路径增加明确规则。Qt 平台补丁需要长期保存时，应导出为小型补丁或独立
仓库提交，而不是把整份 QtBase 复制进本仓库。

## 16. 可复用的排障原则

1. 先画清数据路径。相同的“黑屏”或“无字幕”可能分别发生在 demux、decoder、Surface、纹理、
   subpicture 或 Qt overlay，不要看到 UI 操作成功就跳过中间层。
2. 外挂字幕与容器内嵌字幕不是同一路径。外挂正常只能证明 libass 和最终显示层大致可用，不能证明
   Matroska/avformat 的 codec 映射和 packet 转换正确。
3. 动态库“文件存在”不等于可加载。要看重定位符号、依赖版本和加载命名空间；预加载成功日志也不
   等于依赖符号真的可解析。
4. 注册 bridge、Surface 或权限时要考虑对象继承和初始化时序。关键地址同时放到 LibVLC 根对象与
   playlist，并允许消费侧重试，比只在某一时刻单次查询可靠。
5. Qt HarmonyOS 私有插件必须与安装版源码提交和 node-addon-api ABI 精确匹配。版本号相同但提交不同
   也可能启动即重定位失败。
6. 不修改生成目录作为最终方案。所有 manifest/ArkTS 模板改动必须通过可重复执行的后处理脚本注入。
7. `hilog` 可能被硬解错误刷爆并出现 `write socket failed ... dropped`。需要提前使用统一、低频标记，
   清空日志后复现一次，并避免在逐帧热路径打印大量信息。
8. 验证优先直接部署。除非定位“是否打入旧文件”且没有更直接证据，否则不再重复做哈希计算；使用
   构建成功、文件大小/时间、staging 路径和实机行为闭环即可。

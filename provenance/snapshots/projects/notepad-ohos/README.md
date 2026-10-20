# notepad--_ohos

将Qt Widgets 的开源著名文本编辑器项目 **NotePad--**（常称 Ndd，国产替代NotePad++），从传统桌面形态，迁移为可在 **鸿蒙 PC / 平板** 上安装的 HAP 应用。

原理为将原生编辑器的QT代码，交叉编译为 `libNotePad--.so`，由 DevEco Studio 中的 **ArkTS + Qt 嵌入式模板** 加载并打包为 HAP，可在鸿蒙 **平板 / 2in1（PC 形态）** 等设备上运行。

**NotePad--** 的原始项目地址：[https://gitee.com/cxasm/notepad--](https://gitee.com/cxasm/notepad--)

## 功能与定位

- 复用 `Doc/src_qt_prj/notepad---main` 中的 Qt Widgets 业务代码（含 QScintilla 等依赖子工程,移植需要不同于原始仓，有改动）。
- 鸿蒙侧负责应用生命周期、窗口与 Qt 运行时接入；业务库名在 ArkTS 常量中与 CMake 产物对齐。

## 工程结构

| 路径 | 说明 |
|------|------|
| `entry/` | 主模块：ArkTS 入口、`QtAppConstants.ets`、原生 CMake（`src/main/cpp`）等 |
| `qEmbeddedUiExtensionHost/` | Qt 嵌入式 UI 扩展宿主模块 |
| `Doc/src_qt_prj/notepad---main/` | NotePad-- Qt 源码与顶层 `CMakeLists.txt`（含 `OHOS` 下生成 `SHARED` 库的逻辑） |
| `Doc/NotePad--鸿蒙PC移植实践.md` | 移植思路、CMake 改动与交叉编译命令说明 |


## 移植说明（T0 / T1 / T2 分级）

本仓库按 **T0→T1→T2** 三级递进完成 NotePad-- 从桌面 Qt 应用到鸿蒙 HAP 的移植，当前进展如下：

### T0 — 基础框架迁移（已完成 ✅）

> 应用可在鸿蒙设备上正常启动、显示主界面，生命周期与 Qt 运行时接通。

| 功能项 | 状态 | 说明 |
|--------|------|------|
| ArkTS + Qt 嵌入式工程集成 | ✅ 已完成 | 基于 Qt for OHOS 模板，ArkTS 侧加载 `libNotePad--.so` |
| CMake OHOS 交叉编译改造 | ✅ 已完成 | `add_library(... SHARED ...)` 产出动态库，与 ArkTS 常量库名对齐 |
| 应用生命周期（Ability 创建/销毁/前后台） | ✅ 已完成 | `QAbility.ets` 中完整对接 OHOS 生命周期回调 |
| 窗口创建与显示 | ✅ 已完成 | 支持 `tablet` 与 `2in1`（PC 形态）设备类型 |
| 菜单栏、主编辑区渲染 | ✅ 已完成 | Qt Widgets 界面通过 QPA 插件正常绘制 |
| Qt 运行时依赖部署 | ✅ 已完成 | `libqohos.so` 及各 `libQt5*.so` 随 HAP 打包 |

### T1 — 核心编辑功能迁移（大部分已完成 ✅）

> 编辑器的核心文本功能可正常使用，满足日常文本编辑与代码查看需求。

| 功能项 | 状态 | 说明 |
|--------|------|------|
| 文本打开与编辑 | ✅ 正常 | 支持打开本地文件并进行编辑 |
| 文件保存（含另存为） | ✅ 正常 | Save / Save As 功能正常 |
| 新建文件 | ✅ 正常 | 多标签页新建与关闭 |
| 语法高亮（QScintilla） | ✅ 正常 | 基于 QScintilla 的代码高亮与词法分析 |
| 查找与替换 | ✅ 正常 | 查找窗口、批量查找替换等功能可用 |
| 撤销 / 重做 | ✅ 正常 | 编辑历史撤销与恢复 |
| 拖拽打开文件 | ✅ 正常 | 支持文件拖拽到编辑器打开 |
| 文件编码识别与转换 | ✅ 正常 | 编码检测与转换功能可用 |
| 多标签页管理 | ✅ 正常 | 标签页新建、关闭、拖拽排序 |
| 代码折叠 | ✅ 正常 | 代码块的折叠与展开 |
| 行号显示 | ✅ 正常 | 编辑区行号显示 |
| 状态栏信息 | ✅ 正常 | 行列号、文件编码等状态信息显示 |

### T2 — 完整功能体验迁移（进行中 🚧）

> 补齐外部系统交互与界面适配，达到与桌面版一致的使用体验。

| 功能项 | 状态 | 说明 |
|--------|------|------|
| 外部剪贴板粘贴 | ⚠️ 待修复 | 从系统外部拷贝的内容**无法粘贴**到本应用（已知问题） |
| 剪贴板复制（应用内 → 外部） | ✅ 正常 | 应用内文本可复制并粘贴到外部 |
| 菜单栏字体 / 界面显示适配 | ⚠️ 待优化 | 菜单栏字体显示偏大，与 Windows 桌面版对比有失真感 |
| 文件拖入（外部文件管理器→应用） | ✅ 正常 | 通过鸿蒙文件管理器拖入文件可正常打开 |
| 文件关联打开 | ✅ 正常 | 在文件管理器中关联打开文本文件、图片文件 |
| 插件系统 | ⏸️ 暂未验证 | 原生插件加载机制需在 OHOS 环境下验证 |
| 打印支持 | ⏸️ 暂未验证 | Qt PrintSupport 模块需在 OHOS 环境下验证 |
| 网络功能（远程文件等） | ⏸️ 暂未验证 | Qt Network 模块需在 OHOS 环境下验证 |

### 已知问题汇总

1. **外部粘贴不可用**：从系统剪贴板中（其他应用复制的内容）粘贴到 NotePad-- 时无响应，应用内复制/剪切的内容可以正常粘贴。初步判断为 Qt on OHOS 的剪贴板桥接尚未完全打通。
2. **菜单栏字体偏大**：界面菜单栏字体在鸿蒙 PC 上显示比 Windows 桌面版明显偏大，导致菜单项布局略显失真。可能与 Qt 的 DPI 缩放或 OHOS 下的字体回退策略有关。
3. **其他功能**：正常文本编辑、保存、代码高亮、多标签页管理等核心体验均与桌面版一致，无显著性能问题。

---

## 环境要求

- **DevEco Studio** 与 **HarmonyOS / OpenHarmony SDK**（含 native 工具链、`ohos.toolchain.cmake`）。
- **为鸿蒙目标构建的 Qt 5.12**（例如文档中常用的 `qt-5.12.12-ohos`），与你要运行的设备 **ABI** 一致（常见真机为 `arm64-v8a`）。
- 本工程 `build-profile.json5` 中产品示例：`compatibleSdkVersion` 5.0.0(12)、`targetSdkVersion` 6.0.0(20)，请按本机 SDK 与目标设备调整。

## 编译 Qt 侧 `libNotePad--.so`

**环境准备**

参考文章:[HarmonyOS 鸿蒙PC端 Qt 应用开发：第三方 Qt 开源软件移植指南
](https://blog.csdn.net/qq8864/article/details/160032290)

在 `Doc/src_qt_prj/notepad---main` 外单独建 `build` 目录，使用 SDK 自带的 CMake，并指定 Qt OHOS 路径与 `OHOS_ARCH`。具体命令、批处理示例与 CMake 条件说明见：

**[Doc/NotePad--鸿蒙PC移植实践.md](Doc/NotePad--鸿蒙PC移植实践.md)**

仓库内亦提供参考脚本：`Doc/src_qt_prj/notepad---main/build/build_ohos.bat`（需按本机修改 `SDK_PATH`、`QT_ROOT` 等变量）。

## 编译与运行鸿蒙应用（HAP）

1. 用 **DevEco Studio** 打开本仓库根目录。
2. 将交叉编译得到的 **`libNotePad--.so`** 及 Qt on OHOS 所需依赖库，按模板要求放入 **对应 ABI** 的 native 库目录（与 `entry/build-profile.json5` 中的 `abiFilters` 一致）。
3. 核对 **`entry/src/main/ets/common/QtAppConstants.ets`** 中的库名：

   ```typescript
   export const APP_LIBRARY_NAME = 'libNotePad--.so';
   ```

   须与实际文件名一致（含 `lib` 前缀与 `.so` 后缀）。

4. 在 IDE 中配置 **签名**（勿将个人证书路径提交到公共仓库），执行 **Build → Build HAP(s)**，安装到真机或模拟器。

`entry/src/main/module.json5` 中当前支持的 `deviceTypes` 包含 `tablet`、`2in1`；若需扩展机型，请阅读模块内注释（例如与最小化恢复相关的已知问题）。

## 相关资源

- 移植实践长文：[Doc/NotePad--鸿蒙PC移植实践.md](Doc/NotePad--鸿蒙PC移植实践.md)
- 上游 **NotePad--** 开源项目请参见其官方仓库与许可证说明。

## 许可证

请遵循 **NotePad-- 上游**、**Qt**、**HarmonyOS 工程模板** 及本仓库内各子模块所附的许可条款。

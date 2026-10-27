# qterminal for HarmonyOS (鸿蒙)

[qterminal](https://github.com/lxqt/qterminal) 的 HarmonyOS (鸿蒙) 移植版——LXQt 轻量级 Qt 终端模拟器。

> 原始 README 见 [README_original.md](README_original.md)

## 移植内容

| 项 | 状态 | 说明 |
|---|---|---|
| 终端 PTY (forkpty/termios) | ✅ 已验证 | posix_openpt/forkpty/termios 全链路真机验证通过 |
| Shell 交互 | ✅ | fork+exec /bin/sh 在沙箱内可用 |
| 标签页管理 | ✅ | 新建/关闭/切换标签 |
| 复制/粘贴 | ✅ | Ctrl+Shift+C / Ctrl+Shift+V |
| 字体缩放 | ✅ | Ctrl++ / Ctrl+- / Ctrl+0 |
| 全屏切换 | ✅ | F11 (setWindowState 实现) |
| 偏好设置 | ✅ | PropertiesDialog (有 parent) |
| 设置持久化 | ✅ | QSettings → AppLocalDataLocation |
| 等宽字体 | ✅ | 内嵌 DejaVu Sans Mono (QTBUG-146623 规避) |
| 关闭事件 | ✅ | OHOS 不弹确认框 (L1 铁律) |
| 键盘输入 | ✅ | IMF 拦截已规避 (WA_InputMethodEnabled=false) |
| DBus 多实例 | ❌ 跳过 | OHOS 无 D-Bus (have_qdbus 条件守卫) |
| 全局快捷键 | ❌ 跳过 | Qxt X11 后端不可用 |
| 下拉终端 | ❌ 跳过 | LayerShellQt (Wayland) 不可用 |

## 前置条件

| 项 | 要求 |
|---|---|
| Qt 6.12.0+ OHOS SDK | [下载预构建](https://gitcode.com/ohos-qt/qt-harmonyos-src/releases) (harmonyos_arm64_v8a) |
| DevEco Studio | [下载](https://developer.huawei.com/consumer/cn/deveco-studio/) (API 12+) |
| qtermwidget6 (OHOS) | 需先编译安装到 Qt OHOS SDK (见下方) |
| HarmonyOS 真机/模拟器 | arm64-v8a |

## 快速开始

### 1. 编译依赖 (首次)

```bash
# 编译 qtermwidget6 for OHOS 并安装到 Qt SDK
git clone https://github.com/lxqt/qtermwidget.git
cd qtermwidget

<QtOHosSdk>/bin/qt-cmake.bat -B build -S . -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="<QtOHosSdk>/lib/cmake/Qt6/qt.toolchain.cmake" \
  -DQT_HOST_PATH="<QtHostPath>" \
  -DQT_CHAINLOAD_TOOLCHAIN_FILE="<DevEcoSdk>/native/build/cmake/ohos.toolchain.cmake" \
  -DOHOS_ARCH=arm64-v8a \
  -DOHOS_SDK_NATIVE="<DevEcoSdk>/native" \
  -DCMAKE_INSTALL_PREFIX="<QtOHosSdk>"

cmake --build build --parallel
cmake --install build
```

### 2. 部署 Qt 运行时 .so

```bash
# 复制 Qt6 运行时到 libs/
mkdir -p HarmonyOS/entry/libs/arm64-v8a/styles
cp <QtOHosSdk>/lib/libQt6*.so HarmonyOS/entry/libs/arm64-v8a/
cp <QtOHosSdk>/plugins/platforms/libqohos.so HarmonyOS/entry/libs/arm64-v8a/
cp <QtOHosSdk>/plugins/styles/libqohosstyle.so HarmonyOS/entry/libs/arm64-v8a/styles/
# fontconfig 及其依赖 (从 additional-packages)
cp <AdditionalPackages>/lib/libfontconfig.so HarmonyOS/entry/libs/arm64-v8a/
cp <AdditionalPackages>/lib/libfreetype.so HarmonyOS/entry/libs/arm64-v8a/
cp <AdditionalPackages>/lib/libexpat.so HarmonyOS/entry/libs/arm64-v8a/
# ... 其他 additional-packages 依赖
```

### 3. 运行 setup 脚本

```powershell
# Windows
.\setup.ps1 -QtOhosSdk "D:/Qt/6.12.0/harmonyos_arm64_v8a" `
            -QtHostPath "D:/Qt/6.12.0/mingw_64" `
            -DevEcoPath "D:/Application/DevEcoStudio"
```

```bash
# Linux/macOS
./setup.sh /path/to/qt-ohos-sdk /path/to/qt-host /path/to/deveco
```

### 4. DevEco Studio 构建

1. DevEco Studio → **File → Open** → 选择 `HarmonyOS/` 目录
2. **File → Project Structure → Signing Configs** → 勾选 **Automatically generate signature**
3. 点 **Run** (▶)

## 工程结构

```
qterminal/
├── CMakeLists.txt              ← 已加 OHOS 守卫 (跨平台兼容)
├── src/                         ← 原始源码 + OHOS 适配
│   ├── fonts/DejaVuSansMono.ttf ← 内嵌等宽字体
│   └── third-party/qxtglobalshortcut_ohos.cpp ← Qxt OHOS stub
├── HarmonyOS/                   ← 鸿蒙胶水代码
│   ├── AppScope/app.json5      ← bundleName
│   ├── entry/
│   │   ├── build-profile.json5 ← CMake 交叉编译配置 (模板化路径)
│   │   ├── src/main/ets/       ← QAbility/QAbilityStage/OhosExportModules
│   │   └── libs/arm64-v8a/     ← Qt 运行时 .so (setup 后填充)
│   └── qEmbeddedUiExtensionHost/
├── setup.ps1 / setup.sh        ← 一键配置脚本
└── README_original.md           ← 上游原始 README
```

## OHOS 适配修改清单

### CMakeLists.txt

| 修改 | 说明 |
|---|---|
| `CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH` | 交叉编译 find_package 修复 |
| LayerShellQt → optional | OHOS 守卫，放弃下拉终端 |
| DBus → `if(UNIX AND NOT OHOS)` | 跳过 Qt6DBus |
| X11 → `elseif(UNIX AND NOT OHOS)` | 跳过 X11 |
| Qxt → OHOS stub | 全局快捷键 no-op |
| `add_library(SHARED)` | OHOS 构建为 .so (QPA dlsym("main")) |
| post-build deploy | 自动复制 libqterminal.so 到 libs/ |

### 源码修改 (Q_OS_OHOS 守卫)

| 文件 | 修改 |
|---|---|
| `src/config.h` | DEFAULT_FONT → "DejaVu Sans Mono" |
| `src/main.cpp` | main 加 visibility("default") + QFontDatabase 注册 |
| `src/mainwindow.cpp` | LayerShellQt 6 处守卫 + closeEvent 不弹框 + showEvent 延迟 focus |
| `src/propertiesdialog.cpp` | LayerShellQt 2 处守卫 |
| `src/termwidget.cpp` | WA_InputMethodEnabled=false (规避 IMF 键盘拦截) |
| `src/termwidget.h` | setSize 移出 HAVE_QDBUS |
| `src/qterminalapp.h` | isPrimaryInstance/requestDropDown 移出 HAVE_QDBUS |

## 许可

GPLv2+ (继承上游 qterminal)

## 相关链接

- [上游 qterminal](https://github.com/lxqt/qterminal)
- [qtermwidget](https://github.com/lxqt/qtermwidget)
- [Qt for HarmonyOS](https://gitcode.com/ohos-qt/qt-harmonyos-src)
- [Qt for HarmonyOS Wiki](https://wiki.qt.io/Qt_for_HarmonyOS)

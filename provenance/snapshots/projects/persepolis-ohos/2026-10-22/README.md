# Persepolis Download Manager for HarmonyOS

Persepolis（下载管理器，基于 Python/PyQt5）的 HarmonyOS（鸿蒙）移植版。

![Persepolis on HarmonyOS](doc/persepolis.jpg)

## 概述

本仓库包含将 [persepolisdm/persepolis](https://github.com/persepolisdm/persepolis) 移植到 HarmonyOS 平台所需的全部胶水工程：

| 目录 | 内容 |
|------|------|
| `HarmonyOS/` | DevEco Studio 鸿蒙工程（QAbility + NAPI 启动器 + 资源） |
| `patches/` | 对上游 persepolis Python 源码的 OHOS 适配补丁（按文件整体替换） |
| `scripts/` | 依赖获取工具（Harmonybrew bottle 拉取 + ffmpeg 部署/裁剪） |
| `setup-libs.ps1` | 一键组装运行时依赖到工程目录 |

上游 Python 源码和运行时二进制依赖（Qt / CPython / PyQt5 / ffmpeg 等）**不入库**，由脚本按需拉取组装。

## 架构

```
ArkTS QAbility (HarmonyOS Stage 模型)
    └── libqohos.so (Qt for OHOS QPA 插件，兼 NAPI 模块)
          └── pyqtlauncher.cpp (C++ main)
                └── Py_Initialize() → PyRun_FileExFlags → launcher.py
                      └── PyQt5 QApplication → persepolis MainWindow
```

- **Python 入口**：`HarmonyOS/entry/src/main/resources/resfile/persepolis/launcher.py`
  包含 OHOS 适配层：子窗口 adoption（SubWindow 路由）、本地中文翻译表、ffmpeg
  loader 路由、平台分支短路等——全部 monkey-patch 形式，不侵入上游源码。
- **补丁机制**：`patches/*.py` 覆盖 `resfile/persepolis/persepolis/scripts/` 下的
  同名文件（文件级替换），由 `setup-libs.ps1` 在组装时自动应用。

## 构建步骤

### 前置依赖

1. **DevEco Studio 5.0+**（含 HarmonyOS SDK，apiType=stage）
2. **Qt 5.15.16 for HarmonyOS SDK**（arm64-v8a）
   - 下载：https://gitcode.com/ohos-qt/qt-harmonyos-src/releases
3. **DevPython-OH**（CPython 3.12 交叉编译 wheel）
   - https://gitcode.com/OpenHarmonyPCDeveloper/DevPython-OH
4. **PyQt5 for OHOS**（纯 Python 部分 + 交叉编译 .so）
   - 参考 git-cola HarmonyOS 移植的 site-packages/PyQt5
   - `.so` 模块需链接 libpython（`--qmake-setting "LIBS += -l:libpython3.12.so.1.0"`）
5. **上游 persepolis 源码**
   - https://github.com/persepolisdm/persepolis

### 组装依赖

编辑 `setup-libs.ps1` 顶部的路径变量，然后：

```powershell
powershell -ExecutionPolicy Bypass -File setup-libs.ps1
```

（可选）获取 ffmpeg（Video Finder 音视频合并）：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\fetch-ohos-bottle.ps1 -Formula ffmpeg
# 然后重跑 setup-libs.ps1
```

### 构建 HAP

```powershell
cd HarmonyOS
devecocli build
# 或用 DevEco Studio 打开 HarmonyOS/ 目录直接构建
```

产物：`HarmonyOS/entry/build/default/outputs/default/entry-default-signed.hap`

### 部署运行

```powershell
hdc install -r entry-default-signed.hap
hdc shell aa start -a QAbility -b com.persepolis.ohos
```

> **UI 自动化测试**：启动时追加 `--pb io.qt.experimental.enableA11ySupport true`
> 参数，即可通过 `hdc shell uitest dumpLayout` 获取完整 Qt 控件树。

## OHOS 适配说明

| 特性 | 处理方式 |
|------|----------|
| 子窗口 | QMenuBar/QToolBar 原生渲染；子窗口 adoption→SubWindow |
| 中文 | 本地翻译表补齐上游 .ts 缺失条目（37+28 条） |
| ffmpeg | Harmonybrew bottle 内置；无 exec 位 → loader 路由 |
| 剪贴板 | READ_PASTEBOARD 权限运行时申请（QAbility.ets） |
| 打开文件夹 | QDesktopServices → 系统文件管理器 |
| 托盘图标 | QOhosSystemTrayIcon（状态栏图标+右键菜单） |
| 通知音效 | 已裁剪（QtMultimedia 不在 OHOS SDK） |
| 开机自启/关机 | 已裁剪（桌面专属功能） |
| 浏览器集成 | 已裁剪 |

已知限制与详细问题记录见 `doc/problems.md`（内部交付文档）。

## License

- persepolis 上游：GPL-3.0
- 本仓库胶水代码：GPL-3.0
- ffmpeg 9.0.2：GPL-3.0（Harmonybrew 构建）

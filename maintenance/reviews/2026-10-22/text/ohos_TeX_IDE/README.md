# TeX IDE (based on TeXstudio) - 适用于 OpenHarmony 的 LaTeX 集成开发环境

[![Version](https://img.shields.io/badge/version-v1.0.0-blue.svg)](https://gitcode.com/OpenHarmonyPCDeveloper/ohos_TeX_IDE)
[![OpenHarmony Compatibility](https://img.shields.io/badge/OpenHarmony-5.0+-blue?logo=OpenHarmony)](https://www.openharmony.cn/)
[![License](https://img.shields.io/badge/license-GPL--3.0-green.svg)](LICENSE)

**生产力工具** | **专为 OpenHarmony PC / 2in1 设备设计**

> 📅 最后更新：2026/09/22

## 📖 简介

TeX IDE (based on TeXstudio) 是基于开源项目 [TeXstudio](https://github.com/texstudio-org/texstudio) 在 HarmonyOS NEXT 平台移植的 LaTeX 集成开发环境，提供源码编辑、命令补全、一键编译与 PDF 预览等完整写作工具链。MiKTeX 是主流的 TeX 排版发行版，提供 pdflatex、xelatex、lualatex 等编译引擎与海量宏包。本应用将二者移植到 HarmonyOS PC / 2in1，让你在鸿蒙电脑上完成从编写 .tex 源码到生成 PDF 的全流程。

## ✨ 核心特性

### 📝 编辑体验
- **完整的 TeXstudio 编辑器**：语法高亮、代码折叠、多光标、结构大纲
- **命令补全**：内置海量 LaTeX 命令与宏包补全定义（.cwl）
- **拼写检查**：内置 Hunspell 英语词典
- **中文写作**：支持 ctex 等中文排版，内置 Noto CJK / 文鼎字体

### ⚙️ 编译与预览
- **内置 MiKTeX 编译链**：pdflatex / xelatex / lualatex / bibtex / biber 等，HNP 包随应用分发，无需联网安装
- **一键编译**：编译、查看一气呵成
- **内置 PDF 预览**：基于 Poppler，支持 SyncTeX 正反向定位
- **Ghostscript 支持**：内置 gs 运行时，PS/EPS 处理无忧

### 💻 鸿蒙形态适配
- 折叠屏 / 半屏 / 多窗口自适应布局
- 跟随系统深浅色主题
- ArkTS HAP 壳 + C++ 引擎分层架构，原生运行

### 🔧 版本管理
- 内置 OpenSSH 工具链（ssh / scp / sftp / ssh-agent），支持 Git 远程仓库操作

## 🚀 快速开始

### 系统要求
- HarmonyOS 5.0 或更高版本的 PC / 2in1 设备

### 安装应用
1. 下载最新版本的 TeX IDE 安装包（.app / .hap）
2. 在鸿蒙设备上安装应用
3. 启动应用，首次运行自动初始化 MiKTeX 环境

### 从源码构建

本仓为集合仓，包含三个组件目录（单向依赖，Layer 3 → 2 → 1）：

| 目录 | 层级 | 说明 |
|------|------|------|
| `texstudio-hap/` | Layer 3 | ArkTS 应用壳与 HAP 打包脚本 |
| `texstudio/` | Layer 2 | TeXstudio 编辑器（构建产出 `libtexstudio.so`） |
| `miktex/` | Layer 1 | MiKTeX 编译器套件（构建产出 `miktex.hnp`） |
| `toolchain/qt-sdk/` | — | 预编译 Qt 5.15.12 OHOS arm64 SDK（含本项目全部 QPA 修复，免编译） |

默认工作分支为 `develop/main`。

```bash
git clone -b develop/main https://gitcode.com/OpenHarmonyPCDeveloper/ohos_TeX_IDE.git
```

构建与安装：**[BUILD.md](BUILD.md)**（编译手册，按步骤执行即可打出 HAP 包装机运行）。

## 🏗️ 架构设计

TeX IDE 采用分层架构设计：

- **应用壳（texstudio-hap/）**：ArkTS 编写的 HarmonyOS 应用壳，负责窗口管理、系统能力对接与 HAP 打包
- **编辑器引擎（texstudio/）**：TeXstudio 核心（Qt Widgets），编译为 `libtexstudio.so` 随 HAP 加载
- **排版引擎（miktex/）**：MiKTeX 编译器套件与 TeX 宏包，打包为 `miktex.hnp` 按需挂载
- **运行时**：Qt 5.15.12 OHOS SDK（QPA 插件 `qopenharmony`）、OpenSSH 工具链（`openssh.hnp`）

## 🐛 故障排除

构建相关问题（工具链执行位修复、cmake 版本要求 ≥3.23、签名材料准备等实测踩坑点）请查阅 **[BUILD.md](BUILD.md)**，手册中均有对应章节说明。

## 🤝 贡献

欢迎提交 Issue 和 Pull Request 来帮助改进 TeX IDE！

### 开发指南
1. Fork 本项目
2. 基于 `develop/main` 创建功能分支（`git checkout -b feature/AmazingFeature`）
3. 提交更改（`git commit -m 'Add some AmazingFeature'`）
4. 推送到分支（`git push origin feature/AmazingFeature`）
5. 创建 Pull Request（目标分支 `develop/main`）

## 📞 支持与反馈

- 💬 社区：[TeX IDE 用户社区](https://gitcode.com/OpenHarmonyPCDeveloper/ohos_TeX_IDE)
- 🐛 问题反馈：[GitCode Issues](https://gitcode.com/OpenHarmonyPCDeveloper/ohos_TeX_IDE/issues)

## 📜 许可证

本项目基于 [GPL-3.0 License](LICENSE) 开源协议发布。

本项目包含的第三方开源组件及其许可信息，请参阅 [NOTICE](NOTICE) 文件。

---

**TeX IDE (based on TeXstudio)** - 让鸿蒙电脑成为完整的 LaTeX 工作站！

# PsychoPy Studio — 鸿蒙移植版

[English](./README.md) | [项目首页](https://aik358.github.io/psychopy-studio-harmonyos/)

## 项目简介

**PsychoPy Studio OH** 是开源心理学实验工具 [PsychoPy](https://www.psychopy.org/) 的鸿蒙移植版。通过 **Electron-OH** 运行时（Chromium 132, Electron 34.8.4），将 Builder、Coder、Runner 打包为原生鸿蒙 HAP。

> **当前版本: 2026.1.2** — 分支 `2026.1.2` | 包名: `com.a9iska.psychopy`

## 当前状态 (2026-07-24)

### 核心功能：✅ 已完成

| 功能 | 状态 |
|------|:---:|
| Python 3.12.8 WebSocket 通信 | ✅ |
| psychopy-lib 2026.1.2 完整导入 | ✅ |
| Builder（35组件）→ 编译 → 浏览器运行 | ✅ |
| PsychoJS ESM 实验执行 | ✅ |
| 自动 pip 安装缺失 Python 包 | ✅ |
| 文件系统访问（桌面/文档/下载） | ✅ |
| Python 终端 + 诊断 | ✅ |

### 框架升级：✅ Electron 34.8.4

| 项目 | 旧版 | 新版 |
|------|------|------|
| `libelectron.so` | ~153 MB（2025年） | **159 MB（2026-06-26）** |
| `libadapter.so` | ~1.5 MB | **5.2 MB** |
| 新增适配器 | — | EtsBridge、WebApp、NodeHandle、Popup、KVStore、CommandLine |
| `libsndfile.so` | ❌ 缺失（软跳过） | ✅ **已内置（v1.2.2）** |

### UI：✅ 中文本地化 + 平板模式识别

| 功能 | 状态 |
|------|:---:|
| i18n 框架（Svelte 5 runes、SSR 安全） | ✅ |
| 菜单 / Ribbon / 对话框 翻译（zh_CN + en_US） | ✅ |
| 语言切换器（顶部导航栏） | ✅ |
| 页面缩放滑块（50%–200%） | ✅ |
| 平板模式检测（鸿蒙 + 无 Python） | ✅ |
| 平板模式提示（右下角气泡） | ✅ |
| 视图切换 Bug 修复（移除全量重载回退） | ✅ |

### 应用市场上架：⏳ 审核中

| 项目 | 状态 |
|------|:---:|
| 包名改为 `com.a9iska.psychopy` | ✅ |
| 多尺寸图标（48~1024px） | ✅ |
| 中文本地化（zh_CN） | ✅ |
| 隐私政策 + 版权致谢 | ✅ |
| ACL 权限 ALLOW_WRITABLE_CODE_MEMORY | ⏳ 审核中 |
| Release 签名 + .app 构建 | ⏳ 等待 ACL 通过 |

## 架构图

```
┌─────────────────────────────────────────┐
│  ArkUI → Electron-OH (Chromium 132)      │
│  ┌─────────────────────────────────────┐ │
│  │  harmony-python.js（主进程）        │ │
│  │  ├─ liaison_shim.py（WebSocket）    │ │
│  │  ├─ psychopy-lib 2026.1.2           │ │
│  │  ├─ PsychoJS HTTP 服务器            │ │
│  │  └─ SvelteKit 前端                  │ │
│  └─────────────────────────────────────┘ │
│  ┌─────────────────────────────────────┐ │
│  │  系统 Python 3.12.8（HNP）          │ │
│  │  numpy / scipy / matplotlib / ...   │ │
│  └─────────────────────────────────────┘ │
│  ┌─────────────────────────────────────┐ │
│  │  原生库（arm64-v8a）                │ │
│  │  libelectron / libadapter           │ │
│  │  libffmpeg / libsndfile / libc++    │ │
│  └─────────────────────────────────────┘ │
└─────────────────────────────────────────┘
```

## 已知问题

| 问题 | 状态 |
|------|:---:|
| 平板模式闪退（MatePad Edge） | ⚠️ 等待 ACL 权限 |
| 摄像头/麦克风 | ⚠️ 集成 libsndfile 后未测试 |
| Pavlovia OAuth | ⏳ 待实现 |
| 鸿蒙 WebView 主题 CSS 可能加载失败 | ⚠️ 已有回退变量兜底 |

## 近期更新 (2026-07-24)

构建 `2026.1.2`（分支 `2026.1.2`）—— 本次提交在原有 `436fd47` 基线之上包含：

- **ESM 浏览器运行硬化（无 Python 也可）**：`psychojs-browser/startServer` 现在会输出正确的 ES 模块实验（`<script type="module" src="experiment.js">` + `psychojs-2026.1.2.js`），无论有无 Python 环境，「浏览器运行」表现完全一致。固定的 PsychoJS 路径（`psychojs-2026.1.2.js`）按设计保持不变。
- **Python 实验装配**：`python/psychopy_worker.py`（`generate` / `conditions` 子命令）在具备 Python 环境 + `.psyexp` 时装配同一套 ESM 实验目录，与无 Python 路径字节级一致。
- **平板模式与电脑模式分离**：Bundle 模式（内置 HNP，仅可行性验证）与真正无 Python 的平板模式解耦。无 Python 时应用会优雅隐藏仅 Python 的功能，但「浏览器运行」始终可用；右下角提示改为「平板模式」而非「找不到 Python」。
- **原生库**：新增 `libfreetype.so.6`（PIL / 字体）与 `libusb-1.0.so.0`（pyusb），并提供 `tools/` 下的交叉编译脚本。

## 构建步骤

**环境要求：** DevEco Studio 6.1+、鸿蒙设备（ARM64）。

```bash
git clone https://gitcode.com/A9iska/psychopy-oh.git
cd psychopy-oh && git checkout 2026.1.2
ohpm install
cd web_engine/src/main/resources/resfile/resources/app
npm install && npx vite build
cd ../../../../../../../..
# DevEco：File → Project Structure → Signing Configs → 自动签名 → Apply
# DevEco：Build → Build HAP(s)
```

## 分支

| 分支 | 说明 |
|------|------|
| `2026.1.2` | **主开发分支** — 完整功能、Electron 34.8.4、应用市场上架准备 |
| `main` | 落地页 + 文档 |
| `v0.1.6` | 此前稳定版 |

## 版权声明

PsychoPy © Open Science Tools Ltd / Jonathan Peirce，基于 GPLv3 许可。
本鸿蒙移植版由 [A9iska](https://gitcode.com/A9iska) 完成。
源码：[https://gitcode.com/A9iska/psychopy-oh](https://gitcode.com/A9iska/psychopy-oh)

## 许可

GPL v3 — 与上游 PsychoPy 保持一致。

---

> **最后更新：2026-07-24**

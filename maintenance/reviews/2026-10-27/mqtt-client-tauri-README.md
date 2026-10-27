# AtomMQTT Client — OpenHarmony

[![Rust](https://img.shields.io/badge/Rust-1.95%2B-orange)](https://rustup.rs)
[![Tauri](https://img.shields.io/badge/Tauri-2.x-blue)](https://tauri.app)
[![OpenHarmony](https://img.shields.io/badge/OpenHarmony-ARM64-red)](https://gitee.com/openharmony)

> **🔗 项目起源 & 配套服务端**: [https://atomgit.com/qq8864/atomMqtt](https://atomgit.com/qq8864/atomMqtt)  
> 本项目源自该仓库中的 MQTT 客户端，同时该仓库也提供了 **MQTT 服务端**（Broker），可用于与本客户端配合测试。

一个基于 **Tauri v2** 的 MQTT 桌面客户端，已成功移植到 **OpenHarmony**（鸿蒙）ARM64 设备上运行。支持 MQTT 连接、发布、订阅、消息查看等完整功能。

> 💡 本项目是 **Tauri 应用移植到 OpenHarmony** 的完整实践案例，包含详细的移植过程记录和可复用的技能沉淀。

**实现详细过程参考文章**：[MQTT Client的Tauri应用移植到 OpenHarmony 鸿蒙 PC/ARM64 实践记录](https://blog.csdn.net/qq8864/article/details/161806030)

---

## 🧪 配套测试服务端

**[atomMqtt](https://atomgit.com/qq8864/atomMqtt)** 仓库提供了 MQTT Broker 服务端，可直接用于与本客户端联调测试：

```bash
# 克隆服务端
git clone https://atomgit.com/qq8864/atomMqtt.git

# 启动 Broker（具体方式参见该仓库文档）
```

客户端默认配置连接 `127.0.0.1:1883`，部署到设备后修改为服务端 IP 即可。

---

## 📸 功能截图

<p align="center">
  <img src="Doc/images/home.png" alt="AtomMQTT Client 效果图" width="75%">
</p>
<p align="center"><em>AtomMQTT Client 主界面 — 运行在 鸿蒙PC 设备上</em></p>

## ✨ 功能特性

- **MQTT 连接** — 支持 TCP 直连，自定义 Client ID、用户名密码认证、Clean Session
- **消息发布** — 支持 QoS 0/1/2、Retain 标志、JSON 格式化与语法高亮
- **主题订阅** — 支持 MQTT 通配符过滤器（`+`/`#`）、QoS 选择、取消订阅
- **消息日志** — 实时显示订阅消息、时间戳、主题、QoS、Hex 模式、自动滚动
- **连接状态** — 实时显示在线/离线、连接时长
- **OHOS 调试面板** — 内置 Debug 覆盖层，显示 IPC 状态、invoke 调用日志

## 🏗️ 项目结构

```
atommqtt-client/
├── public/                       # 前端源码
│   ├── index.html                # 主页面（单页应用）
│   ├── script.js                 # 前端逻辑 + Tauri IPC + 调试面板
│   └── styles.css                # 样式
├── src-tauri/                    # Rust 后端
│   ├── src/
│   │   ├── lib.rs                # 所有 Tauri commands
│   │   └── main.rs               # 入口
│   ├── Cargo.toml                # Rust 依赖
│   ├── build.rs                  # tauri-build codegen
│   ├── tauri.conf.json           # Tauri 配置
│   └── gen/ohos/                 # OpenHarmony 工程（DevEco Studio）
│       └── entry/
│           ├── build-profile.json5
│           ├── hvigorfile.ts
│           └── src/main/resources/rawfile/  # 前端产物
├── .skills/                      # 移植技能沉淀
│   └── tauri-ohos-port/
│       ├── SKILL.md              # 完整移植指南
│       └── references/
│           └── ohos-port-details.md
├── KNOWLEDGE_BASE.md             # 项目知识库
├── OHOS_PORT_RECORD.md           # 移植过程记录
├── build-ohos.ps1                # OHOS 构建脚本
└── port.md / tauri_port.md       # 移植参考资料
```

## 🚀 快速开始

### 前提条件

| 工具 | 版本 | 说明 |
|------|------|------|
| Rust | 1.75+ | `rustup.rs` |
| Node.js | 18+ | `nodejs.org` |
| OHOS SDK | 6.x | HarmonyOS 开发者网站 |
| DevEco Studio | 5.0+ | HAP 打包 |
| tauri-cli (OHOS fork) | - | 见下方安装 |
| ohrs | - | `cargo install ohrs` |

### 安装 tauri-cli (OHOS fork)

```bash
# 克隆 OHOS 分支
git clone --branch feat/open-harmony https://ghfast.top/https://github.com/richerfu/tauri.git tauri-ohos

# ⚠️ 修复 cargo-mobile2 版本
# 编辑 tauri-ohos/crates/tauri-cli/Cargo.toml
# 修改: cargo-mobile2 = { version = "0.20.6", ... }
# 为:   cargo-mobile2 = { version = "0.22", ... }

# 安装
cargo install --path tauri-ohos/crates/tauri-cli
cargo install ohrs
```

### 构建 for OpenHarmony

```powershell
# 1. 设置环境变量
$env:OHOS_HOME = "D:\software\sdk\default\openharmony"  # SDK 根目录

# 2. 初始化（仅首次）
cd src-tauri
cargo tauri ohos init --skip-targets-install

# 3. 交叉编译
cargo tauri ohos build -t aarch64

# 4. 同步前端文件
Copy-Item -Path "..\public\*" -Destination "gen\ohos\entry\src\main\resources\rawfile" -Recurse -Force
```

### 部署到设备

```bash
# 1. 用 DevEco Studio 打开 src-tauri/gen/ohos/
# 2. Build → Build HAP(s)
# 3. 安装到设备
hdc install -r entry-default-signed.hap
# 4. 启动应用
hdc shell aa start -a EntryAbility -b com.atommqtt.app
```

## 🔧 常见问题

### 按钮无响应
**原因**: 前端 JS 语法错误导致整个脚本无法执行。OHOS Webview 没有 DevTools，错误会被静默吞掉。

**修复**: 
- 使用 `node --check public/script.js` 验证语法
- 检查 `try`/`catch` 是否配对
- 检查字符串引号是否闭合（中文编码转换可能吃掉结尾引号）

### 界面乱码
**原因**: UTF-8 中文文本被错误编码解码（Mojibake）。

**修复**: 将文件重新以 UTF-8 无 BOM 保存，替换乱码字符串为正确中文。

### DevEco Studio 构建报错

| 错误 | 原因 | 修复 |
|------|------|------|
| `cargo tauri ohos dev-eco-studio-script` 失败 | hvigorfile.ts 中调用了 cargo 命令 | 注释掉该调用 |
| `build-profile.json5` schema 验证失败 | 包含了不被支持的字段 | 删除 `runtimeOSVersion`/`runtimeOSBranch`/`enableParallel` 等 |
| `ohpm not found` | 缺少 ohpm | 安装 DevEco Studio |

### OHOS Debug 面板
`script.js` 中内置了 🐛 OHOS Debug 面板，部署后屏幕底部会显示：
- `window.__TAURI__` 是否存在及类型
- 每次 `invoke()` 调用的请求和响应
- 错误信息

## 📚 参考资源

- **🧪 配套服务端 & 项目起源**: [atomMqtt](https://atomgit.com/qq8864/atomMqtt) — MQTT Broker 服务端，本客户端即源于此项目
- [Tauri v2 官方文档](https://v2.tauri.app)
- [Tauri OHOS Fork (richerfu)](https://github.com/richerfu/tauri/tree/feat/open-harmony)
- [Tauri 移植到 OHOS 示例](https://github.com/richerfu/tauri-demo)
- [OpenHarmony 官方文档](https://gitee.com/openharmony)
- [rumqttc MQTT 库](https://github.com/bytebeamio/rumqttc)

## 📄 许可证

MIT

---

> 本项目是 Tauri 应用移植到 OpenHarmony 的探索性实践，客户端原始代码源自 [atomMqtt](https://atomgit.com/qq8864/atomMqtt)，移植思路参考了 [tauri-demo](https://github.com/richerfu/tauri-demo)，使用 rumqttc 实现 MQTT 协议。

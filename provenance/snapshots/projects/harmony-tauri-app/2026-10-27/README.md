# harmony-tauri-app

**在鸿蒙 PC（HarmonyOS / OpenHarmony，aarch64，2in1）上用 Rust + Tauri 写的鸿蒙应用。**

这个仓库把「从零把一个 Tauri 应用跑在鸿蒙 PC 上」的全过程整理成了一个可复用的工程：
Rust 侧用 `tauri` 的 `feat/open-harmony` 分支编译成动态库，由 ArkTS 的 `NativeAbility`
加载进应用进程，界面是 ArkWeb（Chromium）webview，前端通过 Tauri IPC 调用 Rust 命令。

已在真机验证：HUAWEI MateBook Pro（HAD-W32，HarmonyOS 7.0.0.111 / API 26，aarch64）。

![运行效果](docs/img/running.png)

## 技术栈

| 层 | 组件 | 版本 |
|---|---|---|
| ArkTS 壳 | `@ohos-rs/ability` 的 `NativeAbility` | 0.4.0-beta.5 |
| Rust 框架 | `tauri` / `tauri-runtime-wry` | 2.8.5 / 2.8.1（`feat/open-harmony`） |
| webview 后端 | `wry` / `tao` | 0.55.1 / 0.35.3 |
| Ability 绑定 | `openharmony-ability` | 0.3.0（rev `295a276a`） |
| Rust 工具链 | rustc / cargo，host = `aarch64-unknown-linux-ohos` | 1.99.0 |
| 前端 | 零依赖静态 HTML/JS（`withGlobalTauri`） | — |
| 构建 | hvigor / ohpm / DevEco SDK | 7.26.8 / 6.0.1 / API 26 |

## 架构

```
ArkTS：EntryAbility extends NativeAbility
        moduleName = "tauri_demo_lib"
        import … from 'libtauri_demo_lib.so'
   │
Rust：libtauri_demo_lib.so（cdylib，打包进 libs/arm64-v8a/）
        tauri（应用框架 / IPC / 资源协议）
        wry（ohos 后端 → ArkWeb）
   │
Web：web/index.html + main.js（window.__TAURI__.core.invoke("greet", …)）
```

一次点击的链路：webview 里 `invoke()` → Tauri IPC → `src-tauri/src/lib.rs` 的
`#[tauri::command]` → 返回值回填 DOM。反向（Rust → ArkTS）走 `napi-ohos` 的回调，
示例见 `lib.rs` 里的 `register_main_thread_arkts_callback` 等三个函数。

## 目录结构

```
web/                         零依赖前端（编译期被嵌入 Rust 库）
tools/                       构建脚本（见下）
signing/README.md            签名材料怎么来（仓库不含私钥）
docs/                        两篇长文文档 + 运行截图
src-tauri/
  Cargo.toml                 tauri fork + patch + custom-protocol
  tauri.conf.json            应用身份、窗口、frontendDist
  src/lib.rs                 命令（greet / app_info）+ N-API 回调样例
  gen/ohos/                  鸿蒙工程（等同 `tauri ohos init` 的产物）
    assets/tauri.conf.json   运行时真正读的配置
    entry/libs/arm64-v8a/    Rust 库落点（构建生成，不入库）
```

## 快速开始

### 前置条件

- 鸿蒙 PC（或已连接的真机/模拟器），`hdc list targets` 能看到设备；
- HarmonyOS Command Line Tools（提供 `hdc` / `ohpm` / `hvigorw` / DevEco SDK）；
- Rust 工具链，host 必须是 `aarch64-unknown-linux-ohos`（`rustc -vV | grep host` 自查）；
- Node.js（供 ohpm / hvigorw / devecocli 使用）；
- 一个华为开发者账号（签名材料必须来自 AGC，见 `signing/README.md`）。

Rust 工具链与链接器的准备（本机没有 `cc`，必须配置带签名的链接器包装器）
见文档 [docs/harmonyos-rust-setup.md](docs/harmonyos-rust-setup.md)。

### 构建与运行

```sh
# 1) 拉依赖（脚本会自动修复 cargo 在这台机器上的 EINTR checkout 故障）
sh tools/cargo-fetch-retry.sh

# 2) 编译 Rust 库并放进 ArkTS 工程
sh tools/build-rust.sh

# 3) 生成 AGC 签名材料（需已登录 devecocli 且设备在线）
cd src-tauri/gen/ohos && devecocli signature generate --product default

# 4) 构建 + 签名 + 安装 + 启动
devecocli run --module entry

# 5) 验证界面（读设备上的界面节点树，比看截图直接）
devecocli ui layout | head -40
```

纯 hdc 的替代路径：`sh tools/install-hap.sh`（安装 + 启动，失败时会打印本机 UDID 与
该去 AGC 做什么）。CI 场景可以用 `sh tools/build-hap.sh` 走
「hvigorw 出未签名包 → `hap-sign-tool` 显式签名」。

## 脚本说明

| 脚本 | 解决的问题 |
|---|---|
| `tools/cargo-fetch-retry.sh` | cargo 取依赖的两个故障：内置 git 客户端 TLS 失败、git checkout 不重试 EINTR。脚本从错误里取出出问题的 checkout 目录，用 `git archive` + 空 `.cargo-ok` 重铺后重试 |
| `tools/build-rust.sh` | 手工构建缺失的环境注入（`WRY_OHOS_*` / `TAURI_OHOS_*` / `OHOS_NDK_HOME` / `TAURI_ENV_DEBUG=false`），同步 `assets/tauri.conf.json`，并把 `.so` 放进 `libs/arm64-v8a/` |
| `tools/build-hap.sh` | hvigor 只认 DevEco 加密态口令，因此改为产出未签名 HAP 再用 `hap-sign-tool` 签名；签名材料可用 `SIGN_*` 环境变量替换 |
| `tools/install-hap.sh` | 纯 hdc 的安装与启动，失败时给出明确指引 |

## 几个容易踩的坑

| 现象 | 根因 | 处理 |
|---|---|---|
| `code:9568257 error: fail to verify pkcs7 file` | HarmonyOS 设备不信任 OpenHarmony 根证书签的应用 | 用 AGC 的调试证书 + 绑定本机 UDID 的 Profile（`devecocli signature generate` 可自动化） |
| webview 里只有 `Failed to request tauri://localhost/: builder error for url` | ①构建被判成 dev（未启用 `custom-protocol`）②运行时 `assets/tauri.conf.json` 没同步 ③窗口没写 `url` | 三条一起对齐，详见文档第 7 章 |
| `error: linker \`cc\` not found` | 系统没有 C 工具链 | 配好带签名的链接器包装器（见文档第 6 章） |
| `Operation not permitted`（126） | 产物未签名，或 `.codesign` 只是占位段 | 链接器包装器会在链接后自动剥段并自签名 |
| `OHOS_NDK_HOME not set: NotPresent` | 缺构建期环境变量 | `tools/build-rust.sh` 里已注入 |

完整的报错对照表（20 条）见文档第 10 章。

## 文档

- [docs/harmonyos-tauri-guide.md](docs/harmonyos-tauri-guide.md) —— 主文档：在鸿蒙 PC 上用 Rust + Tauri 开发鸿蒙应用（架构、构建、签名、调试、踩坑总表）
- [docs/harmonyos-rust-setup.md](docs/harmonyos-rust-setup.md) —— 前置文档：鸿蒙 PC 上的 Rust 开发环境搭建（musl / 无 C 工具链 / 必须签名 / EINTR 四个平台差异，以及 `aarch64-unknown-linux-ohos` 工具链的安装）

## 已知限制

- 依赖的是 feature 分支（`tauri` / `wry` / `tao` 的 ohos 支持各在分支上），升级需重新验证；
- `openharmony-ability` 固定在 rev `295a276a`（上游之后移除了 webview 相关能力）；
- 调试签名与设备 UDID 绑定，换设备需重新申请 Profile；
- 首次全量构建约 2 分钟（20 核 aarch64），改配置触发的重编 30～44 秒；
- 热重载（`devecocli run --hotreload`）未在本工程验证。

## 致谢与许可

- 工程骨架（`gen/ohos` 与 N-API 回调样例）来自 [richerfu/tauri-demo](https://github.com/richerfu/tauri-demo)；
- 框架来自 [tauri-apps/tauri](https://github.com/tauri-apps/tauri) 的 `feat/open-harmony` 分支与 [ohos-rs](https://ohos.rs/) 社区；
- 本仓库代码与文档以 **MIT OR Apache-2.0** 释出，与上游 Tauri 保持一致。

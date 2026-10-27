# CC Switch for HarmonyOS（鸿蒙 PC 移植版）

> 将 **Tauri v2** 桌面应用 **cc-switch**（Claude Code / Codex / Gemini CLI 的 All-in-One 配置管理器）迁移为可在 **鸿蒙 PC / 2in1** 上安装运行的 **HAP** 应用。
>
> 技术路线：Rust 后端交叉编译为 `libcc_switch_lib.so`，通过 **napi-ohos 桥接 + ArkWeb** 渲染原版 React 前端，由 DevEco Studio 打包为 HAP —— **前端零框架改动**。

**上游项目**：[farion1231/cc-switch](https://github.com/farion1231/cc-switch)（Tauri v2 + React，支持 Windows / macOS / Linux）

**移植详细文章（猫哥）**：[cc-switch 鸿蒙 PC 移植：使用 Tauri 官方 feat/open-harmony 分支最新版本移植实录](https://blog.csdn.net/qq8864/article/details/163865027)

## 移植原理

```
cc-switch (Rust + React)
  → libcc_switch_lib.so（napi-ohos 桥接，mobile_entry_point 宏展开）
  → OHOS ArkWeb WebView（渲染原前端，withGlobalTauri 全局桥）
  → HAP（hvigor 打包，签名后 hdc 部署）
```

- 依赖的 OHOS fork：[yangyongzhen/tauri](https://github.com/yangyongzhen/tauri) `feat/open-harmony` 分支（**官方 tauri-apps 基线 2.11.5** = crates.io 最新版，插件零降级）
- wry / tao：tauri-apps 官方仓库 `feat/open-harmony` 分支（0.56.0 / 0.36.0）
- openharmony-ability：钉 0.3.0（`yangyongzhen/openharmony-ability` `feat/0.3.0` 分支）
- 依赖全部通过 `[patch.crates-io]` **git URL + rev 固定**，无本机路径，clone 即用

## 工程结构

| 路径 | 说明 |
|------|------|
| `src-tauri/` | Rust 后端（tauri 2.11.5 + 全部 `cfg(desktop)` 平台门控） |
| `src-tauri/capabilities/ohos.json` | OHOS 平台权限（`platforms: ["openHarmony"]`） |
| `src-tauri/gen/ohos/` | DevEco 工程（ArkTS 壳，`entry/hvigorfile.ts` 已去除 cargo 钩子） |
| `src/` | React 前端（原版，仅 `index.html` 加了 localStorage polyfill） |
| `doc/cc-switch-to-harmonyos-porting-guide.md` | 完整移植手册（17 个坑 + 命令速查） |
| `doc/cc-switch-鸿蒙PC移植实录-官方fork版.md` | 官方 fork 移植实录 |

## 移植说明（T0 / T1 / T2 分级）

按 **T0→T1→T2** 三级递进完成 cc-switch 从桌面 Tauri 应用到鸿蒙 HAP 的移植，当前进展如下：

### T0 — 基础框架迁移（已完成 ✅）

> 应用可在鸿蒙设备上正常启动、显示主界面，主进程 / GPU / render 三进程稳定存活。

| 功能项 | 状态 | 说明 |
|--------|------|------|
| Rust 交叉编译（aarch64-unknown-linux-ohos） | ✅ 已完成 | `libcc_switch_lib.so`（aarch64 ELF，strip 后 23.7MB） |
| napi-ohos 桥接与 `mobile_entry_point` | ✅ 已完成 | `RustAbility`（@ohos-rs/ability）加载 .so |
| ArkWeb 前端渲染 | ✅ 已完成 | 前端从 `tauri://localhost/assets/...` 加载（生产模式，内嵌 .so） |
| 应用生命周期与窗口 | ✅ 已完成 | 支持 `2in1`（PC 形态），窗口由系统管理 |
| 数据库迁移与初始化 | ✅ 已完成 | schema v10→v16、189 条默认模型定价、官方 providers 导入 |
| 后端服务初始化 | ✅ 已完成 | Copilot / Codex / Xai OAuth Manager、全局代理服务全部就绪，0 错误日志 |
| HAP 打包 / 签名 / 部署 | ✅ 已完成 | `entry-default-signed.hap`（34.8MB），`hdc install` + `aa start` 真机验证 |

### T1 — 核心功能迁移（已完成 ✅）

> 配置管理、供应商切换、MCP / Prompts / Skills 等核心业务链路可用。

| 功能项 | 状态 | 说明 |
|--------|------|------|
| 供应商配置管理（增删改查 / 一键切换） | ✅ 正常 | 基于 SQLite（rusqlite bundled），原子写入 |
| 模型定价与用量统计 | ✅ 正常 | 189 条内置定价，本地代理服务正常 |
| 本地代理与请求转发 | ✅ 正常 | GlobalProxy 初始化：direct connection |
| OAuth 授权流程（Copilot / Codex / Xai） | ✅ 正常 | Manager 初始化成功，运行时代理链路可用 |
| 前端 UI 全量渲染 | ✅ 正常 | React 挂载正常，0 panic / 0 error_boundary |
| 数据目录落盘 | ✅ 正常 | `<sandbox>/.cc-switch/`（cc-switch.db / settings.json / logs） |

### T2 — 完整功能体验迁移（部分完成 🚧）

> 补齐外部系统交互与平台差异适配；受 OHOS 平台能力限制，部分桌面功能无法等价。

| 功能项 | 状态 | 说明 |
|--------|------|------|
| 系统托盘快速切换 | ❌ 平台无 | OHOS 无系统托盘，tray 命令返回不支持 |
| 原生文件对话框 / 打开文件夹 | ❌ 不支持 | 返回「当前平台不支持」，前端已容错 |
| 应用自更新（updater） | ❌ 不支持 | OHOS 段不注册插件，前端提示「检查更新失败」（ACL 拒绝，非崩溃） |
| localStorage 持久化 | ⚠️ 内存降级 | ArkWeb 自定义 scheme 下 localStorage 为 null，polyfill 内存实现，**重启不持久化** |
| 窗口主题切换（深色模式） | ⚠️ 不支持 | 返回「当前平台不支持切换窗口主题」，前端已容错 |
| 外部链接打开 / deep-link | ⏸️ 待验证 | 命令层降级返回，前端容错 |
| x86_64 鸿蒙 PC | ⏸️ 工具链已备 | `x86_64-unknown-linux-ohos` target + config 已配，直接 `-t x86_64` 构建 |
| 桌面三端回归 | ⏸️ 待验证 | 插件已升到最新版，建议桌面构建回归测试 |

## 已知问题

1. **localStorage 不持久**：ArkWeb 在 `tauri://localhost` 自定义 scheme 下 `window.localStorage` 为 null，当前用内存 polyfill 保证不崩，**重启后偏好设置丢失**。后续可改走后端 store / 配置文件持久化。
2. **检查更新提示**：每次启动前端会提示「检查更新失败」（updater 插件在 OHOS 段未注册，ACL 拒绝）。属预期降级，不影响使用。
3. **无系统托盘 / 无原生对话框**：OHOS 平台能力限制，相关命令返回「当前平台不支持」，前端均已容错。
4. **窗口主题切换不支持**：深色模式切换命令降级返回，界面样式由前端控制。
5. **屏幕锁定时无法自动启动**：真机处于开发者模式时，屏幕锁定会拦截 `aa start`（错误码 10106102），需先唤醒/解锁屏幕（`uinput` 模拟按键可唤醒息屏状态）。

## 环境要求

| 组件 | 要求 | 备注 |
|------|------|------|
| 开发机 | Windows 10/11 x64（本手册实测）或 Linux | |
| Rust | `stable-x86_64-pc-windows-gnullvm` 工具链 | 交叉编译必需（普通 gnu 链接器报 `-lgcc_eh` 缺失） |
| OHOS 目标 | `rustup target add aarch64-unknown-linux-ohos` | 官方 Tier 2；x86 鸿蒙 PC 另加 `x86_64-unknown-linux-ohos` |
| OHOS SDK（NDK） | HarmonyOS NEXT +（含 `native/llvm` + sysroot） | 交叉编译 Rust 用，环境变量 `OHOS_HOME` / `OHOS_NDK_HOME` 指向 SDK 根 |
| DevEco Studio | 5.0+ | 自带 hvigor / ohpm / hdc / node；SDK 指**完整 SDK**（含 ets），指纯 NDK 报 `SDK component missing` |
| tauri CLI（fork） | `cargo install --path <fork>/crates/tauri-cli`（2.11.4）+ `ohrs` 1.5+ | fork 见 `[patch.crates-io]`（`src-tauri/Cargo.toml`），rev 固定 |
| llvm-mingw | `bin` 加入 PATH | ring 等 C 依赖交叉编译必需 |
| 真机 | 鸿蒙 PC / 平板 ARM64，开开发者模式 | 实测 HUAWEI MateBook Pro HAD-W32，API 24 |

## 构建与部署

```bash
# 1. 交叉编译 .so（自动跑 pnpm build 并内嵌 dist）
cd src-tauri
export OHOS_HOME="<SDK根>" && export OHOS_NDK_HOME="$OHOS_HOME"
cargo tauri ohos build -t aarch64

# 2. 同步前端 rawfile（CLI 不会自动做）
cp -r dist/. src-tauri/gen/ohos/entry/src/main/resources/rawfile/

# 3. 手工装配 HAP（Windows）
cd src-tauri/gen/ohos && ohpm install && cd entry && ohpm install && cd ..
node "<DevEco>/tools/hvigor/bin/hvigorw.js" assembleHap --mode module -p product=default --no-daemon

# 4. 签名：首次需 DevEco Studio GUI（华为账号）生成材料写入 build-profile.json5
#    之后命令行可复用；材料位于 ~/.ohos/config/

# 5. 真机部署
hdc install -r entry/build/default/outputs/default/entry-default-signed.hap
hdc shell "power-shell wakeup" && hdc shell "aa start -a EntryAbility -b com.ccswitch.desktop"
```

> 详细踩坑记录与命令速查见 [doc/cc-switch-to-harmonyos-porting-guide.md](doc/cc-switch-to-harmonyos-porting-guide.md)。

## 相关资源

- 移植详细文章（猫哥）：[cc-switch 鸿蒙 PC 移植：使用 Tauri 官方 feat/open-harmony 分支最新版本移植实录](https://blog.csdn.net/qq8864/article/details/163865027)
- 移植实录：[doc/cc-switch-鸿蒙PC移植实录-官方fork版.md](doc/cc-switch-鸿蒙PC移植实录-官方fork版.md)
- 完整移植手册：[doc/cc-switch-to-harmonyos-porting-guide.md](doc/cc-switch-to-harmonyos-porting-guide.md)
- Tauri OHOS fork：[yangyongzhen/tauri](https://github.com/yangyongzhen/tauri)（`feat/open-harmony`）
- 上游 **cc-switch**：[farion1231/cc-switch](https://github.com/farion1231/cc-switch)

## 许可证

本移植仓库遵循上游 **cc-switch** 的 **MIT** 许可（© Jason Young）；上游源码与第三方依赖各遵循其自身许可证。

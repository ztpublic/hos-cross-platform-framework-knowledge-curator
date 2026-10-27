# ElectronShell

基于 **Electron v37** 的 HarmonyOS / OpenHarmony **浏览器测试壳**。

应用本身只做三件事：**全屏承载 Electron 浏览器组件**、**提供一个左下角悬浮的地址入口**、**记住上次访问的网址**。它用于在鸿蒙真机上验证 Electron 内核的渲染、网络、多进程与输入能力，也可以直接作为 Electron 类鸿蒙应用的启动壳。

---

## 目录

- [功能特性](#功能特性)
- [效果](#效果)
- [环境要求](#环境要求)
- [快速开始](#快速开始)
- [使用说明](#使用说明)
- [工程结构](#工程结构)
- [调试与测试](#调试与测试)
- [规范文档](#规范文档)
- [已知限制](#已知限制)
- [FAQ](#faq)

---

## 功能特性

| 能力 | 说明 |
|------|------|
| 沉浸式全屏 | 浏览内容铺满整屏，覆盖系统状态栏与底部导航条；2in1 形态自动最大化并隐藏标题栏 |
| Electron 渲染 | 由 Electron v37.2.3 渲染网页（Chromium 渲染进程 + GPU + 网络服务独立进程） |
| 启动记忆 | 首次启动显示空页面并展开地址栏；之后启动直接加载上次确认的网址 |
| 悬浮地址入口 | 左下角常驻圆形按钮，点击在底部展开地址输入面板 |
| 输入规整与校验 | 省略协议自动补 `https://`；非法地址就地提示、不跳转 |
| 单窗口浏览 | 网页内 `target="_blank"` / `window.open` 一律在当前窗口打开；`mailto:` 等外部协议交系统处理 |
| 响应式 | 手机 / 平板 / 2in1 / TV 四形态，按窗口宽度分紧凑 / 常规 / 宽屏三档，横竖屏切换实时重排 |
| 可观测 | 状态文件路径、导航序号、加载成功/失败均有日志，便于真机排障 |

---

## 效果

全屏浏览（百度）：

```
┌──────────────────────────────────────────┐
│  ← 网页内容（Electron 渲染，铺满整屏）    │
│                                          │
│                                          │
│  ▉ URL                                   │  ← 左下角悬浮按钮
└──────────────────────────────────────────┘

点击按钮后：

┌──────────────────────────────────────────┐
│  ← 网页内容                              │
│                                          │
│   ┌────────────────────────────────┐     │
│   │ https://www.baidu.com  [前往] ✕│     │  ← 底部输入面板
│   └────────────────────────────────┘     │
└──────────────────────────────────────────┘
```

---

## 环境要求

| 项 | 要求 | 本工程实测 |
|----|------|-----------|
| DevEco Studio | 6.0+（提供 HarmonyOS SDK 与 jbr） | 已安装 |
| HarmonyOS SDK | API 20 及以上（`web_engine` 依赖 HarmonyOS 专属 Kit） | 6.1.1（API 24） |
| Node.js | 18.x | v18.20.1 |
| hvigor / ohpm | 6.1+ | `command-line-tools` |
| 设备 | HarmonyOS 真机或模拟器，开启开发者模式与 USB 调试 | `HAD-W32`（2in1，API 26） |
| 华为开发者账号 | **真机安装必需**（DevEco 自动签名生成 AGC debug profile） | 已配置 |

> 仅编译不需要华为账号；**安装到 HarmonyOS 真机必须**使用华为 AGC 签发的 profile。OpenHarmony 设备可用工程内的自签名材料。

---

## 快速开始

### 1. 克隆并装配资源

Electron 内核二进制（约 183MB）与运行时资源（约 18MB）不入库，需要从源工程装配：

```bash
git clone <repo> ElectronShell
cd ElectronShell

# 默认从 d:/pi-workspace/harmonypc-electron 装配，可用 --from 指定其它路径
bash scripts/assemble-resources.sh
```

脚本会复制 `libelectron.so` / `libadapter.so` / `libffmpeg.so` 与 `resfile` 运行时资源，并按 `runtime-manifest.json` 校验 MD5（应为 v37.2.3）。

### 2. 配置本机 SDK 路径

新建 `local.properties`（不入库）：

```properties
nodejs.dir=D\:\\oh-workspace\\DevEco Studio\\tools\\node
sdk.dir=D\:\\oh-workspace\\DevEco Studio\\sdk
hvigor.dir=D\:\\oh-workspace\\command-line-tools\\hvigor
```

### 3. 安装依赖并构建

```bash
ohpm install --all
bash scripts/build.sh          # 产出 electron-default-signed.hap
```

`scripts/build.sh` 的行为：

- 若 `build-profile.json5` 已配置 `signingConfigs`（DevEco 自动签名写入），`hvigor` 直接产出 **已签名** HAP；
- 否则只产出未签名 HAP，再由脚本用 `signing/` 下的 OpenHarmony 调试材料签名（仅适用于 OpenHarmony 设备）。

### 4. 真机安装与启动

```bash
hdc app install -r electron/build/default/outputs/default/electron-default-signed.hap
hdc shell aa start -a EntryAbility -b org.fellow99.test.ElectronShell
```

首次在 DevEco Studio 中打开工程时，建议执行一次自动签名：

> **File > Project Structure > Project > Signing Configs > 勾选 Automatically generate signature > OK**

完成后 `build-profile.json5` 会自动写入 `signingConfigs`。

---

## 使用说明

| 场景 | 操作 |
|------|------|
| 首次使用 | 启动后底部已展开输入框，直接输入网址（如 `www.baidu.com`）→ 点「前往」 |
| 换一个网址 | 点左下角 **URL** 悬浮按钮 → 修改输入 → 「前往」 |
| 取消输入 | 点面板右侧 **✕** |
| 键盘提交 | 输入框内按软键盘「前往 / Go」等同于点「前往」 |
| 继续浏览 | 面板收起后网页全屏可交互，点链接即在当前窗口跳转 |

地址支持省略协议：输入 `www.threejs.org` 会自动补全为 `https://www.threejs.org`。

---

## 工程结构

```
ElectronShell/
├── AppScope/                 # 应用级配置（bundleName=org.fellow99.test.ElectronShell）
│   └── resources/base/media/app_icon.png   # ★ 应用图标（1024×1024 PNG）
├── electron/                 # 主 entry HAP：应用入口、页面、壳界面、持久化、内核通道
│   └── src/main/ets/
│       ├── common/           # ShellConstants、ShellUrlHelper（纯函数）
│       ├── service/          # ShellUrlStore（preferences）、ElectronShellBridge（状态文件）
│       ├── components/       # ShellOverlay（浮动按钮 + 地址面板 + 响应式）
│       ├── pages/            # Index / NodeHandleWindow 等 9 条路由
│       └── entryability/     # EntryAbility（沉浸式全屏）等
├── web_engine/               # HAR：Electron 承载能力 + 46 个系统适配器
│   └── src/main/resources/resfile/
│       └── resources/app/    # 本工程自有的 Electron 应用（main.js / blank.html）
├── scripts/
│   ├── assemble-resources.sh # 从源工程装配内核资源并校验 MD5
│   └── build.sh              # 构建（+ 可选本地签名）
├── signing/                  # OpenHarmony 本地调试签名材料与说明
├── specs/                    # 规范文档（架构、技术、模块规格、测试用例）
├── logs/                     # 开发/评审/测试日志
├── build-profile.json5       # 产品配置与签名
└── runtime-manifest.json     # 上游装配清单（内核版本、MD5）
```

目录级细节见 [specs/STRUCTURE.md](specs/STRUCTURE.md)。

---

## 调试与测试

### 常用命令

```bash
# 安装 / 启动 / 停止
hdc app install -r electron/build/default/outputs/default/electron-default-signed.hap
hdc shell aa start -a EntryAbility -b org.fellow99.test.ElectronShell
hdc shell aa force-stop org.fellow99.test.ElectronShell

# 截图
hdc shell snapshot_display -f /data/local/tmp/s.jpeg
MSYS_NO_PATHCONV=1 hdc file recv /data/local/tmp/s.jpeg ./s.jpeg

# UI 树 / 点击 / 输入
hdc shell "uitest dumpLayout -b org.fellow99.test.ElectronShell -p /data/local/tmp/l.json"
hdc shell "uitest uiInput click <x> <y>"
hdc shell "uitest uiInput text https://www.baidu.com"
hdc shell "uitest uiInput keyEvent 2055"     # 删除一个字符

# 日志（本机 hilog 不支持 -x 与 -z 组合）
hdc shell hilog -x | grep -E "ElectronShellBridge|ShellOverlay|ElectronShell\]"

# 查看持久化数据
hdc shell cat /data/app/el2/100/base/org.fellow99.test.ElectronShell/files/electron-shell-state.json
hdc shell cat /data/app/el2/100/base/org.fellow99.test.ElectronShell/haps/electron/preferences/electron_shell_store
```

### 验证清单（人工）

| 步骤 | 预期 |
|------|------|
| 首次启动 | 空页面 + 底部输入面板自动展开 |
| 输入 `www.baidu.com` → 前往 | 百度首页全屏显示，面板收起，左下角出现 URL 按钮 |
| 点击百度顶部「新闻」 | 当前窗口切换到百度新闻（不出现第二个窗口） |
| 输入 `https://www.threejs.org` → 前往 | Three.js 官网作品墙（该站点首次加载可能需 10~20s） |
| 关闭应用后重新打开 | 直接回到上次访问的网址 |
| 输入 `https://a.comhttps://b.com` → 前往 | 面板下方红字提示，页面不跳转 |

完整用例： [specs/001-electron-shell/test-cases.md](specs/001-electron-shell/test-cases.md)
执行记录： [logs/20261020-1/TEST_REPORT.md](logs/20261020-1/TEST_REPORT.md)

### 测试网站

- <https://www.baidu.com> — 验证中文站点渲染与页内链接跳转
- <https://www.threejs.org> — 验证 WebGL/Canvas 与复杂页面（首页 → `examples` → 具体示例）

---

## 规范文档

全部文档位于 [specs/](specs/)，索引见 [specs/README.md](specs/README.md)。

| 文档 | 说明 |
|------|------|
| [ARCHITECTURE.md](specs/ARCHITECTURE.md) | 分层、运行时拓扑、数据流、架构决策 |
| [TECH.md](specs/TECH.md) | 技术栈、版本、依赖、选型理由 |
| [STRUCTURE.md](specs/STRUCTURE.md) | 目录结构、页面/路由清单、Ability 清单 |
| [constitution.md](specs/constitution.md) | 工程原则与合入红线 |
| [overall-spec.md](specs/overall-spec.md) | 整体规格（做什么） |
| [overall-plan.md](specs/overall-plan.md) | 整体技术方案（怎么做） |
| [overall-test-cases.md](specs/overall-test-cases.md) | 测试用例索引与需求覆盖矩阵 |
| [SPECS_CHECKLIST.md](specs/SPECS_CHECKLIST.md) | 文档完成情况清单 |
| [001-electron-shell/](specs/001-electron-shell/) | 浏览器壳模块的规格、方案、任务、测试用例 |

---

## 已知限制

| 限制 | 影响 | 备注 |
|------|------|------|
| 真机签名依赖华为 AGC | 无账号无法安装到 HarmonyOS 真机 | CI 需预置 profile，或改用 OpenHarmony 设备 |
| 导航延迟 | 确认地址到内核开始加载最多约 300ms | 内核轮询状态文件的周期 |
| 单窗口 | 不支持多标签、弹窗浏览 | `setWindowOpenHandler` 全部收敛 |
| 仅 http/https | 输入框不能打开 `file://`、`chrome://` | 输入校验白名单 |
| 网页内不能粘贴 | 未申请 `READ_PASTEBOARD`（受限权限），网页内容里的 Ctrl+V / 右键粘贴无效 | URL 输入框（ArkUI）粘贴不受影响；需要网页粘贴时恢复该权限即可 |
| 网页不能直写下载/文档/桌面目录 | 已裁剪三个目录权限 | 需要另存到这些目录时恢复对应权限 |
| 不支持扩展 / native messaging | 已裁剪 `WEB_NATIVE_MESSAGING` | 本工程不加载扩展 |
| 平板非 PC 模式 | 可能顶部留白 | `WebAbility` 会把系统状态栏高度写入 LocalStorage 用于内缩原生窗口 |
| HAP 体积 | 约 203MB | 主要来自 `libelectron.so`（181MB） |

---

## FAQ

**Q：`Phone` 设备类型为什么写成 `default`？**
A：hvigor 校验不接受 `phone`，`default` 是手机形态的通用取值；四形态映射见 `specs/TECH.md` §2.3。

**Q：为什么不用 OpenHarmony SDK 编译？**
A：`web_engine` 依赖 `@kit.ShareKit`、`@kit.CoreSpeechKit`、`@kit.CoreVisionKit`、`@kit.StatusBarExtensionKit`，OpenHarmony SDK 不提供，会报 `10311003 ... has no corresponding config file in ArkTS SDK`。

**Q：`hvigorw` 报 `ERR_WORKER_INVALID_EXEC_ARGV`？**
A：`NODE_OPTIONS` 里带了 `--openssl-legacy-provider`，构建脚本已 `unset NODE_OPTIONS`，手动执行时请先 `unset NODE_OPTIONS`。

**Q：安装报 `fail to verify pkcs7 file`？**
A：使用的签名 profile 不是华为 AGC 签发。请在 DevEco Studio 里开启自动签名后重新构建安装。

**Q：网页白了/没加载？**
A：先看日志 `hdc shell hilog -x | grep "ElectronShell\]"`，`did-fail-load` 会给出错误码与 URL；常见原因是目标站点在当前网络不可达。

---

## License

本工程仅用于内部验证与调试；Electron 内核与 `web_engine` 适配层版权归其各自权利人所有（见源工程 LICENSE）。

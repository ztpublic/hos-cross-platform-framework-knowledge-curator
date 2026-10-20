# JupyterLab · OpenHarmony PC 编译运行与适配指南

本文档面向**二次开发者 / 审核验收**，说明如何把本仓库的 JupyterLab 适配工程编译成 HAP、安装到鸿蒙 PC 真机，以及为了跑通「OpenHarmony Electron 承载 JupyterLab」我们具体做了哪些 PC 平台适配。

主文档（能力清单 / 架构 / 路线图）见同仓 `README.md`。

---

## 1. 环境要求

| 项目 | 要求 | 备注 |
|------|------|------|
| Node.js | 18+ | 仅用于把 Electron 主进程与前端资源打包成 OHOS 资源；原工程基于 Electron |
| DevEco Studio | 已安装 HarmonyOS SDK | 需带命令行 `hvigorw` |
| compatibleSdkVersion | `6.0.1(21)` | 见 `ohos_hap/build-profile.json5`，**不要**写 `compatibleSdkVersionStage` |
| targetSdkVersion | `6.0.1(21)` | 实际编译目标版本 |
| 目标设备 | `2in1` / `tablet`（arm64-v8a） | 真机或对应模拟器均可；运行时 `.so` 为 arm64-v8a |
| 签名 | 需为 `org.jupyter.lab.ohos` 重新生成调试签名 | **不可复用** Notebook 的 `.p7b`（见 §6.2） |

> 关键约束：本工程与 `ohos_JupyterNotebook` 共享同一套 runtime 源码（通过 `bootstrap` 派生）。因此**首次必须先从 Notebook 工程同步 runtime**，否则 `ohos_hap/` 内没有可打包的主进程代码。

---

## 2. 整体流程

```text
拉代码 → 装依赖 → 从 Notebook 同步 runtime(bootstrap)
       → 打包资源(build-package) → 编 HAP(build-hap / DevEco)
       → DevEco 自动签名 → 安装到真机/模拟器 → aa start 启动
```

> 注：本仓库的 Electron 主进程 / `staticLabServer.js` 与 `jupyterlab.js` 目前由 `scripts/bootstrap-from-notebook.mjs` 从 `ohos_JupyterNotebook` 派生生成（避免重复维护 ~2k 行代码）。如需独立维护，可把派生结果固化进 `pkg/ohos/runtime/`。

---

## 3. 拉取代码与依赖

```bash
git clone https://atomgit.com/OpenHarmonyPCDeveloper/ohos_JupyterLab.git
cd ohos_JupyterLab
npm install        # 安装打包脚本依赖
```

---

## 4. 从 Notebook 工程同步 runtime（必做）

JupyterLab 与 Jupyter Notebook 共享同一套 OHOS Electron 方案。本仓库通过脚本从 `ohos_JupyterNotebook` 派生出 Lab 专属的 runtime：

```bash
# 仅 runtime 主进程 + static server
node scripts/bootstrap-from-notebook.mjs

# 若需完整 HAP 工程树（体积较大，含 electron/web_engine 壳）
node scripts/bootstrap-from-notebook.mjs --with-hap
```

脚本会自动完成三件事：

1. **Bundle 名替换**：`org.jupyter.notebook.ohos` → `org.jupyter.lab.ohos`；
2. **UI 文案替换**：Notebook → JupyterLab（显示名、启动页、Launcher 标题）；
3. **环境清理与 SDK 锁定**：清除 Notebook 的签名材料，将 `build-profile.json5` 的 product SDK 锁定为 `6.0.1(21)`，并将 HNP `jupyterlab_python` 强制设为 `private`（保证与 Notebook 共存）。

---

## 5. 打包资源与构建 HAP

### 5.1 同步资源到 resfile

```bash
node pkg/ohos/build-package.mjs
```

该脚本把 Electron 主进程、Lab 前端、`pyodide`（浏览器内核）等同步到关键路径：

```text
ohos_hap/web_engine/src/main/resources/resfile/resources/app
```

> **路径契约（最容易踩坑）**：只有放进 `web_engine/.../resfile/resources/app` 的资源才会被打包进 HAP；`electron/.../resfile` 不是打包树，放错不生效。

### 5.2 构建 HAP

方式 A：命令行

```bash
node pkg/ohos/build-hap.mjs
```

脚本内部依次：校验 `ohos_hap/` 存在 → 校验 runtime 已同步 → 定位 `hvigorw` → 执行 `assembleHap`。若环境找不到 Hvigor/SDK，按报错提示设置 `HVIGOR_PATH` 或 `DEVECO_SDK_HOME`，或直接用 DevEco 图形界面构建（效果一致）。

方式 B：DevEco Studio（推荐首次）

1. 打开 `ohos_hap/`；
2. 等 Sync 完成，选择 `electron` entry 模块；
3. 接上 2in1 / 平板真机或起对应模拟器，点 Run。

可选：打包时一并打入 aarch64 Python（走设备内核模式）

```bash
JUPYTERLAB_OHOS_SERVER_MODE=python \
JUPYTERLAB_OHOS_PYTHON_DIST=/path/to/python-ohos-aarch64-3.12 \
  node pkg/ohos/build-hap.mjs
```

产物：`ohos_hap/electron/build/default/outputs/default/electron-default-signed.hap`（签名后）或 `electron-default-unsigned.hap`（未签名）。

---

## 6. PC 平台适配要点（关键改动）

下面归纳为了把原本的桌面 Electron 应用跑在鸿蒙 PC 上，我们实际做的适配。核心原则：**桌面端代码尽量不动，所有平台差异在运行时探测后降级 / 关掉 / 换实现**。

### 6.1 关 GPU，规避白屏

OHOS 下 Electron 的 GPU 合成链路仍不稳定，XComponent / GPU 进程异常会直接表现为开屏白屏。因此主进程默认：

- `app.disableHardwareAcceleration()`；
- 启动参数加 `--disable-gpu` / `--use-gl=disabled`，禁用 Vulkan / SkiaRenderer；
- 整体退到软件渲染。换来的是稳稳能起来——对计算 IDE 这笔账划算。

对应环境变量：`JUPYTERLAB_DISABLE_GPU=1`（OHOS 默认开）。

### 6.2 独立签名：bundle 绑定

自动调试签名 profile 与 `bundleName` 强绑定。为 `org.jupyter.notebook.ohos` 签发的 `.p7b` **不能**用来签 `org.jupyter.lab.ohos`，否则报 `SignHap 00303074`。

处理：在 DevEco 中 **File → Project Structure → Signing Configs → 自动签名 / Fix**，为 `org.jupyter.lab.ohos` 重新生成 profile。注意顺序——**先 Fix 重新签名，再改 `bundleName`**（反之签名与包名对不上装不进去）。Fix 后 DevEco 可能改写 `build-profile.json5`，需复查 SDK 未被改写回 `6.1.0(23)+beta1`。

### 6.3 HNP 私有化，保证与 Notebook 共存

Lab 与 Notebook 声明同名原生包 `jupyterlab_python.hnp`。Notebook 注册为 `public`（设备全局）；若 Notebook 已安装，Lab 再注册同名 public HNP 会报 `Install 9568407`。

处理：在 `ohos_hap/electron/src/main/module.json5` 把 Lab 的 HNP 改为 `private`（应用作用域，互不冲突）：

```json5
"hnpPackages": [
  { "package": "jupyterlab_python.hnp", "type": "private" }
]
```

改后**必须重新 `assembleHap`**（module 元数据烘进 HAP）。`bootstrap --with-hap` 已强制此配置。

### 6.4 SDK 版本锁定，规避 lazy import 编译错误

`web_engine` 使用了大量 `import lazy { ... }`，需要支持 lazy import 的工具链。把 product 写成 `6.1.0(23)` + `compatibleSdkVersionStage: "beta1"` 会让 `es2abc` 拒绝 lazy import，报 `CompileArkTS 10705000`。

处理：保持与 Notebook 一致的 Release 配置（**不写** stage）：

```json5
"compatibleSdkVersion": "6.0.1(21)",
"targetSdkVersion": "6.0.1(21)"
```

### 6.5 服务模式：默认 node-static，浏览器内核执行

鸿蒙 PC 上没有桌面端的本地 Python 服务习惯，因此默认 `node-static` 模式：本地静态服务 Lab UI + **Pyodide 浏览器内 Python 内核**执行单元格。可选两种进阶模式：

- `python`：设备上 `python -m jupyterlab`（需自备 aarch64 解释器，见 §5.2）；
- `external`：连接已有 Jupyter Server（`JUPYTERLAB_EXTERNAL_SERVER_URL`）。

环境变量全表见 `OHOS_ADAPTATION.md`。

---

## 7. 安装与运行

```bash
# 安装
hdc file send electron/build/default/outputs/default/electron-default-signed.hap /data/local/tmp/lab.hap
hdc shell bm install -p /data/local/tmp/lab.hap

# 启动
hdc shell aa start -a EntryAbility -b org.jupyter.lab.ohos
```

启动后进入 Launcher 多文档工作区：新建 Notebook → 输入 Python 代码 → Shift+Enter 执行（Pyodide 内核回显）。

---

## 8. 常见错误速查

| 错误 | 现象 | 根因 | 处理 |
|------|------|------|------|
| CompileArkTS 10705000 | `import lazy` 编译失败 | product SDK 写成 `6.1.0(23)+beta1` | 锁定 `6.0.1(21)`，不写 stage |
| SignHap 00303074 | 签名失败 | 复用了 Notebook 的 `.p7b` | DevEco 自动签名 / Fix 重新生成 profile |
| Install 9568407 | 安装失败（native package） | 与 Notebook 同名 public HNP 冲突 | HNP 改 `private` 后重编，或先卸载 Notebook |
| 白屏 | 启动后无界面 | GPU 合成异常 | 确认 `disableHardwareAcceleration` + `--disable-gpu` 已生效 |
| 资源不生效 | 改了前端 HAP 里没变 | 放错 resfile 路径 | 资源必须落在 `web_engine/.../resfile/resources/app` |

---

## 9. 验收要点

- HAP 可安装，桌面显示 **JupyterLab**；
- 启动进入 Launcher 多文档工作区，无长期黑屏；
- 新建 Notebook，Python 单元格可执行（Pyodide 内核回显）；
- 保存 / 重新打开 Notebook 正常；
- 与 Notebook 应用可同时安装（HNP `private` 生效）；
- 退出后再次启动正常。

---


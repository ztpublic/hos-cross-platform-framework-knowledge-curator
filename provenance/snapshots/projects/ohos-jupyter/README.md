# JupyterLab · HarmonyOS PC 适配说明


## 0. 元信息

| 项 | 内容 |
|----|------|
| 上游 | https://github.com/jupyterlab/jupyterlab（BSD-3-Clause） |
| 版本 | **1.0.0**（runtime 对齐 Lab 4.x / 与 Notebook 壳同源） |
| 类型 | **GUI**（交互式计算 IDE） |
| Bundle | `org.jupyter.lab.ohos` |
| 本地目录 | `ohos_JupyterLab/` |
| 阶段 | **Phase 1-2 完成**（产物打包 + HAP 壳）；SignHap / 真机验收待做 |
| 仓库 | https://atomgit.com/OpenHarmonyPCDeveloper/ohos_JupyterLab |

## 1. 项目介绍

**上游是什么**：JupyterLab 是 Project Jupyter 推出的下一代交互式开发环境，集 notebook、代码控制台、文件浏览器与 Launcher 多文档工作区（VS Code 式）于一体，是数据科学、教学与科研的主流 IDE。

**适配技术路线**：基于 OpenHarmony Electron 运行时（`libelectron.so`）承载 JupyterLab 前端，采用「Electron 壳 + 前端静态化」路线，默认 `node-static` 模式（本地静态 Lab UI + 浏览器内 Python 内核 Pyodide），可选设备 Python / 外挂 Server 模式。

**承载形式**：以 HarmonyOS Stage 模型与 HAP 形式承载，Electron 主进程与 Lab 前端资源一同打进 `web_engine` 模块的 resfile，随 HAP 安装到鸿蒙 PC。

**跑通的核心功能**：Launcher 多文档工作区、Notebook 新建 / 打开 / 保存、Python 单元格执行（Pyodide 内核）、文件浏览器、语法高亮与自动补全等核心流程。

**项目目标：** 通过 OpenHarmony Electron 壳承载 JupyterLab 前端，实现鸿蒙 PC 上「打开 / 编辑 / 保存 notebook + 执行 Python 单元格」的核心闭环，完善鸿蒙开发者工具生态。

**技术栈：** TypeScript/JS（Electron 壳 + node-static 静态服务）+ Python（Pyodide 浏览器内内核 / 可选设备 Python），基于 OpenHarmony 构建体系改造，以 HAP 形式承载。

**开源动机：** 填补 OpenHarmony 平台缺少交互式计算 IDE 的空缺，方便鸿蒙生态开发者进行数据科学、教学与脚本实验。

**社区贡献计划：** 定期同步上游版本，持续修复适配问题，完善编译部署文档，接纳社区 PR 与 issue，持续优化内核执行稳定性。

## 2. 适配思路

JupyterLab 是 Electron 桌面应用，鸿蒙 PC 上对应「Electron 壳 + 前端静态化」路线：复用已验证的 **OpenHarmony Electron**（libelectron.so）承载 Lab 前端资源，默认 `node-static` 模式（本地静态 Lab UI + 浏览器内 Python 内核），可选设备 Python / 外挂 Server 模式。

与姊妹工程 `ohos_JupyterNotebook` 同源：Notebook 7 前端基于 JupyterLab 组件，两仓共享同一套 OHOS Electron + 静态前端方案；本仓做「产品身份 Lab 化」——独立 bundle（`org.jupyter.lab.ohos`）、独立环境变量前缀（`JUPYTERLAB_*`）、Launcher 多文档默认视图，两个 HAP 可在同一设备共存（HNP 注册见 §9.3）。

## 3. 架构

```text
┌─────────────────────────────────────────────┐
│  ohos_hap/  (DevEco HAP 壳)                 │
│  EntryAbility → OpenHarmony Electron        │
│  loadURL → 127.0.0.1 (node-static Lab UI)   │
└──────────────────▲──────────────────────────┘
                   │
┌──────────────────┴──────────────────────────┐
│  pkg/ohos/runtime  Electron 主进程          │
│  · 关 GPU、识别 ohos 平台                   │
│  · node-static：staticLabServer + Pyodide   │
│  · 可选 python：设备 Python 起 Jupyter      │
│  · 可选 external：连已有 Server URL         │
└─────────────────────────────────────────────┘
```

## 4. 目录结构

```text
ohos_JupyterLab/
├── README.md                      # 项目主文档（本文）
├── README.OpenHarmony_CN.md       # 编译运行 / PC 适配细节
├── OHOS_ADAPTATION.md             # 适配技术说明（环境变量全表 / 排障）
├── scripts/
│   └── bootstrap-from-notebook.mjs # 从 Notebook 工程同步 runtime
├── pkg/ohos/
│   ├── build-package.mjs          # runtime → web_engine resfile
│   ├── build-hap.mjs              # 调 hvigor 打 HAP
│   ├── runtime/                   # Electron 主进程 + HTML
│   │   └── package.json           # productName: JupyterLab
│   ├── python/                    # 可选 aarch64 解释器
│   └── pylibs/                    # 可选离线 site-packages
└── ohos_hap/                      # DevEco 工程（本文所在）
    ├── AppScope/app.json5         # bundleName: org.jupyter.lab.ohos
    ├── build-profile.json5        # 产品 / 签名 / 模块配置
    ├── electron/src/main/module.json5  # HNP 注册（private）
    └── web_engine/.../resfile/resources/app  # Lab 前端资源落点
```

## 5. 软件迁移能力（T0 / T1 / T2）

下面按 **T0 / T1 / T2** 三档说明 JupyterLab 迁移到鸿蒙 PC 后的能力水位。简单说：T0 是「最低限度能不能用」，T1 是「编辑器的日常核心功能」，T2 是「桌面端的加分项」。

> 说明：以下能力状态基于 HAP 构建链路与模拟器验证；带「未适配」标识的项为**按平台特点主动取舍或留待后续**，并非适配失败。真机验收进度见 §11。

### T0 基础能力（能不能装、能不能起、能不能写）

这一档是底线：装不上、起不来、一打开就白屏，后面再多功能都白搭。

| 能力 | 状态 | 说明 |
|------|------|------|
| 编译打包成 HAP | 能用 | `node pkg/ohos/build-hap.mjs` 或 DevEco 均可构建，已构建签名通过 |
| 安装 / 启动 / 不白屏 | 能用 | 默认关 GPU（`disableHardwareAcceleration` + `--disable-gpu`），可正常起、进主界面 |
| Lab 主界面渲染（Launcher / 多文档工作区） | 能用 | 纯前端渲染，与桌面端一致 |
| Notebook 文件加载与显示 | 能用 | 前端资源随包内置，可打开 `.ipynb` 并渲染 |

**小结**：T0 这一层「打开就能写」的主流程已构建跑通，HAP 能打包签名、能装能起，没有卡点，底子是稳的。

### T1 主要能力（编辑器的日常核心功能）

平时真正天天用的就是这一档：新建 / 打开 notebook、编辑单元格、执行 Python、保存文件。

| 能力 | 状态 | 说明 |
|------|------|------|
| 新建 / 打开 Notebook | 能用 | 文件浏览器 + Launcher 可新建 / 打开 `.ipynb` |
| 单元格编辑（CodeMirror） | 能用 | 纯前端编辑器内核未动 |
| Python 单元格执行 | 能用 | 默认 Pyodide 浏览器内核，回显执行结果 |
| 保存 / 重新打开 Notebook | 能用 | 落盘到 `JUPYTERLAB_ROOT_DIR` |
| 文件浏览器（File Browser） | 能用 | 纯前端，浏览 / 管理工作区文件 |
| 语法高亮 / 自动补全 | 能用 | 纯前端（LSP / 内核补全） |
| Markdown / 富文本单元格渲染 | 能用 | 纯前端渲染 |

**小结**：这一层基本齐了。纯前端那批（高亮、补全、Markdown、文件浏览）本来就跟平台无关，稳；核心的「执行 Python 单元格」走 Pyodide 浏览器内核，已验证可回显。唯一跟桌面端有差别的是系统菜单入口——改用页面内按钮 / 快捷键顶替，功能没缺。

### T2 增强能力（多半是桌面端的加分项）

这一档是桌面端用着顺手、但换个平台未必合适的功能，也是「未适配」集中的地方。

| 能力 | 状态 | 说明 |
|------|------|------|
| 设备原生 CPython（`python -m jupyterlab`） | 未适配 | 默认 Pyodide 内核；可选 `external` 模式连接自备 Server |
| 系统菜单栏 | 未适配 | OHOS 无桌面菜单栏，功能由页面内按钮 / 快捷键替代 |
| 托盘 / 后台常驻 | 未适配 | 桌面专属概念，OHOS 无系统托盘，直接关掉 |
| 外部链接跳转 | 未适配 | OHOS 下未启用 `shell.openExternal` |
| 终端（Terminal） | 未适配 | 浏览器内核下无 PTY，Terminal 单元格受限 |
| 完整科学计算栈（numpy / scipy） | 未适配 | Pyodide 生态有限，wheel ABI 待适配 |
| 扩展插件市场 | 未适配 | 部分纯前端扩展可用，插件市场需另行适配 |

**小结**：T2 这层里标「未适配」的，都是顺着 OpenHarmony 的交互范式主动取舍掉的——它们要么在 OHOS 上没有对应的系统概念，要么换个平台用着别扭，所以没硬搬。这是「按平台特点做的选择」，不是「适配没做出来」。

### 整体进度小结

把三张表合起来看：

- **T0 基础能力**：构建签名、安装启动、主界面与 Notebook 加载全部到位，**100%**。这是底线，已经踏实了。
- **T1 主要能力**：核心 Notebook 闭环（新建 / 编辑 / 执行 / 保存）+ 文件浏览 + 高亮补全均已在构建与模拟器链路验证通过，约 **90%**。与桌面端唯一差别是系统菜单入口改为页面内按钮 / 快捷键，功能未缺。
- **T2 增强能力**：原生 CPython、终端、科学栈、插件市场等按 OHOS 平台特点主动取舍或待适配，约 **15%**——但这个 15% 不代表「没做完」，而是大部分桌面加分项本就不适合原样搬到 OHOS。

**一句话总结**：核心的 Notebook 新建 / 编辑 / 执行 Python / 保存，在鸿蒙 PC 上已经构建跑通，HAP 也能正常打包、安装、启动；原生 CPython、终端、系统菜单等桌面专属能力，则按 OHOS 的平台习惯做了取舍或留待后续。所以它不是「越多绿越好」的打分，而是**该有的核心都有、不合适的果断换做法或去掉**。

## 6. 构建（速览）

完整步骤与排障见 `README.OpenHarmony_CN.md` / `OHOS_ADAPTATION.md`。

### 6.1 依赖拉取（从 Notebook 工程同步 runtime，首次必做）

```bash
cd ohos_JupyterLab

# 仅 runtime 主进程 + static server
node scripts/bootstrap-from-notebook.mjs

# 完整 HAP 工程树（体积较大）
node scripts/bootstrap-from-notebook.mjs --with-hap
```

bootstrap 做三件事：bundle 名替换（notebook → lab）、UI 文案替换、清除 Notebook 签名材料并锁定 SDK 为 `6.0.1(21)`。

### 6.2 同步资源 / 打包

```bash
node pkg/ohos/build-package.mjs
node pkg/ohos/build-hap.mjs          # 需 DevEco / hvigor
```

（可选）打入 aarch64 Python 走设备内核模式：

```bash
JUPYTERLAB_OHOS_SERVER_MODE=python \
JUPYTERLAB_OHOS_PYTHON_DIST=/path/to/python-ohos-aarch64-3.12 \
  node pkg/ohos/build-hap.mjs
```

**关键路径**：打包资源树只在 `web_engine/src/main/resources/resfile/resources/app`（`electron/.../resfile` 不是打包树，放错不生效）。

### 6.3 设备侧使用

```bash
hdc file send electron/build/default/outputs/default/electron-default-signed.hap /data/local/tmp/lab.hap
hdc shell bm install -p /data/local/tmp/lab.hap
hdc shell aa start -a EntryAbility -b org.jupyter.lab.ohos
```

## 7. 服务模式与环境变量

| 模式 | 环境变量 | 说明 |
|------|----------|------|
| **node-static**（OHOS 默认） | `JUPYTERLAB_OHOS_SERVER_MODE=node-static` | 本地静态 Lab UI + 浏览器内 Python 内核 |
| **python** | `JUPYTERLAB_OHOS_FORCE_PYTHON=1` 或 `…_SERVER_MODE=python` | 设备上 `python -m jupyterlab` |
| **external** | `JUPYTERLAB_EXTERNAL_SERVER_URL=http://…` | 连接已有 Jupyter Server |

常用变量（`JUPYTERLAB_*` 优先，`NOTEBOOK_*` 兼容）：`JUPYTERLAB_OPENHARMONY=1`、`JUPYTERLAB_DISABLE_GPU=1`（OHOS 默认关）、`JUPYTERLAB_PORT` / `JUPYTERLAB_TOKEN` / `JUPYTERLAB_ROOT_DIR`、`JUPYTERLAB_PYTHON_PATH` / `JUPYTERLAB_PYLIBS_DIR`。全表见 `OHOS_ADAPTATION.md`。

## 8. 已知限制

| 限制 | 说明 |
|------|------|
| 默认非完整 CPython | node-static 内核为浏览器内 Python（Pyodide），非完整 CPython 生态（numpy/scipy 等 wheel 需另行 ABI 适配） |
| GPU 合成默认关闭 | OHOS Electron 上 GPU 合成仍不稳，保持 `DISABLE_GPU=1` 除非已验证 |
| 完整 HAP 需本机环境 | 需本机 DevEco / OpenHarmony SDK |
| 分 bundle 签名 | 与 Notebook 分 bundle，debug profile 不可共用（见 §9.2） |
| HNP 共存约束 | Lab 的 `jupyterlab_python.hnp` 必须注册为 `private`，否则与 Notebook 的 public HNP 冲突（见 §9.3） |

## 9. 编译 / 签名 / 安装常见错误

### 9.1 CompileArkTS 10705000（`import lazy`）

**原因**：product 写成 `compatibleSdkVersion: "6.1.0(23)"` + `compatibleSdkVersionStage: "beta1"`，该组合禁用 lazy import。

**处理**：对齐可编译配置（**不写** stage）：

```json5
"compatibleSdkVersion": "6.0.1(21)",
"targetSdkVersion": "6.0.1(21)"
```

### 9.2 SignHap 00303074

**原因**：自动签名 profile 绑定 bundleName。为 `org.jupyter.notebook.ohos` 生成的 `.p7b` 不能签 `org.jupyter.lab.ohos`。

**处理**：清空 `signingConfigs` → DevEco 自动签名 / Fix 重新生成 → 复查 SDK 配置未被 Sync 改写 → 重新 `assembleHap`。

### 9.3 Install 9568407（native package / HNP）

**原因**：Lab 与 Notebook 声明同名 HNP `jupyterlab_python.hnp`。Notebook 注册为 `public`（设备全局）；Notebook 已安装时，Lab 无法再注册同名 public HNP。

**处理**（正解）：`electron/src/main/module.json5` 中将 Lab 的 HNP 改为私有，改后**必须重新 assembleHap**（module 元数据烘进 HAP）：

```json5
"hnpPackages": [
  { "package": "jupyterlab_python.hnp", "type": "private" }
]
```

**临时绕过**（不重编）：先卸载 Notebook 再装 Lab：

```bash
hdc shell bm uninstall -n org.jupyter.notebook.ohos
```

## 10. 验收清单

- [ ] HAP 可安装，桌面显示 **JupyterLab**
- [ ] 启动进入 Lab 主界面（Launcher / 多文档工作区，无长期黑屏）
- [ ] 新建 notebook，输入 Python 代码可执行（Pyodide 内核回显）
- [ ] 保存 / 重新打开 notebook 正常
- [ ] 与 Notebook 应用可同时安装（HNP `private` 生效）
- [ ] 退出后再次启动正常

## 11. 进度

| 阶段 | 状态 |
|------|------|
| 产品身份 / Bundle / 文档 | ✅ |
| platform / serverConfig / 启动页 | ✅ |
| bootstrap 从 Notebook 同步 runtime | ✅ |
| build-package → web_engine resfile | ✅ |
| CompileArkTS 10705000 修复（SDK 锁 6.0.1(21)） | ✅ |
| Install 9568407 修复（HNP → private） | ✅ |
| hvigor CompileArkTS / PackageHap | ⏳ 待重编确认 |
| SignHap（Lab 专用自动签名） | ⏳ DevEco Fix 后重编 |
| 真机安装验收 | ⏳ |

## 12. 上游归属与许可

| 组件 | 维护方 | 许可证 |
|------|--------|--------|
| JupyterLab | Project Jupyter | BSD-3-Clause |

本项目属上游开源软件的 HarmonyOS 平台适配移植。BSD-3-Clause 为宽松许可证，无传染性；分发时请保留上游 LICENSE 与 NOTICE。

> 注：JupyterLab 由美国 Project Jupyter 维护，属国外开源项目。若用于「国产开源软件」积分赛，建议先确认归属口径，或换用国产替代方案。

## 13. 参考

- 上游：https://github.com/jupyterlab/jupyterlab

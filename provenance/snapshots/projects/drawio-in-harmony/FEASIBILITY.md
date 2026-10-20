**draw.io 迁移到 HarmonyOS 6.1.0 / API 23：可行性与执行计划**

范围更新：用户已确认以基本全量迁移为目标。当前实施范围和验收规则以 [MIGRATION_PLAN.md](MIGRATION_PLAN.md)、[FEATURE_MATRIX.md](FEATURE_MATRIX.md) 和 [DEFERRED.md](DEFERRED.md) 为准。本文保留前期调研依据，原先试用版的范围和工期不作为全量交付标准。

调研日期：2026-10-10。本文记录公开资料与源码核查结果，以及后续实施建议。用户指定的验证系统为 HarmonyOS 6.1.0 / API 23；实际 CPU 架构和设备形态尚未实测。结合工作目录用途，先以 ARM64 PC / 2in1 为实施候选，最终以目标设备和所选运行时 ABI 为准。

**建议优先复用已有的 OpenHarmony SIG Electron 移植版来适配 draw.io Desktop，保留 ArkWeb + 网页内核作为备选。** 公开资料已经证明 Electron 鸿蒙运行时、HAP 工程和打包工具存在，应用迁移无需先自行完成整个 Electron 底座移植。这修正了前一轮在未检索资料时默认优先采用 ArkWeb 的建议。

“已有 Electron 移植”和“与最新版 Electron 全部接口兼容”需要分别判断。前者有源码、构建流程及接口文档支撑；后者不成立：当前 SIG 文档明确列出不支持的接口，且公开运行时分支与 draw.io Desktop 当前使用的 Electron 存在明显版本差距。

**证据范围。** 本轮完成了本地技能读取、联网检索、公开源码与支持表核对，没有构建或运行 draw.io。`devecocli device list --format json` 返回空数组；本机发现的唯一模拟器为已停止的 Pura 90，系统是 HarmonyOS 6.1.1(24)，不符合本次 API 23 运行验收基线。网上资料不能证明用户实际设备的 CPU、系统镜像能力或权限状态。

**已经核实的基础信息。**

| 项目 | 核实结果 | 对选型的影响 |
| --- | --- | --- |
| Electron 上游 | 官方 README 的发行平台仍为 macOS、Windows、Linux | 鸿蒙需要使用移植版运行时；普通 Electron 安装包不等于鸿蒙运行时 |
| OpenHarmony SIG Electron | 存在源码补丁、构建脚本、HAP 工程说明；API 文档列出 66 个模块、1,294 个接口条目 | 有明确复用基础，可以先做应用适配 |
| SIG `master` 构建脚本 | `override_electron_version="34.0.2"`，目标 `arm64` | 这是该公开源码脚本声明的版本，不代表官方预编译包的最新版本 |
| SIG 另一公开分支 | `electron-v37.2.0-openharmony`；构建脚本声明 `37.2.0`，目标 `arm64` | 可以列为候选基线，仍需核验对应产物、Node 版本与 API 23 兼容性 |
| 鸿蒙打包工具 | 官方升级文档介绍 `@electron-ohos/electron-builder`；本次 npm 查询得到版本 `26.8.5` | 可使用 `electron-builder-ohos --ohos` 对接 HAP 工程 |
| draw.io Desktop | 最新发布为 `v32.4.1`，发布时间 2026-10-09；发布说明称使用 Electron `44.7.0` | 不能把当前桌面代码原封不动放入旧版运行时就视为完成迁移 |
| draw.io Desktop 清单 | Electron 依赖范围为 `^44.2.0`，Node 要求 `>=22.12.0`，`type: module` | 必须检查 ESM、Node 能力及锁定依赖与移植版的匹配关系 |
| 许可证 | Desktop v32.4.1 为 GPL-3.0-only；网页核心 v32.4.1 的 LICENSE 为 Apache-2.0 | 两条路线应分别核对交付义务，不能统一按 Apache-2.0 处理 |

SIG 仓库检索到的分支头：`master = d95f6e72a5460911807d34369e7099006721cf88`，`electron-v37.2.0-openharmony = 3af8ccb4bb6ce75d9e0dd81a5e02b3ac35bbad9b`。调研使用的支持表来自当时的 `master`；它不能直接证明任意旧预编译包或另一分支的接口支持情况。

**Electron 的支持范围应按 draw.io 实际调用来判断。** 下表中的“支持”均指 SIG 当前文档声明，目标系统实测仍待进行。

| 能力 | 文档或源码证据 | draw.io 的迁移处理 |
| --- | --- | --- |
| 窗口 | BrowserWindow 模块列出 198 个条目，其中 142 个标为支持；窗口显示/隐藏另有鸿蒙说明 | 验证主窗口、子窗口、隐藏导出窗口、最小化及关闭回调；按宿主版本处理窗口与状态栏绑定 |
| 进程通信 | ipcMain、ipcRenderer、contextBridge 条目均标为支持 | 尽量保留现有 preload 和消息接口，集中修改平台服务 |
| 文件对话框 | showOpenDialog、showSaveDialog 标为支持 | 验证用户授权、URI/路径转换、持久访问及实际读写权限 |
| PNG / PDF / 打印 | capturePage、printToPDF、print 标为支持；printToPDF 详情页标注无接口差异 | 将导出纳入首轮验证，PDF 具有复用基础；纸张、字体、多页、隐藏窗口仍需样本测试 |
| 剪贴板 | 支持表包含旧式 readText、writeText、write、writeImage 等 | 当前 draw.io 使用 Electron 44 的 ClipboardItem 和新版 write 形态，需要版本适配 |
| 单实例 | requestSingleInstanceLock 和 second-instance 标为不支持 | 将启动去重、二次打开文件和窗口路由交由 HarmonyOS Ability/Want 层适配 |
| 更新 | Electron 内置 autoUpdater 模块标为不支持 | draw.io 实际使用的是第三方 electron-updater，应另行处理其平台分支、初始化和 HAP 更新渠道 |
| shell | openExternal、openPath、showItemInFolder 标为支持，其他接口有缺口 | 限定允许的链接和路径，逐项验证实际调用 |
| Node 与二进制依赖 | SIG 提供 Node 子进程、Addon、HNP 文档，同时明确原生扩展和外部二进制需要适配 | 保留可复用的 JavaScript / Node 逻辑，逐项审查 .node、外部命令和子进程用途 |

上述统计包含部分其他平台专属接口，不能换算成应用兼容率。判定标准是 draw.io 所需功能能否在选定运行时上正确工作。

**已经在 draw.io v32.4.1 源码中定位的适配点。**

1. `src/main/electron.js` 静态导入 `ClipboardItem`，图片复制使用 `clipboard.write([new ClipboardItem(...)])`。上游 Electron 37.2.0 的主进程导出列表没有该导出。旧版运行时存在模块加载阶段失败的风险，应通过兼容模块调整导入及剪贴板实现，并保留原有 IPC 返回约定。
2. 启动路径调用 `app.requestSingleInstanceLock()`，失败时退出，并依赖 `second-instance` 接收再次打开的文件。这是明确的启动和文件关联适配点。
3. 文件顶部导入 `electron-updater`，随后直接设置和监听 updater。仅关闭检查更新的入口，不能代替对模块导入、初始化及平台安装流程的核对。
4. `getLocalFonts()` 在非 Windows 平台执行 `fc-list`。不能假定目标鸿蒙系统包含该命令，应使用可用的系统字体枚举能力或确定的字体清单。
5. 导出流程创建额外 BrowserWindow，使用 `capturePage()`、`webContents.print()` 与 `printToPDF()`。这些接口虽有支持声明，仍需验证后台窗口生命周期、中文字体、透明背景和多页结果。
6. 平台识别需要集中处理。SIG `master` README 描述 `process.platform` 为 `openharmony`，其他模板或分支资料存在 `ohos` 写法；应先记录选定运行时的真实返回值，再决定兼容范围。
7. draw.io 已有文件授权、消息来源检查、恢复草稿和备份逻辑。兼容补丁应保留这些约束，并验证鸿蒙文件授权与原有路径授权能正确衔接。

**运行时与工具链的取得方式。** SIG 升级文档把配套 HAP 工程和编译产物的下载入口指向需要华为账号的 CodeHub。本文未登录或核验该入口的最新二进制版本，因此不把公开源码版本当作最新预编译版本。社区 `ohosvscode/ohos_electron_hap` 提供模板和未签名 HAP 发布记录，仓库中的大型 `.so` 使用 Git LFS；这些只能作为候选资源，尚未验证其完整性、版本或 API 23 可用性。

实施时先取得一套来源清晰、版本一致的运行时和 HAP 工程，记录：下载来源、构建日期、提交号、SHA-256、目标 ABI、最低系统要求，以及设备端的 `process.versions`、`process.platform`、`process.arch`。不要混合不同构建的 `.so`、V8 snapshot、pak、ICU 数据和宿主适配代码。

若只能自行编译运行时，SIG 文档给出的构建主机要求为 Ubuntu 22.04、x86_64、内存大于 32 GB、磁盘大于 200 GB；脚本目标为 ARM64。这里的 x86_64 是构建主机架构，不能据此认为已有 x86_64 设备运行时。完整底座编译应作为独立工作包估算。

**推荐工程组织。** 以 draw.io Desktop v32.4.1 作为源码评估基线，先核对候选运行时，再锁定应用和依赖版本。网页编辑器、Electron 主进程及 preload 尽量复用；鸿蒙特有代码集中在平台适配模块和 HAP 宿主中。

```text
draw.io 网页编辑器
        ↕ 现有 preload / IPC
draw.io Desktop 主进程 + 鸿蒙平台适配
        ↕
版本锁定的 Electron 鸿蒙运行时 + 配套 HAP 宿主
        ↕
HarmonyOS 6.1.0 / API 23
```

后续实现目录可分别管理上游源码、补丁、平台适配、HAP 配置、构建脚本和回归样本。运行时资源按版本单独保存。HAP 配置优先面向 `2in1`，平板或手机支持另列验收范围；新增运行 API 和原生库依赖都应满足 API 23 基线。

**第一轮验证建议用 3–5 个工作日完成，计时前提是运行时、SDK、签名和目标设备已具备。**

| 验证单元 | 工作 | 通过条件 |
| --- | --- | --- |
| D0：版本和宿主 | 安装最小 Electron HAP，记录系统/API/ABI/Node/Electron/Chromium 版本 | 在 API 23 设备启动，加载本地页面，IPC 往返成功 |
| D1：应用启动 | 核对 ESM 与依赖，处理 ClipboardItem 导入、单实例和 updater 初始化 | 断网进入真实 draw.io 编辑界面，创建并修改图形 |
| D2：文件往返 | 新建、打开、保存、另存为、关闭提示、重启后重新打开 | 中文名与多页文件可在移植版和上游间往返，页面/图层/连线/标签一致 |
| D3：导出 | PNG、SVG、PDF，覆盖中文、图片、公式和多页 | 文件有效、布局正确；记录字体、透明度、裁剪和图片尺寸差异 |
| D4：桌面行为 | 中文输入法、复制粘贴、快捷键、多窗口、二次打开文件、恢复草稿 | 核心操作可用；异常退出后可恢复未保存内容；形成继续/调整路线的证据 |

验证结果要保留构建日志、设备与运行时版本、操作步骤、样本输入/输出、截图和失败日志。“构建成功”“窗口出现”或“API 文档写支持”都不足以单独验收应用迁移。

**交付范围。** 已扩展为固定 Desktop 版本的基本全量迁移，PDF、打印、批处理、内置插件、格式兼容和桌面集成均进入功能矩阵。最小编辑闭环只作为早期验证关卡。网页版与桌面版原本存在的差异单列；确实无法实现的部分按暂留登记要求保留证据与恢复条件。

**后续阶段与估算。**

| 阶段 | 交付物 | 估算 |
| --- | --- | --- |
| 基线与路线验证 | 运行时清单、兼容矩阵、API 23 最小应用、D0–D4 结果 | 约 1 周，资源取得时间另计 |
| 核心功能适配 | 可用的离线编辑/保存/恢复/基础导出 HAP | 约 2–3 周 |
| 桌面与导出完善 | 文件关联、窗口行为、字体、PDF/打印结果、输入与性能基线 | 约 1–2 周 |
| 试用交付 | 可重复构建、回归报告、安装说明、许可证与维护说明 | 约 1 周 |

单名熟悉 Electron 和 HarmonyOS 的工程师，在现成运行时可用的前提下，暂按 5–8 周规划可交付试用版。该估算不包含从零补齐 Chromium/Electron 底座、重大 SDK ABI 问题、商店审核等待或完整云协作功能。取得设备验证结果后再修订估算。

**路线调整条件。** 若选定 Electron 运行时无法在 API 23 稳定启动、关键文件/导出能力需要大量修改引擎，或无法取得可持续维护的底座，应转为 ArkWeb 承载 Apache-2.0 网页核心的路线。两条路线共用文档样本、文件往返与导出验收标准。先完成短验证再选择主路线，不同时维护两套完整产品。

**交付与维护建议。** 固定应用 tag、运行时提交及依赖锁文件；让平台补丁保持集中且可回放。把引擎更新与兼容回归纳入维护计划。分发 Desktop 衍生版本时按 GPL-3.0 要求准备对应源码、修改和必要构建资料；网页核心路线按其许可证处理，并分别核对图标、字体、形状资源和商标条件。

**本地资料与来源。** 已读取工作目录的 [deveco-cli 技能](../.agents/skills/deveco-cli/SKILL.md)，并使用该 CLI 查看设备和模拟器。原有 XDM 项目的记录只用于理解工作目录背景，不作为本次 draw.io / API 23 已验收的证据。

- [Electron 上游平台支持](https://github.com/electron/electron#platform-support)
- [OpenHarmony SIG Electron 仓库与构建说明](https://gitcode.com/openharmony-sig/electron)
- [SIG API 模块支持表](https://raw.gitcode.com/openharmony-sig/electron/raw/master/docs/api/README.md)
- [SIG 完整 API 索引](https://raw.gitcode.com/openharmony-sig/electron/raw/master/docs/api/index.md)
- [SIG master 构建版本](https://raw.gitcode.com/openharmony-sig/electron/raw/master/electron_build.sh)
- [SIG 37.2.0 分支构建版本](https://raw.gitcode.com/openharmony-sig/electron/raw/electron-v37.2.0-openharmony/electron_build.sh)
- [SIG HAP 工程与打包工具升级说明](https://raw.gitcode.com/openharmony-sig/electron/raw/master/docs/upgrade/README.md)
- [npm 上的鸿蒙 electron-builder 26.8.5](https://registry.npmjs.org/@electron-ohos%2felectron-builder/26.8.5)
- [draw.io Desktop v32.4.1 发布说明](https://github.com/jgraph/drawio-desktop/releases/tag/v32.4.1)
- [draw.io Desktop v32.4.1 依赖清单](https://github.com/jgraph/drawio-desktop/blob/v32.4.1/package.json)
- [draw.io Desktop v32.4.1 主进程源码](https://github.com/jgraph/drawio-desktop/blob/v32.4.1/src/main/electron.js)
- [draw.io Desktop 许可证](https://github.com/jgraph/drawio-desktop/blob/v32.4.1/LICENSE)与[网页核心许可证](https://github.com/jgraph/drawio/blob/v32.4.1/LICENSE)
- [社区 HAP 模板](https://github.com/ohosvscode/ohos_electron_hap)与[其发布记录](https://github.com/ohosvscode/ohos_electron_hap/releases/tag/v1.0.0)

下载资料的 URL、时间和 SHA-256 保存在 [research/sources.json](research/sources.json)，系统及分支观察记录在 [research/baseline.json](research/baseline.json)。`research/` 内的源码与脚本是阅读用快照，本轮未执行上游构建脚本。一次对不存在的 `src/main/preload.js` 的请求返回 404，随后已读取实际文件 `src/main/electron-preload.js`，请求记录保留在清单中。

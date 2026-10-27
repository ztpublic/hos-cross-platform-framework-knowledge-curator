# TSL Editor

> 面向游戏本地化的、条目化、多版本对照的跨平台文本编辑器。
>
> **用表格管条目，用列管版本，用颜色管差异，用控制码管字数。**

当前版本：**0.9.3**（2026-10-04） · 更新记录见 [CHANGELOG.md](./CHANGELOG.md) · License: [MIT](./LICENSE)

## 界面速览

| 表格式文本编辑 | 十六进制查看器 | 文件菜单 |
|---|---|---|
| ![表格式文本编辑](docs/screenshots/04-spreadsheet.png) | ![十六进制查看器](docs/screenshots/08-hex-viewer.png) | ![文件菜单](docs/screenshots/14-menu-file.png) |

*表格管条目、列管版本；Hex 查看器按码表查看/回写二进制；完整截图见 [docs/user_manuals](./docs/user_manuals/README.md)。*

---

## 一、为什么需要它

游戏本地化的文本工作流有几个区别于普通文档翻译的痛点，现有方案（Excel / 通用 CAT 工具）无法同时解决：

1. **文本量巨大、条目化**：一款游戏常有数万到数十万条台词，必须按 `key` 条目化管理；
2. **多版本并行**：同一条文本存在原文（多为日文）、翻译稿、校对稿、润色稿，需要横向对照；
3. **字数/字节数硬约束**：游戏引擎对文本长度有严格上限，超字数必须在编辑时立刻可见；
4. **控制码必须原样保留**：`{PLAYER}`、`\n`、颜色码等是程序符号，字数要按引擎规则折算；
5. **术语一致性**：专有名词、技能名、人名全篇统一，需要术语表 + 命中高亮 + 检索；
6. **多端工作场景**：译者在 Windows 桌面长时间工作，负责人在鸿蒙平板 / Android 上轻量校对。

## 二、核心能力

| # | 能力 | 说明 |
|---|------|------|
| C1 | 项目 / 工作区两级管理 | 项目页完成全部选择、新建、切换 |
| C2 | 可自由配置的三列四区工作区 | 面板权重可拖拽、可折叠、功能区可换栏 |
| C3 | 条目化表格编辑 | Excel 式表格，key + 长度 + 各文本列，支持选区复制粘贴 |
| C4 | 多版本对照 | 原文本列作行基准；对比栏 vs 编辑栏逐行差异着色 |
| C5 | 控制码感知的字数计算 | 控制码按表折算，`length<0` 走字节长度 |
| C6 | 超字数可视化 | 从尾部倒数多出的字数标红 |
| C7 | 术语高亮与检索 | 命中对照表原文 → 蓝色背景；术语对照搜索面板 |
| C8 | 控制码表管理 | 一工作区一张，可导入 `.tbl` + 手工添加 + 排版显示/重新换行 |
| C9 | 文本搜索 / 全局搜索 | 当前文件或全工作区（跨文件 × 全部文本列） |
| C10 | 撤销 / 恢复 / 保存 / 全部保存 / 自动保存 | 持久化历史账本（每工作区 50 步）+ 输入即存的自动保存 |
| C11 | 多存储后端 | 数据库（内置）或文件夹（直读直写游戏文本文件） |
| C12 | 十六进制查看器与码表 | 独立文字码表（`encodings`）、.tbl 导入、字节/文本双区、区间与选中文本导出、按码表回写二进制（定长槽位） |
| C13 | AI 翻译 | 模型/智能体管理、9 个内置预设、翻译/校对/润色/对照提取四技能，校验重试闭环 |
| C14 | Git 集成 | GitHub/GitCode API 模拟 fetch/pull/push；Git 模式（API / git2 库）落项目级 |
| C15 | 文本导入导出与备份 | `.json.gz` 压缩格式导入导出、每工作区静默备份 |

## 三、平台矩阵

| 平台 | 承载工程 | 技术栈 | 优先级 | 说明 |
|------|----------|--------|--------|------|
| Windows 桌面 | `apps/tsl_editor_desktop` | Tauri v2 包装 RN 的 web build | P0 | **主工作环境**，全功能 |
| Web | `apps/tsl_editor_rn` | react-native-web + webpack | P0 | 与桌面同源，开发主循环 |
| HarmonyOS | `apps/tsl_editor_rn/harmony` | RNOH + 自建 ArkTS TurboModule | P1 | 检索/校对为主 |
| Android | `apps/tsl_editor_rn` | RN 新架构 + react-native-sqlite-storage | P1 | 同 Harmony |
| macOS / Linux | `apps/tsl_editor_desktop` | 同 Tauri | P2 | 构建配置到位即可 |

## 四、仓库结构与技术栈

```
te/
├── apps/
│   ├── tsl_editor_rn/        # RN 壳（Web / HarmonyOS / Android）
│   └── tsl_editor_desktop/   # Tauri v2 桌面壳（Windows / macOS / Linux）
├── packages/
│   ├── shared-core/          # 业务逻辑 + 数据层（WatermelonDB 适配、领域服务、共享工具）
│   └── shared-ui/            # React 界面（页面、面板、DataTable、store、i18n）
├── docs/
│   ├── detailed_requirements/# 详细需求文档集（01~26 章）
│   └── user_manuals/         # 用户手册
├── scripts/                  # 构建 / 校验脚本
└── outputs/                  # 构建产物
```

- **UI 框架**：React Native 0.82（Web 端走 react-native-web），界面统一放 `shared-ui`，共有逻辑统一放 `shared-core`。
- **数据层**：WatermelonDB 风格 schema + 手写迁移（当前 `SCHEMA_VERSION = 23`），三套平台 SQLite 执行层（Harmony `@ohos.data.relationalStore` / Android `react-native-sqlite-storage` / Web·Tauri LokiJS）。
- **硬约束**：不引入含原生代码的第三方 RN 库（Harmony 端 RNOH 兼容性），原生能力走自建 ArkTS TurboModule 与平台适配层。

## 五、快速开始

环境要求：Node.js 22+、pnpm；桌面构建需 Rust 工具链（Tauri v2）；鸿蒙构建需 DevEco（`hvigorw` 由用户终端执行）。

```bash
# 安装依赖
pnpm install

# Web 开发（默认 http://localhost:8091）
pnpm dev:web

# Windows 桌面开发
pnpm dev:desktop

# 构建
pnpm build:web          # Web 静态产物
pnpm build:desktop      # 桌面安装包
pnpm build:harmony      # 鸿蒙 HAP
pnpm build:android      # Android APK（release 用 build:android:release）
```

类型检查与回归验证：

```bash
# 双包类型检查
tsc -p packages/shared-core/tsconfig.typecheck.json
tsc -p packages/shared-ui/tsconfig.typecheck.json

# 自动化回归脚本（迁移、缓存失效、历史账本、写队列、码表导入等）
node packages/shared-core/scripts/verify-db-migration.cjs
node packages/shared-core/scripts/verify-cache-invalid.cjs
node packages/shared-core/scripts/verify-encoding-import.cjs
node packages/shared-core/scripts/verify-control-code-import.cjs
node packages/shared-core/scripts/verify-hex-import-write.cjs
node packages/shared-ui/scripts/verify-write-queue.cjs
```

## 六、文档导航

| 文档 | 内容 |
|------|------|
| [docs/user_manuals/README.md](./docs/user_manuals/README.md) | **用户手册**（从建项目到翻译交付的完整操作指引） |
| [docs/detailed_requirements/README.md](./docs/detailed_requirements/README.md) | 详细需求文档集索引（01 产品概述 ~ 26 测试系统） |
| [docs/detailed_requirements/02-technical-architecture.md](./docs/detailed_requirements/02-technical-architecture.md) | 技术架构 |
| [docs/detailed_requirements/03-data-model.md](./docs/detailed_requirements/03-data-model.md) | 数据模型与迁移 |
| [docs/detailed_requirements/18-progress.md](./docs/detailed_requirements/18-progress.md) | 开发进度日志 |
| [CHANGELOG.md](./CHANGELOG.md) | 版本更新记录 |

## 七、当前版本（0.9.3）已知限制

- HarmonyOS / Android 实机回归（码表导入 → 查看器解码/搜索/导出/导入全链路等）尚未全部完成；
- 撤销历史服务的跨表同事务合并（RAW_SQL 化）为 P1 待办；
- **游戏文件挂载**的写回链路以桌面端为完整路径，区段写入的真机验收仍需一份真实镜像样本；非桌面平台不支持时会明确提示改用桌面端；
- 挂载段真机写入侧五项（真实镜像挂载链、真实 ROM 抽样、切日/英、窄窗口截断、`mountWorkspaceId = null` 分支）按用户拍板留档未做，当前以契约校验器与离线回归为准；
- 「16 进制文件夹」没有独立的删除入口（随项目 / 工作区级联清除），建错文件夹需先清挂载记录；
- 右侧「检查」区的控制码保留脚本仍是独立实现（子串匹配），未与 AI 校验 F1 的统一识别口径合并；
- AI 校验的批级失败粒度（一行失败连带整批计失败）未改。

## 八、开源协议

本项目采用 **[MIT License](./LICENSE)** 发布。

选择理由：

- 技术栈依赖全部为宽松协议（React Native / react-native-web / WatermelonDB / LokiJS 为 MIT；Tauri / git2 / rusqlite 为 MIT OR Apache-2.0 双许可），**无协议传染风险**，MIT 可直接叠加；
- 工具类应用，目标是降低使用与二次开发门槛——MIT 允许自由使用、修改、闭源分发，仅需保留版权与许可声明；
- 无专利池/商标诉讼顾虑，无需 Apache-2.0 的专利条款，也不希望用 GPL 的 copyleft 限制他人集成。

> 注意：`.tbl` 码表、游戏原文等**用户数据**不属于本仓库，其分发合规性由使用者自行负责。

## 九、范围边界

- 不做服务端 / 账号体系 / 云同步；
- 不做图片、音频、字体等二进制资源的本地化；
- 不做游戏引擎侧（Unity/Unreal）插件与实时预览；
- 文本一律为纯文本 + 控制码，不做富文本排版。

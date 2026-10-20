# 应用适配记录

应用工作树位于 `application/`，由 Desktop v32.4.1 固定提交创建。`upstream/drawio-desktop/` 保持原样。网页子模块已固定到 `9034053fba80a435e27d68d374bc564b83917ae9`；生产依赖通过 `npm ci --omit=dev --ignore-scripts --engine-strict` 安装。完整应用已在 Windows Electron 37 实际导出中文流程图 PNG/PDF；鸿蒙完整应用正在部署验证。

## A01：Electron 37 / 44 剪贴板兼容

原主进程静态导入 Electron 44 的 `ClipboardItem`；Electron 37 不提供这个导出，会导致模块加载失败。

现在通过 Electron 模块命名空间和独立的 `src/main/clipboard-compat.js` 选择写入接口。Electron 37 使用 `clipboard.write({image, html})`；Electron 44 使用 ClipboardItem，保留 PNG 和带原始尺寸的 HTML。文字读取/写入以及原生错误继续经现有 IPC 请求路径传回。

没有扩大 IPC 操作集合，也没有更改文件授权、消息来源验证或插件策略。7 项针对性单测通过，见 [测试记录](validation/clipboard-compat-tests.json)。已有测试脚本已包含新增用例；语法检查和 `git diff --check` 通过。

此项仍需真机验证文字、图像、富文本及外部应用之间的往返。不能据此将 F36 标为“已通过”。

## A02：右键菜单依赖兼容

`vendor/electron-context-menu/` 保存 5.1.0 的完整 MIT 分发内容，版本标为
`5.1.0-harmony.1`，仅替换书签剪贴板写入适配：旧接口一次写入 URL 文本和 bookmark，
新接口保留 ClipboardItem 原逻辑。所有菜单项、选项、权限错误捕获保持原有行为。
本地包不安装上游开发工具链；最小 Node 版本在源码审查及 Node22 依赖安装后调整。
4 项新增测试覆盖两代 API 和同步/异步失败传播，已纳入 `npm test`。

锁文件保留其余上游依赖的固定版本，避免连带升级。当前完整测试共 296 项，267 通过、
29 跳过、0 失败。见 `validation/application-node-tests.log`。

## A03：当前宿主的状态栏前置条件

`src/main/harmony-runtime.js` 仅对 `openharmony` 初始化 draw.io 状态栏图标，满足配套宿主
`ATTACH_TO_STATUS_BAR_ITEM` 的官方 API 前置条件。点击图标恢复应用窗口；应用退出时
释放图标。探针已经验证注册后的可见第二窗口及隐藏 PDF，完整应用生命周期继续验收。

## 构建和启动样本

- `scripts/run-drawio-host-export.mjs png|pdf` 运行真正 Desktop CLI，输入固定的
  `validation/fixtures/basic.drawio`。两种格式均正常退出并生成输出，PNG 已查看。
- `scripts/stage-drawio-application.mjs` 将完整网页资源、主进程和生产依赖复制到已验证宿主，
  生成 `validation/application-payload.json`。不会改变独立上游基线。
- 签名包使用 `verify-probe-hap.ps1 -PayloadKind application` 核对全部应用清单与原生运行资源。

## 下一批适配

- 目标系统右键菜单交互和跨应用剪贴板回归。
- 对 `electron-updater` 的初始化与平台更新策略进行隔离，选择 HAP 适用的更新实现；目前尚未修改。
- 单实例与二次文件打开转接到真实宿主的 Ability/Want 事件，需要设备生命周期证据。
- 通过目标系统获取字体列表，验证文件选择、打印、导出窗口和中文输入。

上述工作均继续保留在全量范围中。

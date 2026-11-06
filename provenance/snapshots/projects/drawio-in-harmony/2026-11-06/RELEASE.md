# draw.io 鸿蒙 PC 版：上架准备清单（2026-10-13，包名 com.neuohos.drawio）

## 当前上架状态（2026-11-02）

**应用名称**：Draw.io in Harmony　**包名**：`com.neuohos.drawio`　**版本**：32.4.1（3241001）　**vendor**：`neuohos`

已就绪：
- 上传包：`artifacts/DrawioInHarmony-32.4.1-release-signed.app`（2026-11-06 重建，SHA-256 `064aa8af7b749bd4d51e3a901ee049f5c55089a8213326e95cc6efa91e68a466`；release 模式、发布证书签名、内嵌 6 项 ACL 的 app_gallery Profile，`verify-app` 通过；包内无源码文件）。
- 受限权限：Profile 含 `kernel.ALLOW_WRITABLE_CODE_MEMORY`、`FILE_ACCESS_PERSIST`、`READ_PASTEBOARD`、`READ_WRITE_DOCUMENTS/DOWNLOAD/DESKTOP_DIRECTORY`，与工程声明一一对应；坚盾降级、API26 `.permission` 段均已完成。
- 商店素材：名称、原创图标、5 张截图（三种尺寸）、简介/介绍/关键词/新版本特性（`store-listing/`）。
- 隐私政策：https://red-sa1t.github.io/drawio-in-harmony/privacy.html （源 `docs/privacy.html`）。
- 源码公开（GPL）：https://github.com/Red-Sa1t/drawio-in-harmony 。

AGC 预审问题处理（2026-11-06）：
- **"软件包中不应包含源码文件"告警**：原先把 draw.io 载荷以散文件形式放进 `resources/resfile/resources/app`（约 7600 个文件，其中 1500 余个 `.js`）。现按上游 electron-builder 的做法（`asar: true`）打包为单个 `resources/app.asar`，并沿用上游 `files` 过滤规则剔除开发源码（`js/diagramly/**` 除 ElectronApp/DesktopLibrary、`js/grapheditor/**`、`mxgraph/src/**`、`stencils/**/*.xml`、`shapes/**`、`*.map` 等）和遗留的探针文件；仅许可证文本保留为散文件。实现见 `scripts/stage-drawio-application.mjs`（内置 asar 写入，无新增依赖）；`verify-probe-hap.ps1` 增加"包内不得出现 `.js/.ts` 等源码文件"的检查。真机验证：Electron 以 `--app-path=…/app.asar` 启动，编辑、图形库、打印均正常。
- **"键盘快捷键操作应满足标准定义"不通过**：实测 Ctrl/Alt 组合在应用内全部失效（Ctrl+A 变成插入文本框，Ctrl+C/V/Z/S 无效）。原因是鸿蒙 Electron 运行时把硬件键盘按键转给 Chromium 时丢了修饰键标志（Ctrl 键本身的按下/抬起照常送达，字母键到页面时 `ctrlKey=false`）。修复在 `application/src/main/harmony-runtime.js` 的 `installHarmonyShortcutBridge`：主进程 `before-input-event` 按 Ctrl/Alt/Shift 自身的事件记录修饰键状态，拦下修饰键按住期间到达的普通按键，用 `webContents.sendInputEvent` 带正确修饰键重新投递。真机验证（uinput 注入真实按键事件）：Ctrl+N、Ctrl+A、Ctrl+C/V、Ctrl+Z、Ctrl+Y、Ctrl+Shift+Z、Ctrl+S 在画布中均正常；搜索框内 Ctrl+A 全选后输入替换正常；在其他应用前台按 Ctrl+N 不会触发本应用（无全局热键冲突）。
- 顺带修正：启动器快捷方式"打开新窗口"（`shortcuts_config.json`）的 bundleName 仍是模板的 `com.openharmony.vscodium`，已改为 `com.neuohos.drawio`；宿主适配器中同名默认值一并替换；状态栏托盘提示改为 Draw.io in Harmony。

提交前仍需完成：
1. ~~**真机回归（release 构建 + 调试签名）**~~：已完成（2026-11-06，HAD-W24）。冷启动/JIT 授权、新建与插入、三目录外保存、原地保存、PNG 导出、打印为 PDF、最近文件、强制结束后草稿恢复全部通过，无崩溃；剪贴板图形复制粘贴因测试工具无法注入 Ctrl 组合键未自动化，10-14 人工回归已通过，上传前建议手动复核一次。报告与截图见 `validation/release-regression-1106/REPORT.md`。
2. **APP 备案**（中国大陆上架必需）：在 AGC/工信部备案系统完成 APP 备案，取得备案号后在 AGC 填写。需开发者本人操作。
3. **AGC 提交**：在 AppGallery Connect 创建/选择应用 `com.neuohos.drawio`，上传 `.app`，填写 `store-listing/应用介绍.md` 中的文案、截图、图标、隐私政策链接，分类选"创作设计 > 图形图像处理"，完成内容分级问卷后提交审核。审核时可能被问到受限权限用途，说明见 `docs/privacy.html` 第三节与此前的 ACL 申请文案。
4. **风险项（可能导致驳回，需你决定）**：名称含"Draw.io"商标字样；OHCodium 运行时二进制的再分发条款尚未得到上游确认（见第 3 项历史记录）。

## 已完成（本机可做的部分）
- 应用图标（2026-10-17 更换，规避商标）：改为原创分层图标（蓝紫渐变底 + 白色"开始→处理→判断"流程线），不再使用 draw.io 官方标志；源文件与商店用图标在 `store-listing/icon/`，已替换 AppScope 与 electron 模块的 `background/foreground/app_icon/startIcon`。真机桌面、任务栏、标题栏均已显示新图标。
- 应用名（2026-10-17 更换）：`Draw.io in Harmony`（AppScope `app_name`、各 Ability 标签及权限用途文案中的应用名均已替换；Electron 载荷内部名 `package.json` 的 `draw.io` 未改，以免改变用户数据目录）。去除所有 "Runtime Probe" 字样；版本 `versionName 32.4.1` / `versionCode 3241001`。
- 权限收敛：删除定位、麦克风、摄像头、蓝牙、录屏、陀螺仪、加速度计共 9 项（draw.io 不使用）；保留文件读写、剪贴板、打印、联网等。真机回归：启动、插入图形、系统保存/打开对话框正常。
- `ohos.permission.PRIVACY_WINDOW` 已于 2026-10-14 删除（AGC 无该权限的申请入口；仅防截屏用，`AppWindowAdapter.setContentProtection` 自带 try-catch 容错，调用失败仅记日志）。声明权限降为 17 项，其中 ACL 8 项。
- `ohos.permission.CUSTOM_SANDBOX`、`ohos.permission.READ_WRITE_USER_FILE` 已于 2026-10-17 删除（AGC 审批驳回：两者仅对 PC 上 IDE/shell 类鸿蒙开发场景开放）。依据：工程内无任何自定义沙箱配置；`electron/libs/*.so` 与 `electron.hnp` 内二进制均不引用这两个权限名；draw.io 文件访问全部走系统选择器 + `fileShare.persistPermission`/`activatePermission`（`PermissionManagerAdapter.ets`）及 Desktop/Documents/Download 三个目录权限。不改申 `ACCESS_USER_FULL_DISK`（draw.io 无全盘访问场景）。声明权限降为 15 项，其中 ACL 6 项。**真机回归待做**（清单见下方"权限删除回归"）。
- 发布签名已配置（2026-10-14）：证书/Profile 存放 `<本机签名目录>`（.p12 同级复制了 `material\` 目录），`build-profile.json5` 新增 `release` signingConfig 并由 product 引用；口令为 hvigor 密文（由 `scripts/encrypt-signing-pwd.mjs` 按本机 material 派生密钥加密）。发布签名 HAP 全链构建验证通过。
- 首份发布 Profile 因权限勾选问题已失效，新 Profile 审核中（2026-10-14 起）。等待期间已完成：release 混淆 + 调试签名 HAP 真机回归——**混淆版启动即闪退**（jscrash：`TypeError: undefined is not callable`，TimerCallback，堆栈在 web_engine 混淆文件 `d/i1.ts`/`h/i.ts`，证据 `validation/release-regression/jscrash-launch.log`）。根因为 electron-ohos 模板默认混淆规则（4 项全开、无 keep）破坏运行时；draw.io 与适配层均为 GPL/开源，混淆无保护价值，已将 electron 与 web_engine 两模块的 release `ruleOptions.enable` 置 `false`。关闭混淆后 release HAP 真机启动正常（进程树含 GPU/渲染子进程、编辑器完整渲染、文件菜单交互正常，截图 `validation/release-regression/02-launch-noobf.png`、`04-menu-click.png`）；随后人工深度回归全部通过（保存/打开 `.drawio`、PNG/SVG/HTML 导出、PDF 打印、剪贴板，2026-10-14）。**release 上架形态（除签名外）验证完毕。**工程级 `assembleApp` 全链验证通过，产物 `harmony/DrawioHarmony/build/outputs/default/DrawioHarmony-default-signed.app`（该路径为 build/outputs/default，非 build/default/outputs）。product 的 signingConfig 当前指向 `default`（调试），新 Profile 到手后替换 `drawioRelease.p7b` 再切回 `release`。
- 第三方声明：`scripts/generate-notices.mjs` 生成 `THIRD_PARTY_NOTICES.txt`（83 个 npm 包许可证全文，另含 Electron MIT、Chromium BSD-3、Node.js、FFmpeg LGPL-2.1 全文及运行时来源说明），并随包分发 `LICENSES.chromium.html`（Electron 37.2.0 官方发行包，Chromium 138.0.7204.45）；`LICENSE`（GPL-3.0）与 draw.io 核心 Apache-2.0 许可均随包。许可证原文存放于 `third_party_licenses/`。lazy-val 上游无 LICENSE 文件，使用依据 package.json 声明生成的 MIT 文本（已在声明中标注）。
- 权限用途文案：已把剪贴板、下载/文档/桌面目录四项的 reason 改写为准确描述 draw.io 用途的中英文说明。
- 构建链路：`stage-drawio-application.mjs` → `build-hap.mjs` → 已签名 HAP（调试签名）。

- **发布签名就绪并完成 APP 构建（2026-11-02）**：新发布证书 `drawio_publish.cer`（由原 CSR 签发，与 `drawio.p12` 私钥匹配，有效期至 2029-11-02）与发布 Profile `drawio_publishRelease.p7b`（type release、app_gallery、`com.neuohos.drawio`）已配置进 `build-profile.json5` 的 `release` signingConfig，product 已切到 `release`。Profile 的 allowed-acls 含全部 6 项：`kernel.ALLOW_WRITABLE_CODE_MEMORY`、`FILE_ACCESS_PERSIST`、`READ_PASTEBOARD`、`READ_WRITE_DOCUMENTS/DOWNLOAD/DESKTOP_DIRECTORY`，与工程声明一致（JIT 权限重新申请已获批）。工程级 release `assembleApp` 构建成功，产物 `harmony/DrawioHarmony/build/outputs/default/DrawioHarmony-default-signed.app`，上传用副本 `artifacts/DrawioInHarmony-32.4.1-release-signed.app`（SHA-256 `a515fe86894dbcf61a163e5f5a0a434d63545cf92cacf76cd5437877815e7e72`，含 vendor 修正）；`hap-sign-tool verify-app` 校验 .app 与模块 HAP 签名均通过，内嵌 Profile 与 `drawio_publishRelease.p7b` 逐字节一致；HAP 内 `buildMode=release`、`debug=false`、仅 2in1，HNP 已带 `.permission` 段。app_gallery 发布签名包无法直接装机，真机回归仍以调试签名进行。

## 历史待办记录（2026-10 期间）
1. ~~**正式包名与发布证书/Profile**~~：已完成（2026-10-14，包名 `com.neuohos.drawio`，发布证书/Profile 配置进 `build-profile.json5`，口令为密文）。
1b. **kernel.ALLOW_WRITABLE_CODE_MEMORY 审核回执义务**（2026-10-15 华为邮件；2026-10-17 AGC 审批以"未收到邮箱回件"驳回，需先回邮确认下列注意事项（尤其 API26 授权变化），再重新申请）：
   - **坚盾模式无闪退**（审核硬要求）：代码已落地（2026-10-16）——`web_engine/src/main/ets/common/CommandLineAdapter.ets` 的 `getArgs()` 首次调用时经 `checkAccessTokenSync` 探测该权限，未授予（坚盾模式/权限收回）则追加 `--jitless` 启动 V8 无 JIT 模式；编译与签名链已验证。**真机实验已完成（2026-10-17，HAD-W24 / 6.1.0.117 / API23，证据 `validation/jit-fallback/`）**：
     - 0 基线（有权限）：日志 `granted; V8 JIT enabled`，编辑器正常。
     - ② 删权限 + 降级路径：日志 `not granted; V8 falls back to jitless`，编辑器正常渲染，插入图形交互正常，无 faultlog。
     - ① 删权限 + 关闭本工程降级：**同样正常启动、无闪退**。原因：运行时 `libadapter.so` 自身在无权限时给渲染进程加 `--js-flags=--jitless`（日志 `StartChildProcess command --js-flags=--jitless`）；主进程无该参数也未崩溃。因此"无权限确实无法启动"未复现，本工程的 `--jitless` 是对主进程的兜底，保留。
     - 进程形态：GPU/utility/renderer 均为 appspawn 拉起的应用子进程（`com.neuohos.drawio:GPU`/`CustomChildProcessN`），不执行 HNP 内 bin；应用代码无 `fork`/`utilityProcess`，唯一 `execFile` 为 `fc-list`（设备无此命令，失败后回落空字体列表）。
   - **API26 起 bin 不继承 kernelpermission**：`independentSign: true` 已配置 ✓；`.permission` 段已接入构建链（2026-10-17）：`scripts/patch-hnp-permission.mjs` 由 `pack-probe-hnp.mjs` 在打包前调用，以 `upstream/ohcodium-runtime/ohos_hap/hnp/arm64-v8a/electron.hnp` 为源，用 SDK `llvm-objcopy` 写入 `{kernel.ALLOW_WRITABLE_CODE_MEMORY, INHERIT_PARENT_PERMISSION}`，输出到 `harmony/DrawioHarmony/hnp/`，记录在 `validation/electron-hnp-permission.json`（`verify-probe-hap.ps1` 据此校验）。上游原段只有 `INHERIT_PARENT_PERMISSION`。**kernel 权限列表从 web_engine `requestPermissions` 自动读取**，因为真机实测：bin 声明了应用未持有的权限时，安装直接失败（`9568407`，BinSec `illegal additional permission`），即官方所说"bin 不能配置本应用无法使用的权限"。
   - **升级路径风险（实测）**：设备从"未持有该权限"的已装版本升级到"申请该权限"的新版本时，同样报 `illegal additional permission` 安装失败——校验依据的是升级前已安装版本的授权状态。本机通过过渡版本解决（先装"申请权限、bin 不声明"版，再装正式版）。**含义：若首个商店版本不带 JIT 权限（例如等不及 ACL 审批），后续加上该权限的升级可能在用户设备上失败**；建议 JIT 权限批下后再发首版，或回邮时就此向华为确认。
   - 用途红线：仅 Chromium JIT，不用于热补丁类行为；权限仅 PC/2in1（deviceTypes 已是 2in1 ✓）。
2. **受限权限申请**（2026-10-17 AGC 审批结果）：已通过 `FILE_ACCESS_PERSIST`、`READ_WRITE_DOCUMENTS/DOWNLOAD/DESKTOP_DIRECTORY`、`READ_PASTEBOARD` 共 5 项，需在 Profile 管理页"添加 → 已获取权限"加入发布 Profile；`kernel.ALLOW_WRITABLE_CODE_MEMORY` 待回邮后重新申请，批下后需再次更新 Profile（最终应含 6 项 ACL）；`CUSTOM_SANDBOX`、`READ_WRITE_USER_FILE` 被驳回并已从工程删除。上架审核会按实际使用场景复核受限权限，各项 reason 文案在 `web_engine/src/main/resources/*/element/string.json`，提交前需逐条核对。新 Profile 替换后用 `strings -n 6 drawioRelease.p7b | tr -d '\n' | grep -o '"acls"[^}]*}'` 确认 ACL 列表。

### 权限删除回归（2026-10-17 删除 CUSTOM_SANDBOX / READ_WRITE_USER_FILE 后，真机待做）
使用调试签名（调试 Profile 仍含旧 ACL，不影响；工程不再声明即可验证）或新发布 Profile 构建的 HAP，**先卸载旧包再安装**，避免残留授权掩盖问题：
1. 冷启动：进程树含 GPU/渲染子进程，编辑器完整渲染，hilog 无 sandbox/permission 相关报错。
2. 新建图并通过系统保存对话框保存到 Desktop、Documents、Download 各一次。
3. 保存/打开**三目录之外**的位置（用户主目录下自建文件夹、外接 U 盘），确认选择器授权足够读写。
4. 打开已有 `.drawio`，修改后 Ctrl+S 原地覆盖保存（不弹框），重新打开内容正确。
5. 最近文件：打开三目录外的文件 → 退出应用 → 重启 → 从"最近文件"再次打开并保存（验证 persistPermission/activatePermission）。
6. 文件管理器双击 `.drawio` 冷启动打开；应用已运行时再双击另一个文件。
7. 从文件管理器拖拽 `.drawio` / 图片进编辑器。
8. 导出 PNG/SVG/HTML/PDF 到三目录外位置；PDF 打印。
9. 插入图片（选择器选本地图片）、剪贴板复制粘贴图形与外部图片。
10. 自动保存/草稿恢复：编辑中强制结束进程后重启，确认恢复提示正常。
3. **许可合规核实**（声明文件已补全，下面只剩需要向第三方确认的部分）：
   - 应用本体 GPL-3.0-only：需在商店详情页和应用内提供源码获取地址（本适配仓库/上游 jgraph/drawio-desktop）。
   - 社区 Electron 运行时（OHCodium，MKCode 仓库）未见 LICENSE 与二进制来源说明；官方 openharmony-sig/electron 同样没有 LICENSE 文件。正式分发前应向运行时提供方确认再分发条款，或改用自行从 SIG 源码构建的运行时。
4. **商店材料**：软件著作权/备案（按你们学校/团队的上架规则）、隐私政策链接、应用截图、简介、分类（创作设计 > 图形图像处理）。
5. 官方上架提示：若 PC 应用市场报权限错误，检查是否有仅 2in1 的权限声明；本工程已不含 pad/phone 入口模块（`entry` 模块未参与构建）。

## 暂留项
原先的暂留项已全部处理（文件关联、打印、导出、字体、拼写检查、更新策略），详见 DEFERRED.md。

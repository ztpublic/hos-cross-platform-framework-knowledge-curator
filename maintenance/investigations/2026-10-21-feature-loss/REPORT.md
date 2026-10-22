# Independent investigation of feature loss in HarmonyOS ports

Review date: 2026-10-21 (Asia/Shanghai). Scope: the revisions already registered in this workspace, not the latest remote branches.

## Conclusion

The evidence does **not** support a binary explanation of “HarmonyOS cannot do it” versus “the author does not know how.” The reviewed losses include explicit MVP/offline product decisions, absent framework adapters or native libraries, SDK/runtime mismatches, temporary workarounds, historical cuts already restored, and narrower permission/signing restrictions.

Several OS-wide absence claims are too broad: native clipboard and PC status-bar APIs are documented, PC printing exists, and other reviewed ports contain implementations or restoration patches. These are capability counterexamples, not proof that each original API can be restored unchanged. An implementation gap does not establish an author’s knowledge, motivation or available time.

This investigation independently read **39 pinned source/document files and 3 historical diffs across 21 selected projects**. Candidate discovery covered the 20 directly registered projects and 60 imported Flutter projects. Of the Flutter keyword screens, 59 completed and one (PiliNara) was interrupted while resolving missing Git blobs; it remains partial. Candidate screening is not an exhaustive parity audit. The imported Qt collection supplied additional leads/counterexamples, but its transitive projects were not all independently re-reviewed.

No application was built, installed or functionally exercised. No finding is a newly reproduced device result, and no finding establishes an unconditional impossibility across all HarmonyOS versions, devices and signing domains. Source-author device claims remain attributed reports. A currently documented API can rebut a present blanket claim without proving that it existed at the time a historical cut was made.

## Classification and evidence rules

| Category | Meaning |
| --- | --- |
| deliberate-scope | Source explicitly narrows the product/MVP or visibly simplifies an application flow; intent is stated only where documented. |
| dependency-or-backend | The application, selected plugin, native library, framework port or distribution backend lacks the implementation. A possible route is not a validated replacement. |
| runtime-workaround | A particular runtime/configuration is disabled to contain observed instability. |
| restored-or-counterexample | Later source restores a cut, or another concrete implementation rebuts a blanket absence claim. Runtime parity remains untested. |
| reported-restriction | A source reports a denial tied to a particular application domain/device/runtime; this review has not reproduced it. |
| unresolved | Evidence identifies a missing feature or claim but does not justify the proposed root cause. |

An OS restriction should name the exact operation, device/API version, app/signing domain and denial, plus applicable official policy or a reproducible probe. “No plugin,” an empty stub, a platform flag, or a README assertion alone does not meet that standard. HarmonyOS product services and OpenHarmony distributions must be checked independently.

## Findings

The following rows are feature cases, not mutually exclusive project labels. Counts describe this selected set and are not estimates of the whole ecosystem.

| Category | Selected cases |
| --- | ---: |
| deliberate-scope | 4 |
| dependency-or-backend | 19 |
| reported-restriction | 1 |
| restored-or-counterexample | 6 |
| runtime-workaround | 1 |
| unresolved | 2 |

### F01 — ohos-ktouch: Course/layout editors, custom lessons, full preferences and shortcut configuration

**Verdict: Explicit MVP cuts, not established OS prohibitions.** Category: `deliberate-scope`.

KTOUCH_HARMONY_MVP guards return before the KDE-backed flows; port notes explicitly choose small shims over a complete KDE port. [Pinned source](https://gitcode.com/OpenHarmonyPCDeveloper/ohos_KTouch) (`src/ktouchcontext.cpp:114`, revision `3a986f032d36`).

**What would settle implementation feasibility:** Restore one flow with real dependencies or a scoped replacement; validate persistence and dialogs. Keyboard-system KCM must be evaluated separately.

### F02 — klayout-on-harmony-pc: Python/Ruby scripting bindings

**Verdict: Selected build excludes bindings; fundamental impossibility is unproven.** Category: `dependency-or-backend`.

Port notes and the patch identify a -nopython -noruby -without-qtbinding build. This review did not reconstruct every release invocation. [Pinned source](https://github.com/yangy79/KLayout-on-Harmony-PC/blob/0189b4544a8065b2fad7ccb0fd1085c48964aacc/docs/PORTING-NOTES.md#L164) (`docs/PORTING-NOTES.md:164`, revision `0189b4544a80`).

**What would settle implementation feasibility:** Establish the exact release flags, then port/package the interpreter, extension ABI and resource closure. Embedded Python in another app is a counterexample to a blanket language ban, not proof of KLayout binding parity.

### F03 — scidavis-for-harmonyos: Printing

**Verdict: Unadapted integration; HarmonyOS PC printing exists.** Category: `dependency-or-backend`.

README lists printing as unavailable without a causal diagnosis; the repository does not include the full native application implementation in the reviewed CMake entry. [Pinned source](https://github.com/forwardz666/SciDAVis-for-HarmonyOS/blob/a1e9e3d71f617691d98061fcf4c966132241fd07/README.md#L28) (`README.md:28`, revision `a1e9e3d71f61`).

Independent platform evidence: [Printing application architecture](https://developer.huawei.com/consumer/cn/doc/doccenter-industry-solutions/practice-tools-app-architecture-v1-0000002041326514), [PC/tablet printing support](https://consumer.huawei.com/cn/support/content/zh-cn16039714/).

**What would settle implementation feasibility:** Trace Qt PrintSupport/QPA support or export PDF and integrate a native print handoff; test printer discovery, cancellation and actual output.

### F04 — scidavis-for-harmonyos: 3D plots and full scripting/advanced analysis

**Verdict: Missing in this port; exact blocking dependency not established.** Category: `unresolved`.

README lists unadapted functionality and explicitly reports insufficient maintenance resources. It does not provide a per-feature root cause. [Pinned source](https://github.com/forwardz666/SciDAVis-for-HarmonyOS/blob/a1e9e3d71f617691d98061fcf4c966132241fd07/README.md#L24) (`README.md:24`, revision `a1e9e3d71f61`).

**What would settle implementation feasibility:** Acquire the actual native source/build inputs and inspect plotting/interpreter dependencies. FreeCAD is only a counterexample to an OS-wide 3D/Python ban.

### F05 — freecad-on-harmonyos: Navigation cube and corner axes

**Verdict: Historical defensive disablement; restoration patches exist.** Category: `restored-or-counterexample`.

The debugging guide identifies FREECAD_OHOS guards around drawing calls, then restores calls and fixes the Qt/gl4es texture namespace boundary. [Pinned source](https://github.com/liang-xiaohui/freecad-on-harmonyos/blob/24d78fcc76cc4af439c7c3268c7ba81e3c664c56/docs/sketcher-rendering-debug-playbook.md#L219) (`docs/sketcher-rendering-debug-playbook.md:219`, revision `24d78fcc76cc`).

**What would settle implementation feasibility:** Validate rendering, cube picking and rotation separately. The guide itself leaves the mixed FBO picking path unverified.

### F06 — freecad-on-harmonyos: Preferences UI

**Verdict: Port/dependency and window-parent bugs were repaired in source.** Category: `restored-or-counterexample`.

README identifies disabled QUiLoader plus missing PySide QtUiTools; a native UiTools patch exists. The debugging guide separately repairs popup parenting. [Pinned source](https://github.com/liang-xiaohui/freecad-on-harmonyos/blob/24d78fcc76cc4af439c7c3268c7ba81e3c664c56/patches/freecad-1.1.2/ohos-native-uitools.patch#L1) (`patches/freecad-1.1.2/ohos-native-uitools.patch:1`, revision `24d78fcc76cc`).

**What would settle implementation feasibility:** Exercise form loading, popup/tooltip interaction and saved preferences on device; the author reports success, which was not rerun here.

### F07 — dsh-desktop-hos: Interactive PTY terminal and job control

**Verdict: App-domain restriction reported on a specific device/runtime; not independently reproduced.** Category: `reported-restriction`.

The specification reports SELinux denials for forkpty/openpty and missing application PTY APIs. It separately reports working non-PTY fork/exec/pipes and implements a resident shell. [Pinned source](https://github.com/fellow99/dsh-desktop-hos/blob/2a69cf6de336255ebaa3f1aff017f45c5281cf23/specs/010-tool-bash/spec.md#L15) (`specs/010-tool-bash/spec.md:15`, revision `2a69cf6de336`).

**What would settle implementation feasibility:** Reproduce PTY denial on the exact device/API/signing profile before calling it a confirmed restriction. Non-PTY command execution does not restore TTY semantics.

### F08 — dsh-desktop-hos: Image attachments and sharp conversion

**Verdict: Missing libvips was partly replaced; image processing was not simply removed.** Category: `restored-or-counterexample`.

README and JS bridge code route conversions to the ArkTS image framework, permit bounded clean-file pass-through and reject unavailable conversions. Thumbnails remain absent. [Pinned source](https://github.com/fellow99/dsh-desktop-hos/blob/2a69cf6de336255ebaa3f1aff017f45c5281cf23/scripts/lib/sharp-stub-body.js#L449) (`scripts/lib/sharp-stub-body.js:449`, revision `2a69cf6de336`).

**What would settle implementation feasibility:** Test conversion, EXIF orientation, limits and no-bridge errors; investigate thumbnail work separately. Do not equate a sharp stub with complete feature loss.

### F09 — dsh-desktop-hos: Bundled standalone Node/pnpm/Python executables

**Verdict: Signing/distribution restriction asserted by the author; eligibility claim is not independently verified.** Category: `unresolved`.

README attributes missing standalone runtimes to binary-certificate eligibility. It distinguishes optional system-installed pnpm from bundled product capability. [Pinned source](https://github.com/fellow99/dsh-desktop-hos/blob/2a69cf6de336255ebaa3f1aff017f45c5281cf23/README.md#L45) (`README.md:45`, revision `2a69cf6de336`).

**What would settle implementation feasibility:** Verify current HNP/ELF signing and account eligibility using official policy for this exact product. Embedded CPython libraries in FreeCAD are not the same packaging route.

### F10 — dsh-desktop-hos: Frameless application window

**Verdict: Explicit engineering tradeoff.** Category: `deliberate-scope`.

README says the frameless design was deferred because moving controls into the renderer costs more than it buys. [Pinned source](https://github.com/fellow99/dsh-desktop-hos/blob/2a69cf6de336255ebaa3f1aff017f45c5281cf23/README.md#L54) (`README.md:54`, revision `2a69cf6de336`).

**What would settle implementation feasibility:** Treat as product scope unless a concrete window-mode requirement justifies implementing and validating it.

### F11 — doubao-watermark-lab: Tray/status-bar integration

**Verdict: Already implemented at the reviewed revision.** Category: `restored-or-counterexample`.

The OH startup branch constructs Tray, sets its tooltip and installs a context menu. DSH also contains a StatusBarManager adapter. [Pinned source](https://github.com/littlestone0806/doubao-watermark-lab/blob/d255b093a4a0fc0ae6c63df2201b566d784f632f/src/main.js#L1691) (`src/main.js:1691`, revision `d255b093a4a0`).

Independent platform evidence: [PC status-bar API changes](https://developer.huawei.com/consumer/en/doc/harmonyos-releases/js-apidiff-desktopextensionkit-6003).

**What would settle implementation feasibility:** Validate icon, menu, hide/show and lifecycle on the relevant PC runtime. Do not infer full tray API parity from construction alone.

### F12 — doubao-watermark-lab: Desktop automatic updater

**Verdict: Desktop update path is gated; a distribution integration question.** Category: `dependency-or-backend`.

setupAutoUpdater explicitly returns when IS_OHOS is true; its comment defers the update channel to the application store. [Pinned source](https://github.com/littlestone0806/doubao-watermark-lab/blob/d255b093a4a0fc0ae6c63df2201b566d784f632f/src/main.js#L1634) (`src/main.js:1634`, revision `d255b093a4a0`).

**What would settle implementation feasibility:** Select an approved signed-HAP distribution/update mechanism for the product; absence of electron-updater support does not mean applications cannot be updated.

### F13 — ohos-jupyter: Terminal, debugger, LSP and extension installation in default mode

**Verdict: Default server-mode reductions, not demonstrated OS impossibility.** Category: `dependency-or-backend`.

staticLabServer disables terminal/debugger/LSP/completer extensions, returns terminal-unavailable responses and marks extension installation read-only. The README also offers Python/external-server modes. [Pinned source](https://gitcode.com/OpenHarmonyPCDeveloper/ohos_jupyter) (`electron/src/main/resources/resfile/resources/app/src/js/staticLabServer.js:72`, revision `18b16874b086`).

**What would settle implementation feasibility:** Build a per-mode matrix. An external server can supply server-side services subject to client support; a local PTY still requires its own policy/implementation assessment.

### F14 — ohos-jupyter: GPU acceleration

**Verdict: Runtime stability workaround.** Category: `runtime-workaround`.

README defaults OH Electron GPU composition off because of instability; frontend editing and Pyodide remain available. [Pinned source](https://gitcode.com/OpenHarmonyPCDeveloper/ohos_jupyter) (`README.md:200`, revision `18b16874b086`).

**What would settle implementation feasibility:** Reproduce on the pinned Electron/engine/GPU set before changing the default. This does not establish a lack of GPU capability in HarmonyOS.

### F15 — drawio-in-harmony: Single-instance events and newer Electron APIs

**Verdict: Electron-port/version coverage gap; lifecycle integration is incomplete.** Category: `dependency-or-backend`.

Feasibility notes distinguish an available Electron port from compatibility with all newer APIs. FEATURE_MATRIX still labels lifecycle/file routing as pending. [Pinned source](https://github.com/Red-Sa1t/drawio-in-harmony/blob/df83eebf55bf15c5cbef6656d863eb94e6d33b28/FEASIBILITY.md#L33) (`FEASIBILITY.md:33`, revision `df83eebf55bf`).

Independent platform evidence: [Stage process model](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides-v5/process-model-stage-V5).

**What would settle implementation feasibility:** Route duplicate launch and file intents through Ability/Want; verify cold/warm paths. The feasibility note is a lead, not independent testing of every Electron API.

### F16 — react-native-harmony-seed: Some WebView props

**Verdict: RNOH adapter incompleteness, not a proven ArkWeb/OS prohibition.** Category: `dependency-or-backend`.

Vendored adapter documentation has a property support matrix and issues for props not yet adapted to HarmonyOS. [Pinned source](https://github.com/kuma0605/react-native-harmony-seed/blob/74c23b6959e7d019e7a9d23c12296210cb6f1eb1/docs/rnoh/react-native-webview.md#L384) (`docs/rnoh/react-native-webview.md:384`, revision `74c23b6959e7`).

**What would settle implementation feasibility:** Choose a specific prop, inspect its native implementation and ArkWeb equivalent, then test that prop. Some iOS-only semantics may require redesign.

### F17 — pure-live-ohos: Recording

**Verdict: Missing FFmpeg backend artifact; source names replacement work.** Category: `dependency-or-backend`.

README describes a vendored empty FFmpeg hook and says an OH FFmpeg build or system capability is needed. [Pinned source](https://github.com/FiveHair/pure_live/blob/e55f82c8d78d54cdd90aeab0db7dc6a7b10b9fc2/README_OHOS.md#L91) (`README_OHOS.md:91`, revision `e55f82c8d78d`).

**What would settle implementation feasibility:** Implement a compatible recorder or native backend; validate codecs, stream protocols, output finalization and background lifecycle.

### F18 — pure-live-ohos: Scanning and Web login

**Verdict: Missing/unselected plugin integrations.** Category: `dependency-or-backend`.

README identifies mobile_scanner as a local Kotlin plugin and points to a CPF WebView adaptation as future work. [Pinned source](https://github.com/FiveHair/pure_live/blob/e55f82c8d78d54cdd90aeab0db7dc6a7b10b9fc2/README_OHOS.md#L98) (`README_OHOS.md:98`, revision `e55f82c8d78d`).

**What would settle implementation feasibility:** Audit plugin-native coverage and select a compatible backend. No claim is made here that the named replacement is currently available or PC-validated.

### F19 — pure-live-ohos: Hardware video decode

**Verdict: The selected libmpv build lacks the OH hardware-decoder backend.** Category: `dependency-or-backend`.

README explicitly distinguishes CPU decode/GPU rendering and proposes an AVPlayer/video_player_ohos path absent from the upstream media_core backend. [Pinned source](https://github.com/FiveHair/pure_live/blob/e55f82c8d78d54cdd90aeab0db7dc6a7b10b9fc2/README_OHOS.md#L102) (`README_OHOS.md:102`, revision `e55f82c8d78d`).

**What would settle implementation feasibility:** Check protocol, headers, HDR, subtitles and texture integration before selecting a hardware-backed player; GPU rendering is not evidence of hardware decode.

### F20 — pure-live-ohos: System PiP and PC tray

**Verdict: Plugin/host integration work; the README already identifies a native PiP path.** Category: `dependency-or-backend`.

Later README text names PiPWindow and missing Flutter texture bridging; an earlier blanket no-corresponding-PC-tray statement conflicts with native PC APIs and other reviewed adapters. [Pinned source](https://github.com/FiveHair/pure_live/blob/e55f82c8d78d54cdd90aeab0db7dc6a7b10b9fc2/README_OHOS.md#L101) (`README_OHOS.md:101`, revision `e55f82c8d78d`).

Independent platform evidence: [PC status-bar API changes](https://developer.huawei.com/consumer/en/doc/harmonyos-releases/js-apidiff-desktopextensionkit-6003).

**What would settle implementation feasibility:** Prototype PiP surface handoff separately from in-app mini-player; use a PC status-bar adapter and retain API/product gates.

### F21 — kelivo: TTS settings

**Verdict: Explicit app UI gate plus a dependency-support explanation.** Category: `dependency-or-backend`.

The TTS settings callback only shows an OH-unsupported message. README attributes this and highlighting loss to Flutter dependency support. [Pinned source](https://github.com/Chevey339/kelivo-ohos/blob/184b54e8bfdb672a566e9553bafc2090e99a5e29/lib/features/settings/pages/settings_page.dart#L179) (`lib/features/settings/pages/settings_page.dart:179`, revision `184b54e8bfdb`).

Independent platform evidence: [Core Speech Kit](https://developer.huawei.com/consumer/cn/sdk/core-speech-kit/).

**What would settle implementation feasibility:** Map the TTS contract to an OH plugin/native speech service; confirm device/voice/API eligibility and every speak/stop/cancel call site. Settings navigation alone is not complete TTS coverage.

### F22 — kelivo: Code-block syntax highlighting

**Verdict: Renderer simplification is visible; its exact technical necessity is unexplained.** Category: `deliberate-scope`.

MarkdownBody omits custom builders while keeping highlight imports and helpers. README lists code highlighting as not implemented. [Pinned source](https://github.com/Chevey339/kelivo-ohos/blob/184b54e8bfdb672a566e9553bafc2090e99a5e29/lib/shared/widgets/markdown_with_highlight.dart#L28) (`lib/shared/widgets/markdown_with_highlight.dart:28`, revision `184b54e8bfdb`).

**What would settle implementation feasibility:** Reconnect a compatible builder and test languages, selection and long blocks. Classify the code-level omission separately from the author’s broad dependency explanation.

### F23 — beecount-oh: Reminder settings and scheduling

**Verdict: Missing application factory branch, with a separate native service entitlement gate.** Category: `dependency-or-backend`.

NotificationFactory supports Android/iOS only and throws for other targets; reminder settings are hidden despite an OH notification dependency. [Pinned source](https://github.com/BeeKitStudio/beecount-openharmony/blob/14e6b10648ef7ce88414d7a562be638609205924/lib/utils/notification_factory.dart#L16) (`lib/utils/notification_factory.dart:16`, revision `14e6b10648ef`).

Independent platform evidence: [Agent-powered reminders](https://developer.huawei.com/consumer/cn/doc/doccenter-capabilities/agent-powered-reminder).

**What would settle implementation feasibility:** Implement an OH scheduling adapter and request required entitlement/permissions, or use an appropriate calendar alternative. Ordinary notifications do not prove scheduled-reminder support.

### F24 — bugaoshan: OCR

**Verdict: Historical platform guard was removed; current source selects another OCR backend.** Category: `restored-or-counterexample`.

The April diff adds a Harmony exception, the May diff removes it, and the pinned current OcrService loads scu_ocr_lite model bytes and calls recognizeAsync. [Pinned source](https://github.com/The-Brotherhood-of-SCU/Bugaoshan-HarmonyOS/blob/90706b12432711cc48f75ed70864708f15adaf35/lib/services/ocr_service.dart#L1) (`lib/services/ocr_service.dart:1`, revision `90706b124327`).

Historical evidence: [diff 24585fd3ca55](https://github.com/The-Brotherhood-of-SCU/Bugaoshan-HarmonyOS/commit/24585fd3ca55cf98834107929098001b80ab3729), [diff a691ace11e4e](https://github.com/The-Brotherhood-of-SCU/Bugaoshan-HarmonyOS/commit/a691ace11e4ea07ea317d50ae656967f9538259c).

Independent platform evidence: [Core Vision OCR FAQ](https://developer.huawei.com/consumer/cn/doc/doccenter-dev-faq/faqs-core-vision-3).

**What would settle implementation feasibility:** Validate the pinned model/backend on the target. Restored source does not prove accuracy, speed or successful PC execution.

### F25 — piliplus: Music charts

**Verdict: Historical Flutter SDK mismatch, followed by source restoration.** Category: `restored-or-counterexample`.

The restoration diff replaces the commented fl_chart import and LineChart with active code and explicitly names the older OH Flutter baseline in removed comments. [Pinned source](https://github.com/dev4harmony/PiliPlus/blob/0aba2c6db94ba9b4cf9c076b1d39179ee3d860b4/lib/pages/music/view.dart#L1) (`lib/pages/music/view.dart:1`, revision `0aba2c6db94b`).

Historical evidence: [diff 3ef92124b8cc](https://github.com/dev4harmony/PiliPlus/commit/3ef92124b8cc6746e8e9c183e4aaf94109d6bfcb).

**What would settle implementation feasibility:** Exercise the chart on the current pinned SDK; do not carry the old disablement forward as an OS limitation.

### F26 — etohos: Cross-application system clipboard

**Verdict: App-local fallback; its blanket OS-unsupported comment is contradicted by native documentation.** Category: `dependency-or-backend`.

AppClipboard stores text in a static string and is used by custom copy/paste handling. It cannot exchange text with other applications. [Pinned source](https://github.com/collaborative-creation/etohos/blob/fdb07dbfd2869502db7603b83d69d9a9661c228a/lib/utils/text_field_utils.dart#L4) (`lib/utils/text_field_utils.dart:4`, revision `fdb07dbfd286`).

Independent platform evidence: [Clipboard collaboration example](https://developer.huawei.com/consumer/en/doc/best-practices/bpta-continue).

**What would settle implementation feasibility:** Use the framework clipboard bridge or native pasteboard with permitted user-triggered access; test cross-app read/write and denial.

### F27 — oronbox: Accounts, network services and community features

**Verdict: Explicit offline Lite product scope.** Category: `deliberate-scope`.

README defines a standalone offline edition and lists removed account/community/network features. [Pinned source](https://github.com/diana9905454/OronBox-Lite/blob/27afbb7da972e240a43043bb21ae2f1851bfe965/README.md#L26) (`README.md:26`, revision `27afbb7da972`).

**What would settle implementation feasibility:** Treat restoration as a product/security/service-scope decision. The removals do not establish a HarmonyOS networking ban.

### F28 — oronbox: BLE paired/scanning-state queries and unpair semantics

**Verdict: Adapter shortcuts; native semantics were not established by these return values.** Category: `dependency-or-backend`.

isScanning/isPaired return false, pair returns true as a no-pair-required convention, and unpair ignores the operation. Actual scan/connect calls elsewhere use native channels. [Pinned source](https://github.com/diana9905454/OronBox-Lite/blob/27afbb7da972e240a43043bb21ae2f1851bfe965/lib/src/core/services/ohos_ble_channel.dart#L252) (`lib/src/core/services/ohos_ble_channel.dart:252`, revision `27afbb7da972`).

Independent platform evidence: [BLE connection and reconnection example](https://developer.huawei.com/consumer/cn/doc/doccenter-industry-solutions/btreconnectdemo-0000002757080811).

**What would settle implementation feasibility:** Separate not-needed, unsupported and unknown states. Audit native pairing APIs for the actual BLE workflow instead of reporting stub success as completed pairing.

### F29 — mi-workbench: BLE scale scanning

**Verdict: Local Flutter plugin stub and UI disablement.** Category: `dependency-or-backend`.

OHOS_BUILD says flutter_reactive_ble is replaced with a local stub and disables scale scanning; it also states the port had not been compiled/device-validated by its author. [Pinned source](https://github.com/Aqbj-ui/mi_workbench_harmony/blob/9b6478eab96dfd78c3b577608f473eca46342874/OHOS_BUILD.md#L96) (`OHOS_BUILD.md:96`, revision `9b6478eab96d`).

Independent platform evidence: [BLE connection and reconnection example](https://developer.huawei.com/consumer/cn/doc/doccenter-industry-solutions/btreconnectdemo-0000002757080811).

**What would settle implementation feasibility:** Replace or implement the BLE plugin and device protocol, then validate on the intended scale and PC hardware. Official BLE APIs do not prove this scale protocol works.

### F30 — neon: Camera, push and window-management features

**Verdict: Application platform capability flags.** Category: `dependency-or-backend`.

OhosNeonPlatform hard-codes all three capabilities false. No native rejection or per-feature root cause is supplied by that class. [Pinned source](https://github.com/shokakucarrier/neon-ohos/blob/4be2e195ebb82322c90bbd4cb1454276e9e84aa9/packages/neon_framework/lib/src/platform/ohos.dart#L29) (`packages/neon_framework/lib/src/platform/ohos.dart:29`, revision `4be2e195ebb8`).

**What would settle implementation feasibility:** Audit each selected plugin and native service separately; distinguish Push Kit onboarding from local notifications and desktop window management from in-app navigation.

### F31 — connssh: Batch SFTP download

**Verdict: App-owned single-file guard; no OS-wide batch-I/O limit established.** Category: `dependency-or-backend`.

_downloadForOhos immediately returns when filesToDownload.length > 1. A single-file OH export path exists below it. [Pinned source](https://github.com/RWPteam/ConnSSH/blob/95174411f2b1daa888ac499d4717efc22075c00b/lib/pages/sftpview.dart#L1681) (`lib/pages/sftpview.dart:1681`, revision `95174411f2b1`).

**What would settle implementation feasibility:** Design a sequential authorized export queue or folder-grant flow with cancellation/partial completion. A one-file picker operation is not proof that a batch workflow is impossible.

### F32 — fusheng-music: Material You dynamic colors

**Verdict: Android-specific integration semantics; generic adaptive colors remain a different feature.** Category: `dependency-or-backend`.

README removes dynamic_color and uses a fixed theme because its Material You integration is absent. [Pinned source](https://github.com/bmbxwbh/fu_sheng_music_ohos/blob/a2add7ecf6d5d8e1ddd55bdf4c0e90931c6fb950/README.md#L23) (`README.md:23`, revision `a2add7ecf6d5`).

**What would settle implementation feasibility:** Decide whether exact Android system-palette parity is required or an image-derived/application palette is acceptable. Do not equate those two contracts.

### F33 — nipaplay: Gamepad input

**Verdict: Explicit no-op plugin replacement.** Category: `dependency-or-backend`.

The OH universal_gamepad replacement returns empty input/connection streams and an empty gamepad list. [Pinned source](https://github.com/AimesSoft/NipaPlay-Reload/blob/7e91f4efef6cee49b05c2ee300931481a438be31/packages/universal_gamepad_ohos_stub/lib/universal_gamepad.dart#L3) (`packages/universal_gamepad_ohos_stub/lib/universal_gamepad.dart:3`, revision `7e91f4efef6c`).

**What would settle implementation feasibility:** Investigate target-device input APIs and the plugin bridge. This review establishes a stub, not native gamepad availability or a guaranteed remedy.

## Priority follow-through

1. **Correct misleading capability wording first:** ETOHOS clipboard and Pure Live’s blanket PC-tray absence. Keep adapter gaps and platform availability separate.
2. **Investigate concrete missing adapters:** BeeCount’s OH notification factory, Mi Workbench BLE, Kelivo TTS/highlighting, ConnSSH export queue and SciDAVis printing. These have clear implementation boundaries; native availability does not yet establish effort or parity.
3. **Avoid reviving obsolete limitations:** use the current Bugaoshan OCR, PiliPlus chart and FreeCAD restoration source when assessing status.
4. **Reserve targeted policy/device probes for real uncertainty:** PTY access, standalone executable/HNP eligibility, reminder approval and device-specific codec/surface/input behavior. These require exact product, API, signing and runtime inputs.

No fixes or source-project setup workflows were run as part of this report. Existing consumer knowledge and imported snapshots were preserved. Findings here remain investigation records rather than new verified consumer recipes.

## Coverage and reproducibility

- [coverage.json](coverage.json) inventories all 80 registered project identities, revisions and review dispositions. The two imported collection records are provenance inputs, not two additional projects.
- [evidence.json](evidence.json) records per-file revisions, SHA-256 hashes, paths/lines, selected excerpts, historical diffs and before/after HEAD/tracked-status fingerprints for the selected readable source checkouts. The fingerprints matched; untracked content and index bytes were not independently audited.
- [findings.json](findings.json) preserves the classification, evidence link, open question and acceptance direction for every case.
- [official-sources.json](official-sources.json) records the primary documentation checks and extraction limits. Several direct page opens yielded only JavaScript shells or timeouts; readable primary-publisher search extracts supplied the stated claims. Inaccessible content was not treated as read.
- [collect_evidence.py](collect_evidence.py) reads pinned Git objects or existing snapshots; [write_report.py](write_report.py) renders the explicit analyst judgments. They do not check out, build, sign or run the source apps. Reading a missing promisor object can require fetching its bytes; earlier discovery did so without updating checked-out branches.

The input index contained 187 knowledge entries. Full metadata/provenance/index validation passed before investigation. See the matching maintenance run record for final validation. No analyzed source checkpoints were advanced: feature-cause inspection does not constitute an exhaustive new commit-range review.

Projects with no detailed finding are **not cleared for feature parity**. Candidate discovery can miss silent removals, code without platform markers, binary-only behavior and server-side restrictions. Confirming complete parity would require an upstream feature inventory and per-feature device execution for each product; this report makes no such claim.

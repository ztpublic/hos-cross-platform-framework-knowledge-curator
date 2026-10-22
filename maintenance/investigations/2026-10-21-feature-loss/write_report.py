"""Produce the review artifact from explicit analyst judgments and pinned evidence."""
from collections import Counter
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
evidence = json.loads((HERE / 'evidence.json').read_text())
files = {(f['project'], f['path']): f for f in evidence['files']}

official = [
 ('O1', 'PC status-bar API changes', 'https://developer.huawei.com/consumer/en/doc/harmonyos-releases/js-apidiff-desktopextensionkit-6003',
  'Huawei documents addToStatusBar, icon/menu updates and DesktopExtensionKit. This rebuts an OS-wide absence claim; it does not establish full Electron Tray parity.'),
 ('O2', 'Core Speech Kit', 'https://developer.huawei.com/consumer/cn/sdk/core-speech-kit/',
  'Huawei exposes text-to-speech as an SDK capability. Exact device, API, voice and plugin integration still need validation.'),
 ('O3', 'Core Vision OCR FAQ', 'https://developer.huawei.com/consumer/cn/doc/doccenter-dev-faq/faqs-core-vision-3',
  'Huawei describes an application-facing textRecognition.recognizeText API. This is an alternative capability family, not a reproduced substitute for the project OCR model.'),
 ('O4', 'BLE connection and reconnection example', 'https://developer.huawei.com/consumer/cn/doc/doccenter-industry-solutions/btreconnectdemo-0000002757080811',
  'Huawei documents BLE scanning, GATT connections and lifecycle handling. A missing Flutter plugin is not evidence of missing Bluetooth in the OS.'),
 ('O5', 'Clipboard collaboration example', 'https://developer.huawei.com/consumer/en/doc/best-practices/bpta-continue',
  'Huawei documents the system pasteboard for local and cross-device copy/paste. Access rules and product support remain relevant.'),
 ('O6', 'Printing application architecture', 'https://developer.huawei.com/consumer/cn/doc/doccenter-industry-solutions/practice-tools-app-architecture-v1-0000002041326514',
  'Huawei describes an ArkTS/C++ printing application with phone, tablet and 2-in-1 HAPs. This rebuts an OS-wide printing impossibility claim.'),
 ('O7', 'PC/tablet printing support', 'https://consumer.huawei.com/cn/support/content/zh-cn16039714/',
  'Huawei documents printing on HarmonyOS 5.0+ PCs/tablets. It does not prove SciDAVis QPrinter integration.'),
 ('O8', 'Agent-powered reminders', 'https://developer.huawei.com/consumer/cn/doc/doccenter-capabilities/agent-powered-reminder',
  'Huawei documents reminders on PC/2in1 and other products, with controlled access, eligible app/scenario requirements, permission and notification authorization. Calendar Kit is a documented alternative.'),
 ('O9', 'Stage process model', 'https://developer.huawei.com/consumer/cn/doc/harmonyos-guides-v5/process-model-stage-V5',
  'Huawei documents 2in1 Ability/HAP process isolation. This is a lifecycle model, not unrestricted POSIX exec, PTY or permission to bundle executables.'),
]
(HERE / 'official-sources.json').write_text(json.dumps({'accessed_on': '2026-10-21',
 'extraction': 'Primary publisher pages located through web search. Substantive search-index extracts were readable; several direct documentation opens returned a JS shell or timed out. No inaccessible full-page content was treated as read.',
 'sources': [dict(id=i, title=t, url=u, assessment=a) for i,t,u,a in official]}, ensure_ascii=False, indent=2) + '\n')

rows = []
def add(pid, feature, category, verdict, observation, next_step, path, line, refs=()):
    f = files[(pid, path)]
    rows.append(dict(id=f'F{len(rows)+1:02d}', project=pid, feature=feature,
        category=category, verdict=verdict, observed=observation, next_step=next_step,
        source=dict(path=path, line=line, revision=f['revision'], sha256=f['sha256'],
                    url=f.get('url', f['repository'])), official_refs=list(refs),
        evidence_tier='source-reviewed; native alternatives documentation-only',
        runtime_reproduced=False, author_competence='not inferable'))

add('ohos-ktouch', 'Course/layout editors, custom lessons, full preferences and shortcut configuration', 'deliberate-scope',
 'Explicit MVP cuts, not established OS prohibitions.',
 'KTOUCH_HARMONY_MVP guards return before the KDE-backed flows; port notes explicitly choose small shims over a complete KDE port.',
 'Restore one flow with real dependencies or a scoped replacement; validate persistence and dialogs. Keyboard-system KCM must be evaluated separately.',
 'src/ktouchcontext.cpp', 114)
add('klayout-on-harmony-pc', 'Python/Ruby scripting bindings', 'dependency-or-backend',
 'Selected build excludes bindings; fundamental impossibility is unproven.',
 'Port notes and the patch identify a -nopython -noruby -without-qtbinding build. This review did not reconstruct every release invocation.',
 'Establish the exact release flags, then port/package the interpreter, extension ABI and resource closure. Embedded Python in another app is a counterexample to a blanket language ban, not proof of KLayout binding parity.',
 'docs/PORTING-NOTES.md', 164)
add('scidavis-for-harmonyos', 'Printing', 'dependency-or-backend',
 'Unadapted integration; HarmonyOS PC printing exists.',
 'README lists printing as unavailable without a causal diagnosis; the repository does not include the full native application implementation in the reviewed CMake entry.',
 'Trace Qt PrintSupport/QPA support or export PDF and integrate a native print handoff; test printer discovery, cancellation and actual output.',
 'README.md', 28, ['O6','O7'])
add('scidavis-for-harmonyos', '3D plots and full scripting/advanced analysis', 'unresolved',
 'Missing in this port; exact blocking dependency not established.',
 'README lists unadapted functionality and explicitly reports insufficient maintenance resources. It does not provide a per-feature root cause.',
 'Acquire the actual native source/build inputs and inspect plotting/interpreter dependencies. FreeCAD is only a counterexample to an OS-wide 3D/Python ban.',
 'README.md', 24)
add('freecad-on-harmonyos', 'Navigation cube and corner axes', 'restored-or-counterexample',
 'Historical defensive disablement; restoration patches exist.',
 'The debugging guide identifies FREECAD_OHOS guards around drawing calls, then restores calls and fixes the Qt/gl4es texture namespace boundary.',
 'Validate rendering, cube picking and rotation separately. The guide itself leaves the mixed FBO picking path unverified.',
 'docs/sketcher-rendering-debug-playbook.md', 219)
add('freecad-on-harmonyos', 'Preferences UI', 'restored-or-counterexample',
 'Port/dependency and window-parent bugs were repaired in source.',
 'README identifies disabled QUiLoader plus missing PySide QtUiTools; a native UiTools patch exists. The debugging guide separately repairs popup parenting.',
 'Exercise form loading, popup/tooltip interaction and saved preferences on device; the author reports success, which was not rerun here.',
 'patches/freecad-1.1.2/ohos-native-uitools.patch', 1)
add('dsh-desktop-hos', 'Interactive PTY terminal and job control', 'reported-restriction',
 'App-domain restriction reported on a specific device/runtime; not independently reproduced.',
 'The specification reports SELinux denials for forkpty/openpty and missing application PTY APIs. It separately reports working non-PTY fork/exec/pipes and implements a resident shell.',
 'Reproduce PTY denial on the exact device/API/signing profile before calling it a confirmed restriction. Non-PTY command execution does not restore TTY semantics.',
 'specs/010-tool-bash/spec.md', 15)
add('dsh-desktop-hos', 'Image attachments and sharp conversion', 'restored-or-counterexample',
 'Missing libvips was partly replaced; image processing was not simply removed.',
 'README and JS bridge code route conversions to the ArkTS image framework, permit bounded clean-file pass-through and reject unavailable conversions. Thumbnails remain absent.',
 'Test conversion, EXIF orientation, limits and no-bridge errors; investigate thumbnail work separately. Do not equate a sharp stub with complete feature loss.',
 'scripts/lib/sharp-stub-body.js', 449)
add('dsh-desktop-hos', 'Bundled standalone Node/pnpm/Python executables', 'unresolved',
 'Signing/distribution restriction asserted by the author; eligibility claim is not independently verified.',
 'README attributes missing standalone runtimes to binary-certificate eligibility. It distinguishes optional system-installed pnpm from bundled product capability.',
 'Verify current HNP/ELF signing and account eligibility using official policy for this exact product. Embedded CPython libraries in FreeCAD are not the same packaging route.',
 'README.md', 45)
add('dsh-desktop-hos', 'Frameless application window', 'deliberate-scope',
 'Explicit engineering tradeoff.',
 'README says the frameless design was deferred because moving controls into the renderer costs more than it buys.',
 'Treat as product scope unless a concrete window-mode requirement justifies implementing and validating it.',
 'README.md', 54)
add('doubao-watermark-lab', 'Tray/status-bar integration', 'restored-or-counterexample',
 'Already implemented at the reviewed revision.',
 'The OH startup branch constructs Tray, sets its tooltip and installs a context menu. DSH also contains a StatusBarManager adapter.',
 'Validate icon, menu, hide/show and lifecycle on the relevant PC runtime. Do not infer full tray API parity from construction alone.',
 'src/main.js', 1691, ['O1'])
add('doubao-watermark-lab', 'Desktop automatic updater', 'dependency-or-backend',
 'Desktop update path is gated; a distribution integration question.',
 'setupAutoUpdater explicitly returns when IS_OHOS is true; its comment defers the update channel to the application store.',
 'Select an approved signed-HAP distribution/update mechanism for the product; absence of electron-updater support does not mean applications cannot be updated.',
 'src/main.js', 1634)
add('ohos-jupyter', 'Terminal, debugger, LSP and extension installation in default mode', 'dependency-or-backend',
 'Default server-mode reductions, not demonstrated OS impossibility.',
 'staticLabServer disables terminal/debugger/LSP/completer extensions, returns terminal-unavailable responses and marks extension installation read-only. The README also offers Python/external-server modes.',
 'Build a per-mode matrix. An external server can supply server-side services subject to client support; a local PTY still requires its own policy/implementation assessment.',
 'electron/src/main/resources/resfile/resources/app/src/js/staticLabServer.js', 72)
add('ohos-jupyter', 'GPU acceleration', 'runtime-workaround',
 'Runtime stability workaround.',
 'README defaults OH Electron GPU composition off because of instability; frontend editing and Pyodide remain available.',
 'Reproduce on the pinned Electron/engine/GPU set before changing the default. This does not establish a lack of GPU capability in HarmonyOS.',
 'README.md', 200)
add('drawio-in-harmony', 'Single-instance events and newer Electron APIs', 'dependency-or-backend',
 'Electron-port/version coverage gap; lifecycle integration is incomplete.',
 'Feasibility notes distinguish an available Electron port from compatibility with all newer APIs. FEATURE_MATRIX still labels lifecycle/file routing as pending.',
 'Route duplicate launch and file intents through Ability/Want; verify cold/warm paths. The feasibility note is a lead, not independent testing of every Electron API.',
 'FEASIBILITY.md', 33, ['O9'])
add('react-native-harmony-seed', 'Some WebView props', 'dependency-or-backend',
 'RNOH adapter incompleteness, not a proven ArkWeb/OS prohibition.',
 'Vendored adapter documentation has a property support matrix and issues for props not yet adapted to HarmonyOS.',
 'Choose a specific prop, inspect its native implementation and ArkWeb equivalent, then test that prop. Some iOS-only semantics may require redesign.',
 'docs/rnoh/react-native-webview.md', 384)
add('pure-live-ohos', 'Recording', 'dependency-or-backend',
 'Missing FFmpeg backend artifact; source names replacement work.',
 'README describes a vendored empty FFmpeg hook and says an OH FFmpeg build or system capability is needed.',
 'Implement a compatible recorder or native backend; validate codecs, stream protocols, output finalization and background lifecycle.',
 'README_OHOS.md', 91)
add('pure-live-ohos', 'Scanning and Web login', 'dependency-or-backend',
 'Missing/unselected plugin integrations.',
 'README identifies mobile_scanner as a local Kotlin plugin and points to a CPF WebView adaptation as future work.',
 'Audit plugin-native coverage and select a compatible backend. No claim is made here that the named replacement is currently available or PC-validated.',
 'README_OHOS.md', 98)
add('pure-live-ohos', 'Hardware video decode', 'dependency-or-backend',
 'The selected libmpv build lacks the OH hardware-decoder backend.',
 'README explicitly distinguishes CPU decode/GPU rendering and proposes an AVPlayer/video_player_ohos path absent from the upstream media_core backend.',
 'Check protocol, headers, HDR, subtitles and texture integration before selecting a hardware-backed player; GPU rendering is not evidence of hardware decode.',
 'README_OHOS.md', 102)
add('pure-live-ohos', 'System PiP and PC tray', 'dependency-or-backend',
 'Plugin/host integration work; the README already identifies a native PiP path.',
 'Later README text names PiPWindow and missing Flutter texture bridging; an earlier blanket no-corresponding-PC-tray statement conflicts with native PC APIs and other reviewed adapters.',
 'Prototype PiP surface handoff separately from in-app mini-player; use a PC status-bar adapter and retain API/product gates.',
 'README_OHOS.md', 101, ['O1'])
add('kelivo', 'TTS settings', 'dependency-or-backend',
 'Explicit app UI gate plus a dependency-support explanation.',
 'The TTS settings callback only shows an OH-unsupported message. README attributes this and highlighting loss to Flutter dependency support.',
 'Map the TTS contract to an OH plugin/native speech service; confirm device/voice/API eligibility and every speak/stop/cancel call site. Settings navigation alone is not complete TTS coverage.',
 'lib/features/settings/pages/settings_page.dart', 179, ['O2'])
add('kelivo', 'Code-block syntax highlighting', 'deliberate-scope',
 'Renderer simplification is visible; its exact technical necessity is unexplained.',
 'MarkdownBody omits custom builders while keeping highlight imports and helpers. README lists code highlighting as not implemented.',
 'Reconnect a compatible builder and test languages, selection and long blocks. Classify the code-level omission separately from the author’s broad dependency explanation.',
 'lib/shared/widgets/markdown_with_highlight.dart', 28)
add('beecount-oh', 'Reminder settings and scheduling', 'dependency-or-backend',
 'Missing application factory branch, with a separate native service entitlement gate.',
 'NotificationFactory supports Android/iOS only and throws for other targets; reminder settings are hidden despite an OH notification dependency.',
 'Implement an OH scheduling adapter and request required entitlement/permissions, or use an appropriate calendar alternative. Ordinary notifications do not prove scheduled-reminder support.',
 'lib/utils/notification_factory.dart', 16, ['O8'])
add('bugaoshan', 'OCR', 'restored-or-counterexample',
 'Historical platform guard was removed; current source selects another OCR backend.',
 'The April diff adds a Harmony exception, the May diff removes it, and the pinned current OcrService loads scu_ocr_lite model bytes and calls recognizeAsync.',
 'Validate the pinned model/backend on the target. Restored source does not prove accuracy, speed or successful PC execution.',
 'lib/services/ocr_service.dart', 1, ['O3'])
add('piliplus', 'Music charts', 'restored-or-counterexample',
 'Historical Flutter SDK mismatch, followed by source restoration.',
 'The restoration diff replaces the commented fl_chart import and LineChart with active code and explicitly names the older OH Flutter baseline in removed comments.',
 'Exercise the chart on the current pinned SDK; do not carry the old disablement forward as an OS limitation.',
 'lib/pages/music/view.dart', 1)
add('etohos', 'Cross-application system clipboard', 'dependency-or-backend',
 'App-local fallback; its blanket OS-unsupported comment is contradicted by native documentation.',
 'AppClipboard stores text in a static string and is used by custom copy/paste handling. It cannot exchange text with other applications.',
 'Use the framework clipboard bridge or native pasteboard with permitted user-triggered access; test cross-app read/write and denial.',
 'lib/utils/text_field_utils.dart', 4, ['O5'])
add('oronbox', 'Accounts, network services and community features', 'deliberate-scope',
 'Explicit offline Lite product scope.',
 'README defines a standalone offline edition and lists removed account/community/network features.',
 'Treat restoration as a product/security/service-scope decision. The removals do not establish a HarmonyOS networking ban.',
 'README.md', 26)
add('oronbox', 'BLE paired/scanning-state queries and unpair semantics', 'dependency-or-backend',
 'Adapter shortcuts; native semantics were not established by these return values.',
 'isScanning/isPaired return false, pair returns true as a no-pair-required convention, and unpair ignores the operation. Actual scan/connect calls elsewhere use native channels.',
 'Separate not-needed, unsupported and unknown states. Audit native pairing APIs for the actual BLE workflow instead of reporting stub success as completed pairing.',
 'lib/src/core/services/ohos_ble_channel.dart', 252, ['O4'])
add('mi-workbench', 'BLE scale scanning', 'dependency-or-backend',
 'Local Flutter plugin stub and UI disablement.',
 'OHOS_BUILD says flutter_reactive_ble is replaced with a local stub and disables scale scanning; it also states the port had not been compiled/device-validated by its author.',
 'Replace or implement the BLE plugin and device protocol, then validate on the intended scale and PC hardware. Official BLE APIs do not prove this scale protocol works.',
 'OHOS_BUILD.md', 96, ['O4'])
add('neon', 'Camera, push and window-management features', 'dependency-or-backend',
 'Application platform capability flags.',
 'OhosNeonPlatform hard-codes all three capabilities false. No native rejection or per-feature root cause is supplied by that class.',
 'Audit each selected plugin and native service separately; distinguish Push Kit onboarding from local notifications and desktop window management from in-app navigation.',
 'packages/neon_framework/lib/src/platform/ohos.dart', 29)
add('connssh', 'Batch SFTP download', 'dependency-or-backend',
 'App-owned single-file guard; no OS-wide batch-I/O limit established.',
 '_downloadForOhos immediately returns when filesToDownload.length > 1. A single-file OH export path exists below it.',
 'Design a sequential authorized export queue or folder-grant flow with cancellation/partial completion. A one-file picker operation is not proof that a batch workflow is impossible.',
 'lib/pages/sftpview.dart', 1681)
add('fusheng-music', 'Material You dynamic colors', 'dependency-or-backend',
 'Android-specific integration semantics; generic adaptive colors remain a different feature.',
 'README removes dynamic_color and uses a fixed theme because its Material You integration is absent.',
 'Decide whether exact Android system-palette parity is required or an image-derived/application palette is acceptable. Do not equate those two contracts.',
 'README.md', 23)
add('nipaplay', 'Gamepad input', 'dependency-or-backend',
 'Explicit no-op plugin replacement.',
 'The OH universal_gamepad replacement returns empty input/connection streams and an empty gamepad list.',
 'Investigate target-device input APIs and the plugin bridge. This review establishes a stub, not native gamepad availability or a guaranteed remedy.',
 'packages/universal_gamepad_ohos_stub/lib/universal_gamepad.dart', 3)

(HERE / 'findings.json').write_text(json.dumps({'schema_version': 1, 'date': '2026-10-21',
    'findings': rows}, ensure_ascii=False, indent=2) + '\n')

counts = Counter(r['category'] for r in rows)
intro = '''# Independent investigation of feature loss in HarmonyOS ports

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

'''
parts = [intro, '| Category | Selected cases |\n| --- | ---: |\n']
parts.extend(f'| {c} | {n} |\n' for c,n in sorted(counts.items()))
parts.append('\n')
for r in rows:
    s = r['source']
    link = s['url'] + (f"#L{s['line']}" if '/blob/' in s['url'] else '')
    parts.append(f"### {r['id']} — {r['project']}: {r['feature']}\n\n")
    parts.append(f"**Verdict: {r['verdict']}** Category: `{r['category']}`.\n\n")
    parts.append(r['observed'] + f" [Pinned source]({link}) (`{s['path']}:{s['line']}`, revision `{s['revision'][:12]}`).\n\n")
    if r['id'] in ['F24','F25']:
        ids = ['bugaoshan'] if r['id'] == 'F24' else ['piliplus']
        parts.append('Historical evidence: ' + ', '.join(f"[diff {h['revision'][:12]}]({h['url']})" for h in evidence['historical_diffs'] if h['project'] in ids) + '.\n\n')
    if r['official_refs']:
        parts.append('Independent platform evidence: ' + ', '.join(f'[{o[1]}]({o[2]})' for o in official if o[0] in r['official_refs']) + '.\n\n')
    parts.append('**What would settle implementation feasibility:** ' + r['next_step'] + '\n\n')

parts.append('''## Priority follow-through

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
''')
(HERE / 'REPORT.md').write_text(''.join(parts))
print(json.dumps({'findings': len(rows), 'categories': dict(counts), 'report': str(HERE / 'REPORT.md')}))

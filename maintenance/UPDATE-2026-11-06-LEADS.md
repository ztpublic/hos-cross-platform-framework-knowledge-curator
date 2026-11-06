# Deferred-lead discovery — 2026-11-06

This pass follows the [earlier update the same day](UPDATE-2026-11-06.md). The database grew from **264 to 283 entries**: **19 added**, **2 extended**, **0 merged**, **0 split**. Additions comprise **8 common** and **11 framework-specific** mechanisms (Electron 6, Qt 3, Flutter 1, Kotlin Multiplatform 1).

The registry now holds **75 project/library/toolchain records plus five knowledge collections**. **Tauri again gained no framework-specific entry.** Nothing was committed.

## Screening outcome

Twenty-two leads were screened: the listed deferred leads, plus Flutter and Tauri candidates from the AtomGit results and 14 extra targeted queries that added only 10 results.

| Disposition | Count | Sources |
| --- | ---: | --- |
| Source reviewed, contributed | 8 | github-desktop, Theia, RStudio, OpenShot, vidcutter, KMP toolchain, flutter_downloader, flutter_tts |
| Collection, contributed | 3 | electron-harmonyos-kb, electron-ohos-adapt, flutter-harmony-ebook |
| Duplicate | 3 | ohos_Electron-skills (identical blobs), tauri-mqtt-client (still partial), nutpi tauri-demo |
| Screened, no new atom | 2 | CPF-RN usage-docs, flutterOhOs |
| Unqualified | 2 | Scribus (mock shell), juCang (Windows only) |
| Insufficient | 2 | compose-multiplatform-core (one odd branch), qq8864/tauri-ohos (no branches) |
| Different framework or host | 2 | git-cola (ArkTS rewrite), OHOS_Tauri (ArkWeb shim, not the Tauri runtime) |

**Descriptions were wrong again:** the RStudio port has no QtWebEngine and is a Qt5 Widgets rewrite, Scribus is a mock shell whose full build stops with FATAL_ERROR, and git-cola is an ArkTS rewrite.

**Why Tauri gained nothing:** the two resolved demos repeat the known fork chain, the MQTT port still points at an absent local checkout, and the migration tool replaces the Tauri runtime with a shim and stubbed Rust.

## Added mechanisms

Electron: [forced exit on close](../knowledge/electron-ohos-close-reload-forced-exit.md), [renderer subprocess proxy](../knowledge/electron-ohos-renderer-subprocess-main-proxy.md), [crash reporting](../knowledge/electron-ohos-crash-window-not-shown.md), [fork and appspawn](../knowledge/electron-ohos-execpath-appspawn-fork.md) (**unresolved**), [flat node_modules](../knowledge/electron-ohos-resfile-flat-node-modules.md) (imported guidance), [exec path metadata](../knowledge/electron-ohos-exec-path-metadata.md) (imported guidance).

Qt: [embedded CPython launcher](../knowledge/qt-pyqt-ohos-embedded-cpython-launcher.md), [ghost tooltip windows](../knowledge/qt-pyqt-ohos-ghost-tooltip-windows.md), [raw GL video window](../knowledge/qt5-ohos-foreign-gl-video-window.md).

Flutter: [background callback engine](../knowledge/flutter-ohos-background-callback-engine.md). KMP: [LLDB attach](../knowledge/kmp-ohos-lldb-native-attach.md).

Common: [private HNP environment](../knowledge/hnp-tool-private-prefix-environment.md), [git and ssh identity](../knowledge/ohos-git-ssh-sandbox-identity.md), [window close veto](../knowledge/ohos-window-close-async-veto.md) (imported guidance), [certificate fingerprint](../knowledge/ohos-certificate-fingerprint-pinning.md) (imported guidance), [system downloads](../knowledge/ohos-system-download-to-public-downloads.md), [TTS completion and voices](../knowledge/ohos-tts-playback-completion-and-voice-rebind.md), [Python extension suffix](../knowledge/python-ohos-extension-suffix-match.md) (imported guidance), [in-process interpreter](../knowledge/ohos-embedded-interpreter-in-process.md).

Extended: [document picker access](../knowledge/document-picker-uri-access.md) and [typed channel payloads](../knowledge/typed-channel-payload-extraction.md), both from documentation claims.

## Evidence notes

- **Five entries are `imported-guidance`**, because their evidence is documentation with no reviewable implementation. The two extensions carry the same caveat in their text.
- **Fork is unresolved.** One host reports `spawn /system/bin/appspawn EACCES`, another reports fork working on Electron 34, and the retained evidence does not reconcile them. The suggested explicit execPath is a curator idea and is labeled untested.
- **Close handling.** The reload-after-close branch exists in code but never appears in the source's retained logs, so it is described as defensive.
- **Flat node_modules.** The source's evidence cites an unsigned HAP next to a device-load claim, so the runtime cause is labeled as reported.
- **Safety callout.** The source's `safe.directory = *` disables git's ownership check, so the entry recommends per-repository entries instead and states the risk.
- **Certificate probe.** A separate TLS probe does not guarantee later requests reach the same server. This is a curator caution, recorded in provenance.

## Licensing

Four sources without a usable license keep **hash-only evidence** (electron-harmonyos-kb, electron-ohos-adapt, flutter-harmony-ebook, vidcutter). Notices for licensed sources are in `maintenance/licenses/`. Two unlicensed sources from the earlier pass today (zcode, Tauri-harmony) were converted to the same form before publishing. Unlicensed sources from runs before today keep their earlier snapshots.

## Remaining leads

Many AtomGit results remain unscreened, including ohos_bolt.diy, ohos_AnythingLLM, ohos_hoppscotch, ohos_etcher, ohos_codelite, ohos_CuteChess, ohos_QualCoder, ohos_Veusz, orange3, pyqt-fluent-widgets, and the oh-tpc Kotlin libraries. **This is not saturation.** `ohos-electron-builder` command-line signing, a low-confidence ESM entry failure and an ohpm multimodule error were noted but not added.

## Validation

Performed: `kb.py index` and `check`, 16 symptom and framework retrieval checks, and `git diff --check`. Fifteen checks returned the expected entry first; "QMediaPlayer not working" returned it second. Key delegated findings were re-read against pinned source before writing. No source-project setup, build, migration, signing, installation, emulator or PC workflow was run, and all entries stay `pc_validation=not-run`.

[Run record](runs/2026-11-06-deferred-leads.json) · [Candidate dispositions](reviews/2026-11-06/candidate-dispositions-2.json) · [Source registry](sources.json)

# HarmonyOS PC adaptation knowledge

Reusable solutions for developers adapting cross-platform applications to HarmonyOS PC. Each entry is a Markdown file in [knowledge/](knowledge/); the structured, searchable catalog is [index.csv](index.csv).

The installed curator is [`harmonyos-pc-knowledge-curator`](/Users/mac/.codex/skills/harmonyos-pc-knowledge-curator/SKILL.md). It supports importing notes, researching new adaptations, updating tracked sources and querying this database for Flutter, Qt, Electron, React Native and Kotlin Multiplatform.

## Classification

- **common**: a platform or native integration contract independent of a particular UI framework. An empty `frameworks` field means no framework-specific requirement, not successful validation on every framework.
- **framework-specific**: a solution dependent on the named framework's APIs, plugins, embedding or build integration.

For example, document URI identity and read/write grants are common; consuming an XFile or mapping QFileDialog suffix filters are framework adapters. General guidance and adapters link to one another.

The database contains **187 entries: 88 common, 32 Flutter-specific, 43 Qt-specific, 13 Electron-specific, 5 React Native-specific and 6 Kotlin Multiplatform-specific**. Queries for Electron, RN and KMP now include framework adapters as well as relevant common knowledge.

## Query and maintenance

```sh
python3 /Users/mac/.codex/skills/harmonyos-pc-knowledge-curator/scripts/kb.py search 'DocumentViewPicker permission' --framework rn
python3 /Users/mac/.codex/skills/harmonyos-pc-knowledge-curator/scripts/kb.py search 'modulesFactories' --framework qt
python3 /Users/mac/.codex/skills/harmonyos-pc-knowledge-curator/scripts/kb.py stats
python3 /Users/mac/.codex/skills/harmonyos-pc-knowledge-curator/scripts/kb.py index
python3 /Users/mac/.codex/skills/harmonyos-pc-knowledge-curator/scripts/kb.py check
```

Commands resolve this database from the skill's configuration, regardless of the current directory. Override with `--database /absolute/path` before the command. No third-party Python package is required for database operations. `--framework rn` and `--framework kmp` accept canonical-name aliases; add `--scope framework-specific` to search only framework adapters.

Edit entry metadata/body and its provenance, then regenerate the CSV. JSON frontmatter is the canonical metadata, so the CSV cannot silently disagree with the Markdown. See the installed [schema](/Users/mac/.codex/skills/harmonyos-pc-knowledge-curator/references/schema.md) and [authoring rules](/Users/mac/.codex/skills/harmonyos-pc-knowledge-curator/references/authoring.md).

## Evidence and import

All 63 Flutter knowledge documents were imported, with broad entries split and common contracts generalized. Independent Qt/native lessons were extracted from the Qt collection rather than copying its end-to-end migration workflow. See [the import report](maintenance/IMPORT-REPORT.md) and [source-to-entry map](maintenance/import-map.csv).

`provenance/` retains original content snapshots, hashes, revisions, source-reported evidence and transformation notes. `maintenance/` holds source inventories, review outcomes, attribution and the one-time import recipe. Consumer search excludes these maintenance files.

A source-review pass over twenty HarmonyOS PC projects added Electron runtime, addon, packaging, platform-gate, resfile-injection, process-isolation, version-shim, status-bar/host, local-server, private-HNP, window-type and main-inspect entries; Qt entrypoint/resource/plugin/linker/multi-process, Qt 6 High-DPI/subwindow, Qt 6 media texture/subtitle/AVSession, Qt 6 GL-translation/splash, Qt/ArkTS work-folder/dialog/chrome, Qt QML/KDE-shim, and unresolved Qt clipboard/font entries; RNOH host/bundle/package-adaptation, offline-asset and HAR-ReactCommon entries; KMP Compose target/host, OH-artifact/renderer, rawfile-resource and KuiklyUI host/resize entries; and Flutter OH-PC form-factor, versioned-library packaging and TargetPlatform-vendoring entries. Their commit-pinned evidence is under `provenance/snapshots/projects/`, with the screened candidates recorded in `maintenance/runs/`.

A final registry sweep found no further framework-level project that added a distinct, source-backed mechanism. Remaining candidates were app-specific, incomplete, empty, or outside the five curated frameworks (ArkWeb/WASM, Python, native C++/Rust). The sweep and its scope limits are recorded in [the exhaustion run](maintenance/runs/2026-10-20-project-research-16.json).

**The database validates integrity and retrieval, not app behavior.** The initial import left every entry at `evidence_tier=imported-guidance`; the project-review passes added forty-nine entries at `evidence_tier=source-reviewed`, two of them explicitly unresolved. All entries remain `pc_validation=not-run`. Source-reported builds or device experiments remain attributed historical reports. Each entry's “Check it works” section is an acceptance plan, not a claim those tests ran here.

The Qt collection's [MIT notice](maintenance/licenses/qt-MIT.txt) is retained. The project-review sources retain their original [MIT](maintenance/licenses/dsh-desktop-hos-MIT.txt), [GPL-3.0](maintenance/licenses/klayout-GPL-3.0.txt), [GPL-3.0](maintenance/licenses/drawio-in-harmony-GPL-3.0.txt), [GPL-3.0](maintenance/licenses/notepad--GPL-3.0.txt), [GPL-2.0](maintenance/licenses/vlc-GPL-2.0.txt), [GPL-2.0](maintenance/licenses/scidavis-GPL-2.0.txt), [GPL-2.0-or-later](maintenance/licenses/ktouch-GPL-2.0-or-later.txt), [AGPL notice](maintenance/licenses/ohos-joplin-AGPL-3.0.txt), [AGPL notice](maintenance/licenses/pure-live-AGPL-3.0.txt), [LGPL-2.1](maintenance/licenses/freecad-LGPL-2.1.txt), [BSD-style](maintenance/licenses/ohos-pc-launcher-BSD.txt), [Qt LGPL/GPL](maintenance/licenses/ohshark-Qt-LGPL-NOTICE.md) and [KuiklyUI License](maintenance/licenses/kuiklyui-LICENSE.txt) notices, plus the Electron samples' [MIT](maintenance/licenses/doubao-watermark-lab-MIT.txt) and [Apache-2.0](maintenance/licenses/electron-egg-ohos-Apache-2.0.txt) notices and the [web-IDE](maintenance/licenses/ohos-jupyter-NOTICE.md), [Electron-HAP template](maintenance/licenses/ohos-electron-hap-NOTICE.md) and [KMP source](maintenance/licenses/harmonykmp-NOTICE.md) notices. The other RN samples declare no repository-level license, so only generalized mechanisms are recorded for them. No database-wide license was found in the Flutter source; its original per-project license and evidence context is preserved in provenance. No new blanket license is inferred for the combined database.

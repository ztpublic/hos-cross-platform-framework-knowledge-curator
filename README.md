# HarmonyOS PC adaptation knowledge

Reusable solutions for developers adapting cross-platform applications to HarmonyOS PC. Each entry is a Markdown file in [knowledge/](knowledge/); the structured, searchable catalog is [index.csv](index.csv).

The installed curator is [`harmonyos-pc-knowledge-curator`](/Users/mac/.codex/skills/harmonyos-pc-knowledge-curator/SKILL.md). It supports importing notes, researching new adaptations, updating tracked sources and querying this database for Flutter, Qt, Electron, React Native and Kotlin Multiplatform.

## Classification

- **common**: a platform or native integration contract independent of a particular UI framework. An empty `frameworks` field means no framework-specific requirement, not successful validation on every framework.
- **framework-specific**: a solution dependent on the named framework's APIs, plugins, embedding or build integration.

For example, document URI identity and read/write grants are common; consuming an XFile or mapping QFileDialog suffix filters are framework adapters. General guidance and adapters link to one another.

The database contains **146 entries: 88 common, 29 Flutter-specific, 26 Qt-specific and 3 Electron-specific**. RN and KMP are supported classification and research categories with no framework-specific entries yet. Queries for them still retrieve relevant common knowledge.

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

A source-review pass over two HarmonyOS PC ports added Electron-specific runtime/JIT, native-addon and command-execution entries, plus Qt-specific entrypoint, resource, plugin, linker and packaging entries. Their commit-pinned evidence is under `provenance/snapshots/projects/`, with the screened candidates recorded in `maintenance/runs/`.

**The database validates integrity and retrieval, not app behavior.** The initial import left every entry at `evidence_tier=imported-guidance`; the project-review pass added eight entries at `evidence_tier=source-reviewed`. All entries remain `pc_validation=not-run`. Source-reported builds or device experiments remain attributed historical reports. Each entry's “Check it works” section is an acceptance plan, not a claim those tests ran here.

The Qt collection's [MIT notice](maintenance/licenses/qt-MIT.txt) is retained. No database-wide license was found in the Flutter source; its original per-project license and evidence context is preserved in provenance. No new blanket license is inferred for the combined database.

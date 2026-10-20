---
name: harmonyos-pc-knowledge-curator
description: Summarize and maintain atomic HarmonyOS PC adaptation knowledge from Flutter, Qt, Electron, React Native and Kotlin Multiplatform projects. Use for importing migration notes, researching project adaptations, updating tracked source changes, or querying the CSV-indexed knowledge base; not for performing an application migration.
---

# HarmonyOS PC knowledge curator

Turn adaptation evidence into practical, searchable solutions. Keep one mechanism and independently verifiable outcome per Markdown entry. Classify the mechanism, not the framework of the source project.

## Database and modes

Resolve this skill from the loaded SKILL.md. `config.json` selects the default database, initially `/Users/mac/projects/cross-platform-experiment`. An explicit `--database /absolute/path` overrides it; the shell's working directory does not. Never infer a different destination from the source checkout. Keep mutable knowledge outside this skill.

Use `python3 <skill>/scripts/kb.py --database <db> check|index|stats` and `... search "terms" [--framework flutter|qt|electron|react-native|kotlin-multiplatform] [--scope common|framework-specific]`. Framework searches include common entries by default. The helper uses only Python's standard library.

Select the requested mode:

- **Import/summarize:** extract the supplied notes, databases or projects. Preserve local uncommitted source content by hash; do not substitute HEAD for the actual bytes. Do not fetch or execute source setup/migration workflows merely to import documentation.
- **Discover/analyze:** research the requested framework/project/count. If unspecified, find one previously unstudied project in the requested framework scope. Read [research.md](references/research.md).
- **Update:** inspect only new changes in registered sources; preserve worktrees and incomplete checkpoints. Read [research.md](references/research.md).
- **Query:** search/read matching consumer entries. No source updates or database edits unless requested. State applicability and the evidence boundary.

Before writing, run `check`, inspect `index.csv`, and read [schema.md](references/schema.md) and [authoring.md](references/authoring.md). A new database may be initialized with `init`; never reinitialize or overwrite a populated destination.

## Classify and extract

- `scope: common`: an OS, native-library or integration mechanism usable across frameworks under stated conditions. `frameworks: []` means no framework-specific requirement, not verification on all five frameworks.
- `scope: framework-specific`: the remedy depends on a framework API, embedding, plugin registry, build integration or runtime contract. List the actual framework(s).
- Separate a common invariant from its framework adapter when each is useful independently. For example URI identity, path representation and access grants are common; Flutter XFile consumption or Qt QFileDialog argument mapping are adapter entries.
- Split broad guides by independently actionable failure/solution. Merge duplicate mechanisms and attach multiple evidence records. Keep an unresolved observation as `status: unresolved`; never manufacture a successful remedy.
- Search aliases such as RN/React Native and KMP/Kotlin Multiplatform, but store canonical names. Language/runtime labels such as Rust, C++, ArkTS, JVM or JavaScript belong in keywords/conditions; they are not additional top-level frameworks.

PC applicability must describe the relevant window/input/display/storage/lifecycle boundary. Do not change phone-only evidence into PC verification by renaming a title. Product services, API gates, signing privileges and OpenHarmony versus HarmonyOS differences stay explicit. Electron/RN/KMP are supported curation categories; create no speculative framework entries to fill empty categories.

## Persist and verify

Write consumer Markdown and separate provenance before regenerating `index.csv`. Store provenance snapshots, source revisions and hashes, license context, transformations and limitations outside `knowledge/`. Preserve imported source-reported build/device claims as reports, separately from checks performed during this curation.

Use the schema's run and source records for reviewed ranges and partial outcomes. Advance an analyzed checkpoint only after its complete review and successful database validation. An inaccessible branch retains its prior checkpoint. Do not silently change it.

Run `index`, then `check`, then realistic searches using symptoms and framework filters. Inspect split entries for missing preconditions and broken links. Report added/merged/split counts, common versus framework labels, source coverage, and validation actually performed. Curation authorizes knowledge edits; building, signing, installing or changing source applications requires its own task scope.

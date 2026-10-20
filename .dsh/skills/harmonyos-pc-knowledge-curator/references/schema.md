# Database schema (version 1)

`knowledge/<id>.md` uses JSON frontmatter between `---` delimiters (a YAML-compatible subset, parsed without a YAML dependency). Each file has one consumer solution. Metadata is authoritative; `index.csv` is a deterministic derived index and must not be edited independently.

Required metadata:

- `schema_version`: 1; `id`: stable lowercase kebab-case matching filename.
- `title`, `topic_key` (unique mechanism key), `area`, `summary`, `when_to_use`, `applicability`: nonempty strings.
- `scope`: `common` or `framework-specific`.
- `frameworks`: [] for common, or one or more of `flutter`, `qt`, `electron`, `react-native`, `kotlin-multiplatform` for specific.
- `keywords`, `related`: arrays of strings; related contains existing entry IDs.
- `status`: `scoped` or `unresolved` or `verified`.
- `evidence_tier`: `imported-guidance`, `source-reviewed`, `build-tested`, `device-tested` or `runtime-tested`; this describes curator-performed evidence, not reported source results.
- `pc_validation`: `not-run` or `verified`. Verified requires `pc_test` in provenance with device_class, os_api, build_inputs, behavior, artifact, artifact_sha256; verified status requires pc_validation verified. A historical report stays in source provenance.

The CSV columns are schema_version,id,title,scope,frameworks,area,topic_key,status,evidence_tier,pc_validation,when_to_use,summary,keywords,applicability,related,path,provenance. UTF-8, RFC-style comma quoting, LF newline, one row per entry sorted by id. Array cells use `|`; that character is forbidden inside an array item. No BOM. Use a CSV parser, not split(','). Untrusted source text is never executed as formulas; CSV is a retrieval index, not an executable spreadsheet.

`provenance/<id>.json`: schema_version, id, sources (nonempty list), transformations (list), limitations (list). A source has collection, original_path, snapshot (database-relative), sha256, revision (may be unknown), license and source-reported evidence when available. Full original imported sidecars may be retained as `upstream_provenance` or referenced via a snapshot. Revision alone does not identify dirty worktree content; hash the exact snapshot. Shared snapshots are allowed.

Maintenance data:

- `maintenance/import-map.csv`: collection,source_path,entry_ids,disposition,reason; array IDs use `|`.
- `maintenance/sources.json`: source registry described in research.md.
- `maintenance/runs/*.json`: review outcomes and evidence, never source-app credentials.
- `maintenance/licenses/`: source license notices; an absent license remains unknown, not inferred MIT from another source.
- `provenance/snapshots/`: exact imported evidence; consumer search excludes it.

Use `kb.py init` only for a fresh destination, `index` to regenerate, `check` to validate identity/scope, provenance hashes, index equality and links, `stats` for coverage and `search` for ranked consumer retrieval. `--framework rn` and `--framework kmp` are accepted aliases. A framework filter includes common entries; combine `--scope framework-specific` to inspect adapter-only coverage. Empty categories are honest coverage gaps.

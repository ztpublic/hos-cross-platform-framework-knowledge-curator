# Initial import — 2026-10-20

Created a global `harmonyos-pc-knowledge-curator` skill and a workspace-local knowledge database with 138 consumer Markdown entries and a generated CSV index.

| Scope | Entries |
|---|---:|
| Common platform/native contracts | 88 |
| Flutter-specific adapters | 29 |
| Qt-specific adapters | 21 |
| Electron-specific | 0 |
| React Native-specific | 0 |
| Kotlin Multiplatform-specific | 0 |

## Source coverage

- **Flutter:** all 63 knowledge documents, their original provenance sidecars, and the project registry were retained as exact snapshots. Extraction/splitting produced 88 entries with Flutter-source evidence. Local modifications and untracked knowledge were included by file hash; the checkout was not reset to HEAD.
- **Qt:** 68 source documents contributed to 54 entries, including four shared entries also supported by Flutter material. The import covers every non-index document in the source `problems/` directory, selected semantic guides, and the embedded common HarmonyOS collection.
- Counts overlap where evidence was merged. The import does not claim exhaustive extraction of every concept in the larger Qt migration guides. [Qt source inventory](qt-source-inventory.json) identifies incorporated and not-extracted documents; [import-map.csv](import-map.csv) maps each incorporated source to all resulting atoms.
- Source checkouts were read only. No source setup scripts, migration steps, builds, signing, installation or device operations were executed.

## Content changes

Classification follows the mechanism, not the source project's framework. URI/path/grant semantics, sandbox storage, native dependency closure, JIT authorization, ABI details and surface lifetimes are common contracts. QFileDialog filter mapping, QPA startup coherence, Flutter plugin registration and renderer ownership remain adapters.

Broad Flutter topics were separated into independent entries, including shared database access, typed preference migration, numeric codec normalization, batch partial commits, encoded-byte page budgets, upload resumption, native URI streaming, density/inset updates and player reconstruction.

Qt material was rewritten as symptoms, recommended approaches, implementation steps, conditions, mistakes and observable checks. Source-specific identities and historical evidence remain in provenance. Corrections include:

- Do not promote empty native stubs or copied system libraries as a general dependency repair.
- Do not infer a fixed modules/modulesFactories mapping solely from a Qt version label.
- Preserve resource linkage and retention requirements rather than deleting Q_INIT_RESOURCE unconditionally.
- Treat missing SDK modules and historical build-host restrictions as scoped observations.
- Keep stream and codec time bases distinct when replacing removed multimedia APIs.
- Mark the precise mixed-surface API compatibility boundary unresolved.

## Validation

- Skill Creator's `quick_validate.py`: passed. PyYAML was installed only in an isolated temporary validation environment; normal database tools use the Python standard library.
- Final database check: 138 metadata/provenance pairs; SHA-256 snapshot integrity, relative links, unique identifiers/topics and exact CSV regeneration passed.
- Coverage checks: all 63 original Flutter filenames mapped; every map target exists; visible titles match metadata.
- Source-preservation check: imported snapshot hashes still matched the original local files.
- Cwd-independent queries from `/tmp`: passed. RN queries returned common document-grant knowledge and excluded Qt adapters. Qt modulesFactories query returned the Qt startup-contract entry. KMP adapter-only queries correctly found no entries.
- Independent forward-test: 13 expected command outcomes passed on an isolated earlier 123-entry copy. It tested semantic merging of a synthetic Qt note into existing common/adaptor entries, framework filtering, and rejection of populated initialization, invalid scope, unsupported verification claims, tampered snapshots and stale CSV. Its finding led to explicit per-file-hash collection update rules. See [results](runs/2026-10-20-forward-test.json).

All entries remain `evidence_tier=imported-guidance` and `pc_validation=not-run`. Original source claims are attributed reports; consumer acceptance checks have not been executed on HarmonyOS PC during this import.

## Licensing and maintenance

The Qt MIT notice is preserved in [licenses/qt-MIT.txt](licenses/qt-MIT.txt). The Flutter source had no database-level license file; original per-project license context remains in the imported provenance. No blanket license is assigned to the combined collection.

The one-time recipe is under [import/](import/). It refuses to overwrite an existing database when directly invoked. Routine changes should use the curator workflow, edit authoritative Markdown/provenance, then run `index` and `check`; do not rerun the initial import over later curation.

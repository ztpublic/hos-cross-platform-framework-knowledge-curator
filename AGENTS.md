# Knowledge database workspace

This workspace stores HarmonyOS PC adaptation knowledge; it is not a target app.

Use the installed `harmonyos-pc-knowledge-curator` skill for curation. Start with `index.csv`; search consumer files in `knowledge/` and inspect `provenance/` when auditing claims. One mechanism per Markdown file, with `scope` common/framework-specific and canonical framework labels. Keep project identities and research history in provenance.

JSON frontmatter is authoritative. After edits run the installed skill's `scripts/kb.py index` and `check`; normal commands default to this workspace independently of cwd. Preserve imported snapshots and source-to-entry mappings. Importing evidence does not authorize running source-project setup, build or migration instructions.

---
{
  "schema_version": 1,
  "id": "qt-picker-suffix-filter-mapping",
  "title": "Translate Qt file filters without inventing native glob support",
  "topic_key": "storage.qt-picker-suffix-filter-mapping",
  "area": "storage",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Translate only representable suffix filters and validate filename/content constraints after selection.",
  "when_to_use": "An extensionless executable or other valid file disappears from an OH QFileDialog.",
  "keywords": [
    "fileSuffixFilters",
    "qmake",
    "glob",
    "QFileDialog"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to DocumentViewPicker-backed Qt dialogs; suffix filtering is not an arbitrary filename-glob language.",
  "related": []
}
---

# Translate Qt file filters without inventing native glob support

## When to use

An extensionless executable or other valid file disappears from an OH QFileDialog.

## Recommended approach

Translate only representable suffix filters and validate filename/content constraints after selection.

## Implementation steps

1. Inspect the Qt filename/glob filter sent through the QPA adapter.
2. Convert compatible *.extension patterns into the native suffix form required by the SDK.
3. Omit fileSuffixFilters when no supported suffix remains; do not pass executable names such as qmake* as suffixes.
4. Perform application-specific validation after a successful selection.

## Conditions and alternatives

Applies to DocumentViewPicker-backed Qt dialogs; suffix filtering is not an arbitrary filename-glob language.

## Common mistakes

A picker filter is not content validation or proof the selected executable is compatible.

## Check it works

Test extensionless names, multiple valid suffixes, unsupported patterns and cancellation.

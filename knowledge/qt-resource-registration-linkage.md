---
{
  "schema_version": 1,
  "id": "qt-resource-registration-linkage",
  "title": "Keep Qt resource initialization compatible with the exported entrypoint",
  "topic_key": "build.qt-resource-registration-linkage",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Compare C/C++ linkage and resource inclusion; keep explicit initialization in a C++ helper when it is required.",
  "when_to_use": "Linking an OH shared-library entry shim reports an undefined qInitResources symbol.",
  "keywords": [
    "Q_INIT_RESOURCE",
    "qInitResources",
    "AUTORCC",
    "extern C"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to Qt apps loaded as native shared libraries. Static archives and linker retention can require explicit resource references.",
  "related": []
}
---

# Keep Qt resource initialization compatible with the exported entrypoint

## When to use

Linking an OH shared-library entry shim reports an undefined qInitResources symbol.

## Recommended approach

Compare C/C++ linkage and resource inclusion; keep explicit initialization in a C++ helper when it is required.

## Implementation steps

1. Inspect generated resource object symbols and the reference emitted by the exported entry shim.
2. Ensure qrc inputs are compiled and linked, using the appropriate AUTORCC or Qt resource helper.
3. If explicit initialization is needed, call Q_INIT_RESOURCE from a C++-linkage helper outside the C-exported entry definition.
4. Omit redundant explicit initialization only after confirming resource objects and their registration actually survive the link.

## Conditions and alternatives

Applies to Qt apps loaded as native shared libraries. Static archives and linker retention can require explicit resource references.

## Common mistakes

Deleting Q_INIT_RESOURCE blindly can trade a link error for missing icons/QML/translations.

## Check it works

Inspect symbols and test access to representative resources in the packaged app.

---
{
  "schema_version": 1,
  "id": "qt-native-entry-initialization",
  "title": "Preserve application initialization when adapting the native entrypoint",
  "topic_key": "architecture.qt-native-entry-initialization",
  "area": "architecture",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Preserve the original application initialization contract before constructing its UI.",
  "when_to_use": "A minimal OH entry shim launches a window with missing domain objects and crashes immediately.",
  "keywords": [
    "entry shim",
    "nullptr",
    "startup",
    "SIGSEGV"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to Qt entrypoint migration; a placeholder window is not the original application running successfully.",
  "related": []
}
---

# Preserve application initialization when adapting the native entrypoint

## When to use

A minimal OH entry shim launches a window with missing domain objects and crashes immediately.

## Recommended approach

Preserve the original application initialization contract before constructing its UI.

## Implementation steps

1. Trace required configuration, services and model objects from the original entrypoint to the initial window.
2. Adapt only the platform entry/export boundary while retaining valid initialization order and error handling.
3. Do not replace unavailable dependencies with null objects unless the consumer explicitly supports that state.
4. Fail with a visible recoverable initialization result when a required service cannot be created.

## Conditions and alternatives

Applies to Qt entrypoint migration; a placeholder window is not the original application running successfully.

## Common mistakes

A non-null QApplication and a constructed main window do not establish a valid domain model.

## Check it works

Test first launch, invalid configuration and normal startup through the real application entrypoint.

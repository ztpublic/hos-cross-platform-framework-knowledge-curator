---
{
  "schema_version": 1,
  "id": "qt-qpa-runtime-template-coherence",
  "title": "Align Qt runtime, QPA and ArkTS startup templates",
  "topic_key": "build.qt-qpa-runtime-template-coherence",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Use a compatible runtime/QPA/template set and inspect its actual startup contract before changing property names.",
  "when_to_use": "Startup crashes with missing modules/modulesFactories, deviceInfo or undefined QPA handlers after replacing one native artifact.",
  "keywords": [
    "modulesFactories",
    "modules",
    "deviceInfo",
    "libqohos",
    "NAPI"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Qt branch/version labels alone are insufficient; different snapshots can expect opposite setup forms. Qt 5 and Qt 6 integration APIs differ.",
  "related": []
}
---

# Align Qt runtime, QPA and ArkTS startup templates

## When to use

Startup crashes with missing modules/modulesFactories, deviceInfo or undefined QPA handlers after replacing one native artifact.

## Recommended approach

Use a compatible runtime/QPA/template set and inspect its actual startup contract before changing property names.

## Implementation steps

1. Identify exact QtCore, libqohos and ArkTS template artifacts and compare their exported/required symbols.
2. Inspect which setup object the installed QPA reads: modules or modulesFactories, and whether values are objects or factories.
3. Align the whole template and its module exports/import timing with that implementation, preserving app resources, configuration and signing inputs.
4. Check APP_LIBRARY_NAME against the packaged native artifact; rebuild/repackage all changed components and inspect final hashes.

## Conditions and alternatives

Qt branch/version labels alone are insufficient; different snapshots can expect opposite setup forms. Qt 5 and Qt 6 integration APIs differ.

## Common mistakes

Do not blindly rename modules to modulesFactories or replace one libqohos against an unrelated QtCore.

## Check it works

Test a clean launch, warm Want delivery and module-backed features; inspect native fault logs as well as process survival.

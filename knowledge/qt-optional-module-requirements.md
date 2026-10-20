---
{
  "schema_version": 1,
  "id": "qt-optional-module-requirements",
  "title": "Separate missing Qt modules from unnecessary example dependencies",
  "topic_key": "build.qt-optional-module-requirements",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Inspect the installed target modules and determine whether the missing module is required by shipping code or only optional examples.",
  "when_to_use": "Qt package discovery fails on UiTools or falls back to the wrong Qt major version.",
  "keywords": [
    "UiTools",
    "QUiLoader",
    "Qt4",
    "find_package"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Module presence depends on the Qt distribution and build route. Historical host-build limitations are not proof that every future cross-build is impossible.",
  "related": []
}
---

# Separate missing Qt modules from unnecessary example dependencies

## When to use

Qt package discovery fails on UiTools or falls back to the wrong Qt major version.

## Recommended approach

Inspect the installed target modules and determine whether the missing module is required by shipping code or only optional examples.

## Implementation steps

1. List actual target package configs and compare them with the requested components.
2. Trace QUiLoader or other affected API usage before removing a requirement.
3. Disable optional examples/features that alone require the module; if shipping behavior needs it, obtain/build a compatible module through a verified supported route.
4. Report the unresolved capability when the needed module is unavailable rather than silently selecting an older Qt major.

## Conditions and alternatives

Module presence depends on the Qt distribution and build route. Historical host-build limitations are not proof that every future cross-build is impossible.

## Common mistakes

A module marked supported upstream may still be absent from an installed SDK; deleting its link target does not replace runtime UI loading.

## Check it works

Configure core and optional targets independently and execute any retained module-backed feature.

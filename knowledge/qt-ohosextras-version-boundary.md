---
{
  "schema_version": 1,
  "id": "qt-ohosextras-version-boundary",
  "title": "Resolve QtOhosExtras against the actual Qt generation",
  "topic_key": "build.qt-ohosextras-version-boundary",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Inspect installed headers and build metadata instead of carrying Qt 5 public Extras recipes into Qt 6.",
  "when_to_use": "QtOhosExtras package discovery, headers or enums fail after changing the Qt OH SDK.",
  "keywords": [
    "QtOhosExtras",
    "qmake",
    "CloseEventRootCause",
    "CamelCase"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "The imported examples concern particular Qt 5.12/5.15 and Qt 6 OH distributions; module organization is not universal.",
  "related": []
}
---

# Resolve QtOhosExtras against the actual Qt generation

## When to use

QtOhosExtras package discovery, headers or enums fail after changing the Qt OH SDK.

## Recommended approach

Inspect installed headers and build metadata instead of carrying Qt 5 public Extras recipes into Qt 6.

## Implementation steps

1. For a Qt 5 package lacking generated CMake configs, use its supported qmake integration or an explicitly verified manual link contract.
2. Use installed header names; some distributions expose lowercase qohos headers without CamelCase forwarding headers.
3. Qualify scoped enums and locate the owning class/free function in actual headers.
4. For Qt 6, locate the equivalent integration API and its public/private status; version-pin private APIs if unavoidable.

## Conditions and alternatives

The imported examples concern particular Qt 5.12/5.15 and Qt 6 OH distributions; module organization is not universal.

## Common mistakes

Do not invent a CMake target/library suffix or copy a Qt 5 close enum into Qt 6.

## Check it works

Compile a minimal use against the installed headers, inspect the linked artifact and test the requested native capability.

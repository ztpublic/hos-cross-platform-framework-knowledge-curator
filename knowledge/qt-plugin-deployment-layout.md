---
{
  "schema_version": 1,
  "id": "qt-plugin-deployment-layout",
  "title": "Package Qt plugins in the loader layout expected by the selected SDK",
  "topic_key": "build.qt-plugin-deployment-layout",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Deploy the selected Qt runtime and categorized plugins with their dependency closure and expected search layout.",
  "when_to_use": "The app cannot load QPA, QSQLITE, image formats or the OH style despite packaging QtCore.",
  "keywords": [
    "QPA",
    "QSQLITE",
    "Driver not loaded",
    "libqsqlite.so",
    "sqldrivers",
    "styles"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Qt 5 source examples use styles, sqldrivers and imageformats directories; exact names and deployment helpers must match the selected Qt generation.",
  "related": [
    "bundle-data-write-location",
    "native-dependency-closure"
  ]
}
---

# Package Qt plugins in the loader layout expected by the selected SDK

## When to use

The app cannot load QPA, QSQLITE, image formats or the OH style despite packaging QtCore.

## Recommended approach

Deploy the selected Qt runtime and categorized plugins with their dependency closure and expected search layout.

## Implementation steps

1. Link/integrate the selected QOhosPlatformIntegrationPlugin target where that SDK exposes it and inspect libqohos in the HAP.
2. Determine the SDK's actual SQL/style/image plugin filenames and copy/integrate them into the loader's category directories when automatic deployment does not.
3. Inspect plugin dependencies, including DBus where the selected QPA requires it.
4. Diagnose QSqlDatabase driver availability separately from database-path writability; keep native runtime versions coherent.

## Conditions and alternatives

Qt 5 source examples use styles, sqldrivers and imageformats directories; exact names and deployment helpers must match the selected Qt generation.

## Common mistakes

Do not assume desktop plugin filenames, that linking QtSql deploys its driver, or that a missing QPA dependency is an application-code crash.

## Check it works

Inspect package contents and runtime plugin diagnostics; enumerate SQL drivers, open a writable database and exercise required image/style plugins.

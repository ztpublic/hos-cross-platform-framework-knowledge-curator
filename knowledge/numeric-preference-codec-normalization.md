---
{
  "schema_version": 1,
  "id": "numeric-preference-codec-normalization",
  "title": "Normalize numeric preferences at the codec boundary",
  "topic_key": "storage.numeric-preference-codec-normalization",
  "area": "storage",
  "scope": "common",
  "frameworks": [],
  "summary": "Accept verified finite numeric representations at the read boundary while preserving validation and legitimate zero.",
  "when_to_use": "A persisted integral double returns as an integer and breaks a typed preference consumer.",
  "keywords": [
    "SQLite",
    "sqflite",
    "Drift",
    "drift_sqflite",
    "SqfliteQueryExecutor",
    "getDatabasesPath",
    "SharedPreferences",
    "JSON",
    "数据库",
    "migration",
    "SQLite load error",
    "sqflite OH",
    "database migration",
    "数据库后端切换"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies when the resolved storage/codec actually changes numeric representation; it is not permission to coerce arbitrary values.",
  "related": [
    "shared-database-extension-contract"
  ]
}
---

# Normalize numeric preferences at the codec boundary

## When to use

A persisted integral double returns as an integer and breaks a typed preference consumer.

## Recommended approach

Accept verified finite numeric representations at the read boundary while preserving validation and legitimate zero.

## Implementation steps

For cached numeric preferences, normalize an accepted finite `num` into the domain's double at the read boundary when a verified codec/backend round trip can change integral doubles to ints. Do not erase all preferences or repeatedly write `72.0` hoping to fix representation. Validate the range and distinguish missing, invalid and legitimate zero values. Treat this as a compatibility adapter for the observed backend, not a reason to accept arbitrary types globally.

## Conditions and alternatives

Applies when the resolved storage/codec actually changes numeric representation; it is not permission to coerce arbitrary values.

## Common mistakes

Rewriting the same numeric literal repeatedly does not fix a codec representation change.

## Check it works

Round-trip fractional and integral values, zero, missing data, strings, nonfinite numbers and out-of-range input.

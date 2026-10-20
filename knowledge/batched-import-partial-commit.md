---
{
  "schema_version": 1,
  "id": "batched-import-partial-commit",
  "title": "Make partial commit and resume explicit for batched imports",
  "topic_key": "storage.batched-import-partial-commit",
  "area": "storage",
  "scope": "common",
  "frameworks": [],
  "summary": "Use bounded sequential batches and a durable progress policy rather than claiming whole-import rollback.",
  "when_to_use": "An import exceeds a native store transaction limit or fails after earlier batches were committed.",
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
  "applicability": "Applies when the actual store imposes per-commit limits. Whole-import atomicity needs a separate staging/publication design.",
  "related": [
    "database-backend-and-data-contract"
  ]
}
---

# Make partial commit and resume explicit for batched imports

## When to use

An import exceeds a native store transaction limit or fails after earlier batches were committed.

## Recommended approach

Use bounded sequential batches and a durable progress policy rather than claiming whole-import rollback.

## Implementation steps

A host's per-commit record limit may require sequential batches for large imports. That makes each batch atomic under its own contract, not the whole import. Preserve stable record identities, conflict checks and a retry/resume policy; test failure after an earlier batch committed and before final metadata publication. Do not describe a partially committed import as rolled back merely because the last batch failed.

## Conditions and alternatives

Applies when the actual store imposes per-commit limits. Whole-import atomicity needs a separate staging/publication design.

## Common mistakes

The last batch failing does not roll back earlier committed batches.

## Check it works

Fail before, during and after a batch commit; verify stable record IDs, conflict handling and idempotent resume without duplicate publication.

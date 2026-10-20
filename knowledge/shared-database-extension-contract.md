---
{
  "schema_version": 1,
  "id": "shared-database-extension-contract",
  "title": "Share a database with a native extension deliberately",
  "topic_key": "storage.shared-database-extension-contract",
  "area": "storage",
  "scope": "common",
  "frameworks": [],
  "summary": "Verify physical store identity and configuration, use one migration owner and scope extension access.",
  "when_to_use": "A native extension opens an empty or incompatible store instead of application data.",
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
  "applicability": "Requires a native extension and shared runtime that can access the same physical database with compatible schema, configuration and permissions.",
  "related": [
    "database-backend-and-data-contract",
    "numeric-preference-codec-normalization"
  ]
}
---

# Share a database with a native extension deliberately

## When to use

A native extension opens an empty or incompatible store instead of application data.

## Recommended approach

Verify physical store identity and configuration, use one migration owner and scope extension access.

## Implementation steps

The shared-layer filename/path and a native `relationalStore.StoreConfig.name` must identify the same physical store through the resolved plugin's mapping; comparing just basenames is insufficient. Record and verify path mapping, security/tokenizer configuration, schema version and access permissions before enabling an independent reader. Use read-only extension access when appropriate, retain one migration owner, and close cursors and stores on every outcome. Handle a missing store distinctly from malformed rows or a failed open.

See [Normalize numeric preferences at the codec boundary](numeric-preference-codec-normalization.md).

## Conditions and alternatives

Requires a native extension and shared runtime that can access the same physical database with compatible schema, configuration and permissions.

If native preferences and a file fallback coexist, define which copy is authoritative and record a revision for reconciliation. A timed-out channel write can still finish later; a nonempty native readback can be older than the latest file save. Validate the expected revision/content, preserve valid empty collections and distinguish missing data from corruption or read failure.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Populate through the primary owner, read through the native extension, and verify identical records/schema. Cover an absent store, mismatched paths, open failure, migration and simultaneous access.

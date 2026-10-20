---
{
  "schema_version": 1,
  "id": "typed-preference-schema-migration",
  "title": "Migrate string-encoded preferences to native JSON values by schema",
  "topic_key": "storage.typed-preference-schema-migration",
  "area": "storage",
  "scope": "common",
  "frameworks": [],
  "summary": "Migrate only schema-declared keys and publish a complete validated snapshot with explicit version reconciliation.",
  "when_to_use": "Preferences change from encoded strings to typed values or synchronize between schema versions.",
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
  "applicability": "A preferences/JSON store may be sufficient for small simple state, but it is not automatically equivalent to SQLite. A missing package implementation does not imply the OS cannot support SQLite.",
  "related": [
    "database-backend-and-data-contract",
    "sqlite-read-write-transaction-contention"
  ]
}
---

# Migrate string-encoded preferences to native JSON values by schema

## When to use

Preferences change from encoded strings to typed values or synchronize between schema versions.

## Recommended approach

Migrate only schema-declared keys and publish a complete validated snapshot with explicit version reconciliation.

## Implementation steps

Give each preference domain one schema/migration owner. When replacing a map of encoded strings with typed JSON values, convert only keys whose historical type is declared. Preserve ordinary text that happens to look like JSON; a generic parse-every-string rule changes legitimate content. Keep missing keys, explicit null, empty collections and decoding failures distinct, and write the new schema version only with a successfully converted snapshot.

If those preferences synchronize, version the operation payload as well as the local file. Define how old-format operations and mismatched schema versions are rejected, translated or reconciled; a new local decoder alone cannot make an old peer compatible. Keep a multi-key migration or invariant-preserving edit atomic at the store's declared transaction boundary.

Route replacement writes through the actual OH storage host's complete-write/replace implementation, preserving private-file access policy and updating caches/observers only after the persistence contract succeeds. A method named atomic does not by itself prove crash durability: check its flush, rename, error and replay behavior. Keep corrupt originals recoverable rather than treating a migration error as an empty preference set.

When replacing a complete snapshot, validate and migrate the old state in memory before the final replacement. Persisting a schema-only intermediate snapshot can leave durable metadata changed even when the complete replacement fails. Verify both persisted bytes and cached state after an injected write failure.

For independent native SQLite connections that read and then write, also check [transaction-upgrade contention](sqlite-read-write-transaction-contention.md); selecting the correct engine does not determine the transaction mode or busy-handler behavior.

## Conditions and alternatives

A preferences/JSON store may be sufficient for small simple state, but it is not automatically equivalent to SQLite. A missing package implementation does not imply the OS cannot support SQLite.

If native preferences and a file fallback coexist, define which copy is authoritative and record a revision for reconciliation. A timed-out channel write can still finish later; a nonempty native readback can be older than the latest file save. Validate the expected revision/content, preserve valid empty collections and distinguish missing data from corruption or read failure.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Migrate valid legacy strings, ordinary JSON-looking text, explicit null, empty collections and invalid values. Inject publication failure and verify persisted bytes/caches remain consistent; test mixed-version peers.

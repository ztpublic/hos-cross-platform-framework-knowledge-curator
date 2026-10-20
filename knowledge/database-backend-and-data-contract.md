---
{
  "schema_version": 1,
  "id": "database-backend-and-data-contract",
  "topic_key": "storage.database-backend-and-data-contract",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "title": "Choose or replace a database backend without losing data",
  "area": "storage",
  "summary": "Keep the storage interface stable, but evaluate engine loading, semantics and existing-data migration separately from package selection.",
  "when_to_use": "A Dart database layer cannot load SQLite on OH, or you are considering a preferences/JSON fallback while an OH database plugin becomes available.",
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
  "applicability": "Flutter OH with the selected embedding/plugin versions; PC behavior requires the stated acceptance checks.",
  "related": [
    "batched-import-partial-commit",
    "shared-database-extension-contract",
    "typed-preference-schema-migration"
  ]
}
---

# Choose or replace a database backend without losing data

## When to use

A Dart database layer cannot load SQLite on OH, or you are considering a preferences/JSON fallback while an OH database plugin becomes available.

## Recommended approach

Keep the storage interface stable, but evaluate engine loading, semantics and existing-data migration separately from package selection.

## Implementation steps

1. Trace the database API to its actual native engine: platform plugin, bundled native library, FFI loader or a library embedded in another native module.
2. Verify that engine is built and packaged for OH and receives an app-writable database location.
3. If replacing the engine temporarily, list the semantics the fallback must preserve: transactions, queries, concurrency, data types and failure recovery.
4. Version the stored format and implement migration for users who already wrote data through the old backend. Detect which format exists before opening a new empty database.
5. Switch the backend only after the migration and rollback/error behavior are defined.

For a small JSON fallback, serialize initialization and writes, persist through a temporary file and replace only after a complete write, and preserve a corrupt original for recovery. A read exception must not silently become an empty authoritative database that the next edit overwrites. Separately test interruption during writing and simultaneous edits; awaiting each caller's write does not serialize different callers.

For a Drift application, one scoped alternative to `drift/native.dart` is an OH-capable `sqflite` implementation reached through `drift_sqflite`. Keep the generated database and repository interface, and change the executor construction inside `LazyDatabase`. A compatible `SqfliteQueryExecutor.inDatabaseFolder` uses the backend's database-folder convention. Confirm the resolved bridge/plugin versions support the operations you need before selecting this route.

Treat the old document-directory location and the new database-directory location as separate inputs to migration, even if the filename is identical. Resolve both with their respective APIs; if existing data is present, close active connections and use a SQLite-safe backup or transfer procedure before opening the destination. Copying only the main file while a connection or WAL writes remain active can lose data. Validate schema and content before considering the migration complete.

## Conditions and alternatives

A preferences/JSON store may be sufficient for small simple state, but it is not automatically equivalent to SQLite. A missing package implementation does not imply the OS cannot support SQLite.

If native preferences and a file fallback coexist, define which copy is authoritative and record a revision for reconciliation. A timed-out channel write can still finish later; a nonempty native readback can be older than the latest file save. Validate the expected revision/content, preserve valid empty collections and distinguish missing data from corruption or read failure.

See [Share a database with a native extension deliberately](shared-database-extension-contract.md) for this independent contract.

See [Migrate string-encoded preferences to native JSON values by schema](typed-preference-schema-migration.md) for this independent contract.

## Common mistakes

See [Make partial commit and resume explicit for batched imports](batched-import-partial-commit.md).

Changing a backend selector does not migrate existing data. A Dart abstraction compiling says nothing about native-library availability or crash durability.

Feature synchronization can restore a default FFI executor while OH plugin dependencies remain selected. Review executor imports, construction and database location together after upstream changes; a dependency lock alone does not preserve the chosen storage route.

## Check it works

Test old-data migration, transactions, concurrent writes, restart, corrupt data and interrupted migration in a release app using the actual native backend.

Also test an upgrade with a populated database in the old location and confirm that subsequent restarts use the same migrated data rather than creating another empty database.

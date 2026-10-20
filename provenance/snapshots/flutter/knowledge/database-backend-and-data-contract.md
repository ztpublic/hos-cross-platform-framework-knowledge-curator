---
{
  "id": "database-backend-and-data-contract",
  "title": "Choose or replace a database backend without losing data",
  "topic_key": "storage.database-backend-and-data-contract",
  "area": "storage",
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
  "summary": "Keep the storage interface stable, but evaluate engine loading, semantics and existing-data migration separately from package selection.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "A Dart database layer cannot load SQLite on OH, or you are considering a preferences/JSON fallback while an OH database plugin becomes available."
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

### Share a database with a native extension deliberately

The Dart filename/path and a native `relationalStore.StoreConfig.name` must identify the same physical store through the resolved plugin's mapping; comparing just basenames is insufficient. Record and verify path mapping, security/tokenizer configuration, schema version and access permissions before enabling an independent reader. Use read-only extension access when appropriate, retain one migration owner, and close cursors and stores on every outcome. Handle a missing store distinctly from malformed rows or a failed open.

For cached numeric preferences, normalize an accepted finite `num` into the domain's double at the read boundary when a verified codec/backend round trip can change integral doubles to ints. Do not erase all preferences or repeatedly write `72.0` hoping to fix representation. Validate the range and distinguish missing, invalid and legitimate zero values. Treat this as a compatibility adapter for the observed backend, not a reason to accept arbitrary types globally.

### Migrate string-encoded preferences to native JSON values by schema

Give each preference domain one schema/migration owner. When replacing a map of encoded strings with typed JSON values, convert only keys whose historical type is declared. Preserve ordinary text that happens to look like JSON; a generic parse-every-string rule changes legitimate content. Keep missing keys, explicit null, empty collections and decoding failures distinct, and write the new schema version only with a successfully converted snapshot.

If those preferences synchronize, version the operation payload as well as the local file. Define how old-format operations and mismatched schema versions are rejected, translated or reconciled; a new local decoder alone cannot make an old peer compatible. Keep a multi-key migration or invariant-preserving edit atomic at the store's declared transaction boundary.

Route replacement writes through the actual OH storage host's complete-write/replace implementation, preserving private-file access policy and updating caches/observers only after the persistence contract succeeds. A method named atomic does not by itself prove crash durability: check its flush, rename, error and replay behavior. Keep corrupt originals recoverable rather than treating a migration error as an empty preference set.

When replacing a complete snapshot, validate and migrate the old state in memory before the final replacement. Persisting a schema-only intermediate snapshot can leave durable metadata changed even when the complete replacement fails. Verify both persisted bytes and cached state after an injected write failure.

For independent native SQLite connections that read and then write, also check [transaction-upgrade contention](sqlite-read-write-transaction-contention.md); selecting the correct engine does not determine the transaction mode or busy-handler behavior.

## Common mistakes

A host's per-commit record limit may require sequential batches for large imports. That makes each batch atomic under its own contract, not the whole import. Preserve stable record identities, conflict checks and a retry/resume policy; test failure after an earlier batch committed and before final metadata publication. Do not describe a partially committed import as rolled back merely because the last batch failed.

Changing a backend selector does not migrate existing data. A Dart abstraction compiling says nothing about native-library availability or crash durability.

Feature synchronization can restore a default FFI executor while OH plugin dependencies remain selected. Review executor imports, construction and database location together after upstream changes; a dependency lock alone does not preserve the chosen storage route.

## Check it works

Test old-data migration, transactions, concurrent writes, restart, corrupt data and interrupted migration in a release app using the actual native backend.

Also test an upgrade with a populated database in the old location and confirm that subsequent restarts use the same migrated data rather than creating another empty database.

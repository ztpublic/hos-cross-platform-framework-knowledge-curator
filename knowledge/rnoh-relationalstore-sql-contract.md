---
{
  "schema_version": 1,
  "id": "rnoh-relationalstore-sql-contract",
  "title": "Preserve SQL semantics in an RNOH relationalStore adapter",
  "topic_key": "storage.rnoh-relationalstore-sql-contract",
  "area": "storage",
  "scope": "framework-specific",
  "frameworks": [
    "react-native"
  ],
  "summary": "Implement a narrow UITurboModule SQL contract over relationalStore with explicit result types, transactions and schema-version metadata rather than treating it as a transparent SQLite driver.",
  "when_to_use": "An RN database abstraction needs an OH backend but its expected SQL/query/version operations do not map directly onto the selected relationalStore surface.",
  "applicability": "RNOH applications with an app-owned SQL TurboModule using UIAbilityContext and OH RdbStore. PC database persistence and concurrent RN windows require separate tests; SQL support is API/backend specific.",
  "keywords": [
    "UITurboModule",
    "relationalStore",
    "RdbStore",
    "querySql",
    "executeSql",
    "ResultSet",
    "schema version",
    "transaction"
  ],
  "related": [
    "rnoh-native-host-and-codegen",
    "sqlite-read-write-transaction-contention",
    "batched-import-partial-commit"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Preserve SQL semantics in an RNOH relationalStore adapter

## When to use

An RN database abstraction needs an OH backend but its expected SQL/query/version operations do not map directly onto the selected relationalStore surface.

## Recommended approach

Implement a narrow UITurboModule SQL contract over relationalStore with explicit result types, transactions and schema-version metadata rather than treating it as a transparent SQLite driver.

## Implementation steps

1. Register the exact JS module name in the ArkTS UITurboModule factory and match the host RNOH package/version; obtain the store from the owning UIAbilityContext.
2. Define query row/value conversion and execute semantics explicitly. Close every ResultSet in finally, including conversion failures.
3. If the selected abstraction cannot use its expected version PRAGMA, maintain a dedicated metadata table and update version plus schema changes within the same migration transaction.
4. Expose begin/commit/rollback as an owned transaction contract. Serialize migration and connection initialization; reject conflicting operations.
5. Accept structured statements/parameters for batches or use a real SQL parser. Never copy a `split(";")` script executor into a general adapter.

## Conditions and alternatives

The source demonstrates registration, query conversion and metadata-table routing. Its PRAGMA comment is a source-specific compatibility claim, not a platform-wide prohibition. Its split-based batches, initialization races and success-only ResultSet cleanup need hardening.

## Common mistakes

A SQL-looking API does not guarantee SQLite PRAGMA, migration or transaction equivalence. Semicolons in strings/triggers make naive script splitting incorrect.

## Check it works

Test typed null/blob/numeric values, query failure cleanup, simultaneous open calls, migration rollback, semicolons inside values and reopening after process death.

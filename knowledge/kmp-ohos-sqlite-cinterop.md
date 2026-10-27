---
{
  "schema_version": 1,
  "id": "kmp-ohos-sqlite-cinterop",
  "title": "Link the SQLite implementation behind an OH KMP binding",
  "topic_key": "storage.kmp-ohos-sqlite-cinterop",
  "area": "storage",
  "scope": "framework-specific",
  "frameworks": [
    "kotlin-multiplatform"
  ],
  "summary": "Treat the OH Kotlin target, cinterop declarations and real SQLite native implementation as one dependency chain; a klib or shared target declaration does not package sqlite3 automatically.",
  "when_to_use": "A KMP SQLite API compiles but the OH host fails on sqlite3 symbols or cannot load the SQLite shared library.",
  "applicability": "Kotlin/Native OH targets using SQLite C interop, whether bundled statically or through a separately packaged OH library. PC database paths and concurrent-window access need independent validation.",
  "keywords": [
    "sqlite3",
    "cinterop",
    "ohosArm64",
    "linkerOpts",
    "headers",
    "klib",
    "libsqlite3.so",
    "Kotlin Native"
  ],
  "related": [
    "database-backend-and-data-contract",
    "native-dependency-closure",
    "sqlite-read-write-transaction-contention",
    "kmp-ohos-compose-target"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Link the SQLite implementation behind an OH KMP binding

## When to use

A KMP SQLite API compiles but the OH host fails on sqlite3 symbols or cannot load the SQLite shared library.

## Recommended approach

Treat the OH Kotlin target, cinterop declarations and real SQLite native implementation as one dependency chain; a klib or shared target declaration does not package sqlite3 automatically.

## Implementation steps

1. Build the actual OH target and connect its source set to the native driver; Android JNA and iOS system SQLite are different backends.
2. Point cinterop at the correct sqlite3 headers and inspect def-file linker options. Choose either a built static archive or a real ABI-compatible SQLite shared library.
3. For dynamic linkage, include the native SQLite library in the host dependency/package closure and ensure its requested loader name and exports match the binding.
4. Keep database files and WAL/SHM sidecars in writable app storage. Preserve prepared-statement, string/blob ownership and connection/thread policy separately from packaging.

## Conditions and alternatives

The two reviewed libraries demonstrate different integration shapes; they are not interchangeable binary dependencies. One embeds the bundled SQLite implementation in its cinterop declaration; the other documents dynamic host packaging. The latter's Gradle linker option names `-lsqlite3.h`, which needs correction/verification against the real library name before reuse. Source-reported multiplatform support does not establish PC execution.

## Common mistakes

Shipping only a klib or assuming the OH image provides Android/iOS SQLite imports leaves the final native dependency unresolved.

## Check it works

Inspect final native dependencies and run create/insert/query/close/reopen on the intended PC, including WAL sidecars, transaction failure and the selected concurrent-access policy.

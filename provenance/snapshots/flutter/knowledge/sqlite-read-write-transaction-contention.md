---
{
  "schema_version": 2,
  "id": "sqlite-read-write-transaction-contention",
  "title": "Avoid SQLite read-to-write upgrade failures despite a busy timeout",
  "topic_key": "storage.sqlite-read-write-transaction-contention",
  "area": "storage",
  "keywords": ["SQLite", "SQLITE_BUSY", "SQLITE_BUSY_SNAPSHOT", "BEGIN IMMEDIATE", "busy_timeout", "rusqlite", "TransactionBehavior", "WAL", "read modify write", "database locked", "数据库锁", "事务升级", "写锁竞争"],
  "summary": "For transactions that will read and then write, acquire write intent before reading and handle contention at transaction start with a bounded retry policy.",
  "when_to_use": "Independent native/plugin SQLite connections read successfully but an UPDATE fails immediately with SQLITE_BUSY or SQLITE_BUSY_SNAPSHOT even though a busy timeout is configured.",
  "status": "scoped"
}
---

# Avoid SQLite read-to-write upgrade failures despite a busy timeout

## When to use

Independent native/plugin SQLite connections read successfully but an UPDATE fails immediately with `SQLITE_BUSY` or `SQLITE_BUSY_SNAPSHOT` even though a busy timeout is configured.

## Recommended approach

For a transaction known to read and then write, start with `BEGIN IMMEDIATE` or the backend's equivalent. Acquire write intent before making decisions from a snapshot, and keep the transaction short. This changes where contention is handled; it does not create concurrent writers or guarantee success.

## Implementation steps

1. Confirm that the connections reach the same physical database. Record the actual SQLite library, journal mode, transaction mode and extended error code. A wrapper compiling for OH does not establish its runtime locking behavior.
2. Find read-modify-write operations that begin with the default deferred transaction. A SELECT can establish a read transaction that later needs promotion. SQLite may skip the busy handler to avoid a lock deadlock; in WAL mode, another committed writer can make that snapshot unsuitable for promotion.
3. Select immediate mode at the transaction API boundary before the first read. With a compatible `rusqlite` API, use `transaction_with_behavior(TransactionBehavior::Immediate)`. With another plugin, use its documented transaction option; do not issue a nested raw BEGIN inside an already-owned transaction.
4. Configure a bounded connection-local busy policy and handle failure to start the write transaction. Roll back and restart a stale read-modify-write transaction, rereading its inputs. Retrying only the UPDATE against the old snapshot does not refresh the decision.
5. Keep network calls, user interaction and long preparation outside the write transaction. Finalize statements and close cursors promptly, then commit or roll back on every path.
6. Treat commit failure according to the actual transaction state and error. A busy COMMIT can leave a transaction active; do not announce success or blindly repeat the business operation and its external side effects.

## Conditions and alternatives

Immediate mode is appropriate when the operation will write. Read-only work can keep its normal read transaction. WAL can improve reader/writer overlap but still permits only one writer and does not make an old read snapshot writable.

A single writer queue can reduce contention within an application, but it cannot serialize another process or independent native plugin by itself. Preserve the [backend, shared-store and migration contract](database-backend-and-data-contract.md) as well as the transaction policy.

## Common mistakes

Increasing `busy_timeout` alone does not fix every transaction-upgrade failure. Turning every read into an immediate transaction unnecessarily reserves the writer. Suppressing lock errors or retrying without releasing the stale snapshot can hide lost updates or prolonged contention.

## Check it works

Use two independent connections to the same test database. Reproduce the deferred read followed by another connection's write and verify the resulting error. With immediate mode, confirm the second writer contends before reading; after the first commits, restart and reread the new value. Cover rollback, exhausted retries, long readers and the journal modes actually used by the packaged OH backend.

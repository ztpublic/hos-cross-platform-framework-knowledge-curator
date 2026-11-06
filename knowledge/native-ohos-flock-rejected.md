---
{
  "schema_version": 1,
  "id": "native-ohos-flock-rejected",
  "title": "Replace flock with an in-process claim on OH when one process writes",
  "topic_key": "storage.native-ohos-flock-rejected",
  "area": "storage",
  "scope": "common",
  "frameworks": [],
  "summary": "If flock(2) is rejected, use an in-process write claim only when a single process really owns the data, and say that no inter-process exclusion exists.",
  "when_to_use": "A dependency that takes a file lock fails on OH, with an unsupported-platform error from its binding or a rejected flock call.",
  "applicability": "Single-process hosts on OH whose dependency uses flock only to exclude other writers. Rejection of flock is stated by the reviewed patch comment for one environment; verify on your device. Not valid when helpers or multiple abilities write the same store.",
  "keywords": [
    "flock",
    "file lock",
    "ERR_FLOCK_UNSUPPORTED_PLATFORM",
    "tryLock",
    "single process",
    "lock file",
    "EPERM"
  ],
  "related": [
    "native-hmdfs-no-hardlink-publish",
    "sqlite-read-write-transaction-contention",
    "qt-ohos-secondary-ability-process"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Replace flock with an in-process claim on OH when one process writes

## When to use

A dependency that takes a file lock fails on OH, with an unsupported-platform error from its binding or a rejected flock call.

## Recommended approach

If flock(2) is rejected, use an in-process write claim only when a single process really owns the data, and say that no inter-process exclusion exists.

## Implementation steps

1. Test flock on the actual storage path from a small native program rather than assuming the result.
2. List every process that can open the store: helpers, extensions, secondary abilities and any spawned tools.
3. If there is exactly one, keep an in-process claim and let the lock binding succeed on the OH branch, keeping real locking on other platforms.
4. If there are several, use a mechanism that works across them, such as a database with its own locking, an owner process with IPC, or O_EXCL lock files with stale handling.

## Conditions and alternatives

A stub only removes a failure: it gives no protection. Keep it visible in code with the reason. Re-check when a new process type is added.

## Common mistakes

Stubbing the lock in an app that later adds a helper process. Applying the stub on all platforms.

## Check it works

With two processes opening the store, confirm your design either prevents the second writer or reports it, and that single-process startup and crash recovery still work.

---
{
  "schema_version": 1,
  "id": "native-directory-polling-fallback",
  "title": "Use bounded directory snapshots when native watching is unavailable",
  "topic_key": "storage.native-directory-polling-fallback",
  "area": "storage",
  "scope": "common",
  "frameworks": [],
  "summary": "Use a cancellable polling worker that diffs directory snapshots, with explicit loss, error and shutdown semantics.",
  "when_to_use": "A native OH dependency needs file-change notifications but its selected backend cannot use its normal inotify implementation.",
  "applicability": "Native OH filesystem consumers with app-authorized readable directories and no usable event backend. PC project/cache trees can be large; this is a latency/CPU tradeoff rather than equivalent inotify behavior.",
  "keywords": [
    "FileSystemWatcher",
    "inotify",
    "poll",
    "lstat",
    "mtime",
    "cancel pipe",
    "directory watcher"
  ],
  "related": [
    "application-path-namespace",
    "foreground-recovery-and-save-lifetime"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Use bounded directory snapshots when native watching is unavailable

## When to use

A native OH dependency needs file-change notifications but its selected backend cannot use its normal inotify implementation.

## Recommended approach

Use a cancellable polling worker that diffs directory snapshots, with explicit loss, error and shutdown semantics.

## Implementation steps

1. Register only needed directories and take an initial snapshot of filename plus relevant metadata. Define whether watching is shallow or recursive.
2. On a worker, use poll on a cancellation pipe with a finite interval. On timeout scan and emit added, removed and changed records by comparing snapshots.
3. Treat scan failure as an error/unknown state, not an empty directory. Preserve the previous valid snapshot on EACCES or transient I/O failure to avoid false mass-deletion events.
4. Serialize directory registration, snapshot mutation and notification delivery, or use one owner thread with a command queue. Copying a directory list under a lock does not protect a separate snapshot map.
5. On stop, signal cancellation, join the worker and close pipe descriptors only after work is quiescent. Reinitialize/drain cancellation state before restart as required.

## Conditions and alternatives

The source tracks second-resolution mtime and size and scans one directory level. Same-size rewrites within the same timestamp interval and short-lived files can be missed. Renames may appear as remove/add; add higher-resolution metadata or stronger checks only where the feature requires them.

## Common mistakes

Do not assert that inotify is absent on every OH product, translate opendir failure into deletion, or perform large recursive scans on the UI thread.

## Check it works

Test add/remove/rename, same-size fast rewrites, permission loss/recovery, directory disappearance, concurrent registration and stop/restart. Measure latency and scan cost on a realistic PC workspace.

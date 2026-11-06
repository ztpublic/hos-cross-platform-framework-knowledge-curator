---
{
  "schema_version": 1,
  "id": "native-hmdfs-no-hardlink-publish",
  "title": "Publish files without hard links on OH hmdfs",
  "topic_key": "storage.native-hmdfs-no-hardlink-publish",
  "area": "storage",
  "scope": "common",
  "frameworks": [],
  "summary": "Treat a refused link as an unsupported operation: check the target is absent, then rename the staged file from the same directory, rather than failing atomic file creation.",
  "when_to_use": "Atomic create-if-absent writes, session logs or package stores fail with EPERM from link() in the app sandbox, while the same code works on desktop Linux.",
  "applicability": "Native or Node code on OH app storage where hmdfs provides no hard-link operation, according to the reviewed patch comment. Tools that hard-link caches, such as package managers, are affected in the same way.",
  "keywords": [
    "hard link",
    "link EPERM",
    "hmdfs",
    "EXDEV",
    "ENOTSUP",
    "writeFileAtomic",
    "atomic write",
    "rename",
    "publish"
  ],
  "related": [
    "sandbox-paths-and-app-identity",
    "foreground-recovery-and-save-lifetime",
    "electron-ohos-private-hnp"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Publish files without hard links on OH hmdfs

## When to use

Atomic create-if-absent writes, session logs or package stores fail with EPERM from link() in the app sandbox, while the same code works on desktop Linux.

## Recommended approach

Treat a refused link as an unsupported operation: check the target is absent, then rename the staged file from the same directory, rather than failing atomic file creation.

## Implementation steps

1. Write the staging file in the destination directory, so rename stays on one filesystem.
2. Try link() where it may work. Treat EACCES, EPERM, ENOSYS, EOPNOTSUPP, ENOTSUP and EXDEV as refusal, and let other errors fail normally.
3. After a refusal, lstat the target. Only ENOENT or ENOTDIR counts as absent, so that an unreadable target keeps the failure path.
4. Rename the staging file to the target, then remove any leftover temp on failure.
5. For exclusive creation across writers, use O_CREAT|O_EXCL on a lock or the target, as the check then rename leaves a window.

## Conditions and alternatives

rename overwrites silently, so this fallback is not as strict as link for concurrent writers: the source calls it no-overwrite, but that holds only when one process owns the directory. Do not rely on it across processes.

## Common mistakes

Using copy across directories, losing atomicity. Treating any error as refusal. Assuming a sandbox path shares the capabilities of a test system's ext4.

## Check it works

On device storage, run create-if-absent with an existing target, a missing target and concurrent writers, kill mid-write, and confirm no partial or lost files.

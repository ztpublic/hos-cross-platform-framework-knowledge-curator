---
{
  "schema_version": 1,
  "id": "qt-ohos-work-folder-grant",
  "title": "Give Qt file operations a persistent work folder",
  "topic_key": "storage.qt-ohos-work-folder-grant",
  "area": "storage",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Ask once for a folder grant, store its URI and real path, persist the permission policy, re-activate on restart, and verify access with the ArkTS filesystem before pushing the path to Qt.",
  "when_to_use": "A Qt app on OH needs repeated open/save operations, but each picker grant is temporary, dialogs reopen outside the granted tree after restart, or file writes fail with permission errors.",
  "keywords": [
    "DocumentViewPicker",
    "FOLDER",
    "persistPermission",
    "activatePermission",
    "work folder",
    "FileUri",
    "preferences",
    "file access",
    "restart",
    "Qt"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Qt apps on OH PC that need repeated file access inside a user-chosen folder. Persistent grants depend on the API level and the FILE_ACCESS_PERSIST permission.",
  "related": [
    "persistent-file-access-grants",
    "qt-ohos-embedded-file-dialogs",
    "qt-picker-initial-uri"
  ]
}
---

# Give Qt file operations a persistent work folder

## When to use

A Qt app on OH needs repeated open/save operations, but each picker grant is temporary, dialogs reopen outside the granted tree after restart, or file writes fail with permission errors.

## Recommended approach

Ask once for a folder grant, store both its URI and real path, persist the permission policy, re-activate it on restart, and verify access with the ArkTS filesystem before pushing the path to Qt. Clear a stale path so the next operation re-prompts.

## Implementation steps

1. Launch the document picker in folder mode; take the returned URI and derive the real path with `FileUri.path`.
2. Persist the grant with `fileShare.persistPermission` for read, write, create, delete and rename. Treat unsupported/not-permitted errors as session-only and continue.
3. Store the URI and real path in preferences. Push the path to Qt so its file dialogs start inside the granted tree.
4. On restart, read the stored URI, call `fileShare.checkPersistentPermission` and `fileShare.activatePermission` before using the path.
5. Verify real access with `fs.listFileSync` (or an equivalent direct file operation), not with `QDir`; a denied directory can look empty or readable to Qt.
6. If the path is inaccessible, clear the stored URI/path and re-prompt on the next file operation; do not push a stale path.
7. Keep the grant scoped to the work folder and re-check it before opening a dialog or writing a file; the user can revoke it in Settings.

## Conditions and alternatives

Persistent grants require the API level and permission that support `fileShare.persistPermission`. When persistence is unavailable, the grant is process-scoped and must be re-requested after restart. A per-file picker remains appropriate for one-off imports outside the work folder.

## Common mistakes

- Storing a real path without the URI, then having no way to re-activate the grant.
- Trusting `QDir` readability when the ArkTS grant was revoked.
- Pushing a stale path and opening an empty file dialog.
- Forgetting to re-activate a persisted grant before the first file operation.
- Copying every file into the sandbox instead of using the authorized folder.

## Check it works

Pick a folder, open and save several files without further prompts, then restart and repeat. Confirm the grant is re-activated or a clean re-prompt appears. Revoke the permission in Settings and verify the app detects it and re-prompts rather than opening an empty dialog.

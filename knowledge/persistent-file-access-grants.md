---
{
  "schema_version": 1,
  "id": "persistent-file-access-grants",
  "topic_key": "storage.persistent-file-access-grants",
  "scope": "common",
  "frameworks": [],
  "title": "Reopen a selected folder after restart or permission changes",
  "area": "storage",
  "summary": "Persist supported URI access grants and reactivate them before I/O. Track access loss separately from file deletion.",
  "when_to_use": "A folder selected earlier stops working after restart, a media scan loses access, or a native library cannot open a remembered external path.",
  "keywords": [
    "URI",
    "FILE_ACCESS_PERSIST",
    "授权",
    "persistent grants",
    "Rust",
    "media library",
    "revocation",
    "folder access after restart",
    "permission revoked",
    "目录权限失效",
    "扫描后文件消失"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": []
}
---

# Reopen a selected folder after restart or permission changes

## When to use

A folder selected earlier stops working after restart, a media scan loses access, or a native library cannot open a remembered external path.

## Recommended approach

Persist supported URI access grants and reactivate them before I/O. Track access loss separately from file deletion.

## Implementation steps

1. Query whether the device and file authority support the required directory selection and persistent authorization APIs.
2. Request the directory, acquire the appropriate policy, and persist the identifier needed to restore it together with the app's folder record.
3. Restore or activate the grant before startup scans, native-library initialization and playback. Pass an authorized usable path or handle into the native core.
4. Represent folder state explicitly: accessible, permission required, unavailable volume, or missing. Do not run destructive library cleanup for the first three states.
5. Offer reauthorization or importing selected files when ongoing external access cannot be provided.

## Conditions and alternatives

Persistent-grant support depends on the SDK, device and document authority. An imported copy is a different product contract: changes to the original will not automatically appear.

## Common mistakes

Storing only the path does not preserve authorization. Treating every failed directory listing as an empty folder can delete valid library metadata.

## Check it works

Restart, revoke access, disconnect/reconnect a volume and delete one real file. Verify native I/O succeeds after restoration and only confirmed deletion triggers cleanup.

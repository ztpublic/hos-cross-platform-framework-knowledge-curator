---
{
  "schema_version": 1,
  "id": "document-picker-uri-access",
  "title": "Keep document URI identity separate from path and access grants",
  "topic_key": "storage.document-picker-uri-access",
  "area": "storage",
  "scope": "common",
  "frameworks": [],
  "summary": "Preserve the original URI and its operation-specific grant; convert to a path only when the selected native backend requires and permits it.",
  "when_to_use": "A selected document cannot be read or written, or a remembered selection stops working after restart.",
  "keywords": [
    "DocumentViewPicker",
    "file picker",
    "URI",
    "permission",
    "文件选择",
    "授权"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Relevant to all framework adapters using HarmonyOS document selection, including PC external volumes. File and directory authority, API level and access lifetime can differ.",
  "related": [
    "persistent-file-access-grants",
    "sandbox-import-as-access-fallback"
  ]
}
---

# Keep document URI identity separate from path and access grants

## When to use

A selected document cannot be read or written, or a remembered selection stops working after restart.

## Recommended approach

Preserve the original URI and its operation-specific grant; convert to a path only when the selected native backend requires and permits it.

## Implementation steps

1. Return a typed selection containing URI, name and known access mode/lifetime. Distinguish cancellation before attempting I/O.
2. Use the target SDK fileuri/fileShare or equivalent supported APIs to resolve access. Converting URI text does not grant read, write or persistent access.
3. Pass an authorized handle or verified path to native code. Test read, create, overwrite, rename, delete and directory-child access independently.
4. Persist and reactivate grants only where supported; otherwise offer reauthorization or an explicit private import copy.

## Conditions and alternatives

Relevant to all framework adapters using HarmonyOS document selection, including PC external volumes. File and directory authority, API level and access lifetime can differ.

**Folder selection is not a write grant.** A write-up reports that a folder URI from DocumentViewPicker.select carries only temporary read permission, so writing a child file fails with `OpenFile ... err: 5`. It uses DocumentViewPicker.save with defaultFilePathUri set to that folder, letting the user confirm. This is a documentation report; check it on your API level and see the persistent-grant entry for long-term access.

## Common mistakes

Do not strip a URI prefix into a path, infer recursive access, or treat permission denial as file deletion.

## Check it works

Select a file and a directory; test cancellation, separate read/write access, restart, revocation and removal of the underlying volume.

---
{
  "schema_version": 1,
  "id": "sandbox-import-as-access-fallback",
  "topic_key": "storage.sandbox-import-as-access-fallback",
  "scope": "common",
  "frameworks": [],
  "title": "Give path-based native code access to user-selected content",
  "area": "storage",
  "summary": "Import the selected content into app-owned storage when direct authorized access cannot satisfy the native library. Make the copy semantics explicit.",
  "when_to_use": "A Rust/C/C++ library requires ordinary files, but a picker or incoming share provides a URI with limited or temporary access.",
  "keywords": [
    "URI copy",
    "cache",
    "sandbox",
    "导入",
    "filesDir",
    "file descriptor",
    "workspace",
    "URI to native path",
    "import folder",
    "Rust file access",
    "导入沙箱"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": []
}
---

# Give path-based native code access to user-selected content

## When to use

A Rust/C/C++ library requires ordinary files, but a picker or incoming share provides a URI with limited or temporary access.

## Recommended approach

Import the selected content into app-owned storage when direct authorized access cannot satisfy the native library. Make the copy semantics explicit.

## Implementation steps

1. Decide whether the content is temporary processing input or a durable user import; choose storage with the corresponding lifetime.
2. Resolve/open the selected URI while its access grant is valid. Copy with native descriptors or a stream rather than returning huge byte arrays through a method channel.
3. Write to a temporary destination, close handles, and rename only after a complete copy. Remove incomplete output on failure.
4. For folder imports, enforce containment, file/count/size limits and a symlink policy before exposing the imported tree to native code.
5. Return the owned destination only after success, and record how it relates to the original for later cleanup or reimport.

## Conditions and alternatives

Prefer retained access grants when editing the original is a requirement. Use a copy when offline availability or ordinary file access is more important than synchronization.

## Common mistakes

An imported workspace is not permission to edit the original. A cache copy can be evicted; durable imports consume duplicate storage and may be removed when the app is uninstalled.

## Check it works

Check byte equality, zero-byte and large files, interrupted copies, insufficient storage, malicious relative names, symlinks and cleanup after cancellation.

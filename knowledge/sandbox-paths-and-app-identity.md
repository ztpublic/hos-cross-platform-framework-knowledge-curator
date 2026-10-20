---
{
  "schema_version": 1,
  "id": "sandbox-paths-and-app-identity",
  "topic_key": "storage.sandbox-paths-and-app-identity",
  "scope": "common",
  "frameworks": [],
  "title": "Choose writable locations for shared and native application data",
  "area": "storage",
  "summary": "Resolve application storage through the platform context/path provider and pass it explicitly to native services. Treat identity changes as data-migration decisions.",
  "when_to_use": "Native code uses desktop HOME, a hard-coded bundle-derived location, or loses existing data after an application identity change.",
  "keywords": [
    "applicationSupportDirectory",
    "filesDir",
    "HOME",
    "bundleName",
    "沙箱路径",
    "app identity",
    "HOME directory",
    "native writable path",
    "data lost after update",
    "应用存储目录"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": []
}
---

# Choose writable locations for shared and native application data

## When to use

Native code uses desktop HOME, a hard-coded bundle-derived location, or loses existing data after an application identity change.

## Recommended approach

Resolve application storage through the platform context/path provider and pass it explicitly to native services. Treat identity changes as data-migration decisions.

## Implementation steps

1. Classify each file as temporary cache, durable private data, download staging or user export.
2. Resolve the corresponding app directory at runtime before initializing storage engines. Create required subdirectories and pass the resolved location to Rust/C/C++ rather than letting those layers guess HOME.
3. Keep identity-dependent strings in one application configuration source. When changing bundle identity or internal storage names, determine whether an upgrade can still reach the old data.
4. Implement a deliberate migration when access is available: verify the destination, copy or transform data, and retain recovery information until validation succeeds.
5. Keep any in-app directory browser contained within its advertised root.

## Conditions and alternatives

Private app storage is suitable for internal databases and caches. Use a document/export workflow for files the user must access outside the app.

## Common mistakes

Distinguish trusted platform storage prefixes from application-controlled descendants. A legitimate platform directory can have symlinked ancestors; rejecting every ancestor can block valid storage. Resolve or otherwise establish the trusted application root from the platform context, then enforce the containment and link policy on the selected root and its descendants. Lexical prefix comparison alone is insufficient, and a one-time path check does not eliminate filesystem races.

A successful write to a private directory does not mean a successful public export. Do not reuse another application's identity, assume directories survive uninstall, or silently select a new empty database.

Do not cache `Directory.current` as the durable app directory when the native directory request fails. Surface the failure or retry an authorized directory provider, so a transient channel error cannot permanently redirect later saves.

Check that the application facade actually consumes the runtime directory result. A platform-specific early return to a fixed sandbox path can bypass a correctly implemented `filesDir` handler. A fallback should preserve an explicit error or use another documented runtime directory provider.

## Check it works

Test first launch, restart, upgrades with existing data, storage errors and containment. Confirm the native engine uses the same resolved directory as the shared application layer.

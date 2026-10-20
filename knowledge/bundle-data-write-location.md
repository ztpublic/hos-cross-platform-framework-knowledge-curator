---
{
  "schema_version": 1,
  "id": "bundle-data-write-location",
  "title": "Copy mutable seed data out of installed resources",
  "topic_key": "storage.bundle-data-write-location",
  "area": "storage",
  "scope": "common",
  "frameworks": [],
  "summary": "Read immutable resources in place; initialize mutable copies in platform-provided application data storage before editing.",
  "when_to_use": "Updating a bundled configuration, database or model fails with read-only or permission errors.",
  "keywords": [
    "readonly",
    "bundle",
    "配置",
    "database",
    "sandbox"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to ordinary packaged apps on HarmonyOS PC. Application and shell namespaces may expose different path prefixes.",
  "related": [
    "sandbox-paths-and-app-identity"
  ]
}
---

# Copy mutable seed data out of installed resources

## When to use

Updating a bundled configuration, database or model fails with read-only or permission errors.

## Recommended approach

Read immutable resources in place; initialize mutable copies in platform-provided application data storage before editing.

## Implementation steps

1. Classify the artifact as resource, persistent data, cache or executable code.
2. Resolve the application data directory through its runtime context, create an owned destination, and copy seed data only when initialization or migration requires it.
3. Route subsequent writes to the mutable copy and preserve existing data during upgrades.
4. Keep initialization failure distinct from an empty default store and retain recoverable originals during migration.

## Conditions and alternatives

Applies to ordinary packaged apps on HarmonyOS PC. Application and shell namespaces may expose different path prefixes.

## Common mistakes

Do not chmod installed resources, write next to argv[0], or assume reinstall preserves private data.

## Check it works

Exercise first install, populated-data upgrade, failed copy, interrupted initialization and restart; confirm writes target the runtime-resolved directory.

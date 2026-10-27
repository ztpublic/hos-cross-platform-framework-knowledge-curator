---
{
  "schema_version": 1,
  "id": "tauri-ohos-web-storage-fallback",
  "title": "Handle unavailable Web Storage before Tauri frontend imports",
  "topic_key": "storage.tauri-ohos-web-storage-fallback",
  "area": "storage",
  "scope": "framework-specific",
  "frameworks": [
    "tauri"
  ],
  "summary": "Probe Web Storage before frontend modules execute, provide an explicitly temporary fallback when needed, and route durable preferences through a supported backend store.",
  "when_to_use": "A Tauri OH frontend crashes during module initialization because localStorage is null under its custom scheme, or preferences disappear after relaunch.",
  "applicability": "Experimental Tauri OH/ArkWeb custom-scheme generations exhibiting unavailable Web Storage. This is a backend/version condition, not a claim that all ArkWeb schemes lack persistence; PC validation remains unrun.",
  "keywords": [
    "localStorage",
    "null",
    "tauri://localhost",
    "ArkWeb",
    "module initialization",
    "memory fallback",
    "preferences"
  ],
  "related": [
    "tauri-ohos-ability-host",
    "file-secret-store-guarantees"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Handle unavailable Web Storage before Tauri frontend imports

## When to use

A Tauri OH frontend crashes during module initialization because localStorage is null under its custom scheme, or preferences disappear after relaunch.

## Recommended approach

Probe Web Storage before frontend modules execute, provide an explicitly temporary fallback when needed, and route durable preferences through a supported backend store.

## Implementation steps

1. Probe storage access and a small write/read/remove operation inside try/catch before the first module that reads preferences.
2. If startup requires a synchronous fallback, install it before ES module evaluation and implement the Storage methods your app uses with string key/value semantics.
3. Label in-memory storage as session-only. For durable data, use a registered OH-compatible store or app-owned sandbox file API and reconcile initialization before dependent features read it.
4. Test durable reload independently from initial rendering; keep original data intact during migration.

## Conditions and alternatives

The reviewed inline polyfill prevents null access and explicitly loses data on restart. It is not proof of a persistent store or a fully conforming Storage replacement.

## Common mistakes

Adding a fallback after imports is too late for top-level i18n/preference access. A rendered UI does not establish persistence.

## Check it works

Test null storage, throwing getters, module startup and a full application restart. Confirm temporary versus durable values follow the intended contract.

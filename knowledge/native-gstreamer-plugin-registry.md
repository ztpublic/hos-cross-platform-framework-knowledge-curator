---
{
  "schema_version": 1,
  "id": "native-gstreamer-plugin-registry",
  "title": "Locate GStreamer plugins and its writable registry in an OH HAP",
  "topic_key": "media.native-gstreamer-plugin-registry",
  "area": "media",
  "scope": "common",
  "frameworks": [],
  "summary": "Derive the packaged plugin directory from the loaded application library, set GStreamer search paths before initialization, and place its generated registry in writable sandbox storage.",
  "when_to_use": "A native OH media player cannot discover packaged GStreamer elements or tries to write its registry into the read-only bundle.",
  "applicability": "Native GStreamer builds embedded in OH HAPs, including PC. Plugin ABI/dependency closure and any external scanner execution must match the selected port.",
  "keywords": [
    "GStreamer",
    "GST_PLUGIN_PATH",
    "GST_PLUGIN_SYSTEM_PATH",
    "GST_REGISTRY",
    "dladdr",
    "gstreamer-1.0",
    "registry"
  ],
  "related": [
    "native-dependency-closure",
    "bundle-data-write-location"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Locate GStreamer plugins and its writable registry in an OH HAP

## When to use

A native OH media player cannot discover packaged GStreamer elements or tries to write its registry into the read-only bundle.

## Recommended approach

Derive the packaged plugin directory from the loaded application library, set GStreamer search paths before initialization, and place its generated registry in writable sandbox storage.

## Implementation steps

1. Use `dladdr` on an address owned by the loaded application library to locate its directory, then resolve the staged `gstreamer-1.0` subdirectory.
2. Before GStreamer initializes, set `GST_PLUGIN_PATH` and `GST_PLUGIN_SYSTEM_PATH` to that verified directory for the intended isolated plugin set.
3. Create a writable sandbox directory and set `GST_REGISTRY` to a file there. Keep optional GST_DEBUG_FILE output writable too.
4. Verify actual element discovery and recursive native dependencies; a plugin filename present in the HAP is only packaging evidence.

## Conditions and alternatives

If a plugin scanner is required, its ability to execute is a separate packaging/sandbox contract. Use a versioned registry location or invalidate stale plugin caches across upgrades.

## Common mistakes

Deriving paths from the host executable points at the Ability shell. Moving the registry does not make an incompatible plugin loadable.

## Check it works

Start with an empty registry, enumerate needed elements, restart offline and upgrade the plugin cohort. Confirm no search of developer-machine paths or writes into the bundle.

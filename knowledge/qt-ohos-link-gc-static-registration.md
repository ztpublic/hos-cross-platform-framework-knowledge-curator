---
{
  "schema_version": 1,
  "id": "qt-ohos-link-gc-static-registration",
  "title": "Keep static plugin registration through link-time GC",
  "topic_key": "build.qt-ohos-link-gc-static-registration",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Link-time garbage collection can discard self-registering static objects that have no external reference; disable section GC for plugin targets or give the registration object an explicit reference.",
  "when_to_use": "A plugin library loads successfully but its self-registering static objects never run, so formats or types are missing, while other plugins register correctly.",
  "keywords": [
    "gc-sections",
    "no-gc-sections",
    "static initialization",
    "init_array",
    "RegisteredClass",
    "dlopen",
    "plugin registration",
    "linker"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Native or Qt plugins that register types through static constructors and are linked with section garbage collection. The linker behavior must be verified for the selected toolchain.",
  "related": [
    "qt-ohos-plugin-loading",
    "native-registration-and-handler-coverage"
  ]
}
---

# Keep static plugin registration through link-time GC

## When to use

A plugin library loads successfully but its self-registering static objects never run, so formats or types are missing, while other plugins register correctly.

## Recommended approach

Link-time garbage collection can discard static registration objects that have no external reference. Disable section GC for plugin targets or give the registration object an explicit reference.

## Implementation steps

1. Reproduce with a plugin that logs its registry contribution at load time. Compare the registry size before and after `dlopen`.
2. Inspect the plugin link flags for `-Wl,--gc-sections`; check whether the registration objects appear in `.init_array` and whether their symbols survive.
3. Disable GC for that plugin target (`-Wl,--no-gc-sections`) or mark the registration object/section as retained.
4. As an alternative, compile the registration source into the main application library so registration and lookup share one registry and load order.
5. Rebuild and compare registry counts again; a load-only test would have passed before the fix and cannot detect this failure.

## Conditions and alternatives

The risk depends on the linker, optimization level and whether the registration object has any referenced symbol. Blanket `--whole-archive` or stripping can hide or worsen the problem. Fix the plugin target rather than weakening the whole build.

## Common mistakes

- Assuming a successful `dlopen` means the plugin registered.
- Adding `-Wl,--gc-sections` globally to reduce size without checking static constructors.
- Re-adding the object to the link without checking section retention.
- Testing only one plugin; the discarded object may affect only one format family.

## Check it works

Load the affected plugin and assert that its registry entry count increases and that opening a representative file succeeds. Confirm the linker flags for that target no longer discard the registration object.

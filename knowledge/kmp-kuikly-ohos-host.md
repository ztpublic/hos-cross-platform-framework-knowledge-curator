---
{
  "schema_version": 1,
  "id": "kmp-kuikly-ohos-host",
  "title": "Host a KuiklyUI KMP page in an OH app",
  "topic_key": "host.kmp-kuikly-ohos-host",
  "area": "host",
  "scope": "framework-specific",
  "frameworks": [
    "kotlin-multiplatform"
  ],
  "summary": "Combine the Kotlin/Native business .so, the Kuikly renderer HAR with its C++/NAPI library, and an ArkTS host that passes resource dirs/fonts and registers custom views and modules.",
  "when_to_use": "A KuiklyUI app needs to run on OH/PC, or the page renders but custom views/modules, fonts or resources are missing because the host was only partially wired.",
  "keywords": [
    "KuiklyUI",
    "Kotlin Multiplatform",
    "ohosArm64",
    "libshared.so",
    "libkuikly.so",
    "@kuikly-open/render",
    "NAPI",
    "KuiklyViewDelegate",
    "custom view",
    "resource manager"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "KuiklyUI KMP apps targeting OH. The renderer HAR, NAPI entry library and delegate APIs are version-specific.",
  "related": [
    "kmp-kuikly-ohos-resize-sync",
    "kmp-ohos-compose-host",
    "kmp-ohos-rawfile-resources"
  ]
}
---

# Host a KuiklyUI KMP page in an OH app

## When to use

A KuiklyUI app needs to run on OH/PC, or the page renders but custom views/modules, fonts or resources are missing because the host was only partially wired.

## Recommended approach

Combine the Kotlin/Native business `.so`, the Kuikly renderer HAR with its C++/NAPI library, and an ArkTS host that passes resource dirs/fonts and registers custom views and modules.

## Implementation steps

1. Cross-compile the Kotlin product for `ohosArm64` and copy two artifacts into the host: the shared library (for example `libshared.so`) into `entry/libs/arm64-v8a`, and its generated C header into the native third-party include directory.
2. Add the renderer HAR (`@kuikly-open/render`) and the NAPI entry library to the host module's `oh-package.json5`, then sync with ohpm.
3. In the ability, set the router adapter, load the page, and pass the resource manager, cache directory, resfile directory and any bundled font path to the native entry library before rendering.
4. Register custom render views and native-to-Kotlin modules through the view delegate's creator maps; unregistered names arrive as unknown views/modules.
5. Keep the host's native CMake target linked against the copied business library/header and the renderer library; a missing link surfaces as an unresolved symbol at load time.
6. Build the HAP, install it, and verify the first page, a custom view, a custom module call, a bundled font and a resource read.
7. Re-copy the `.so` and header whenever the Kotlin product changes; a stale business library can render an old page against a new renderer.

## Conditions and alternatives

The Kotlin/Native ABI must match the device (arm64). The renderer HAR and NAPI entry library versions must be compatible with the host and the Kotlin product. Passing resource paths is host-specific; a pure ArkTS app can skip the Kotlin library but not the renderer.

## Common mistakes

- Copying the `.so` but not the generated header, so the native host cannot compile.
- Forgetting to pass the resource manager/cache/resfile dirs, producing missing images or fonts.
- Registering a custom view in Kotlin but not in the ArkTS delegate (or vice versa).
- Linking the wrong ABI or leaving an old library in `entry/libs`.
- Assuming the page works after a successful build without exercising a custom module.

## Check it works

Open the first page and confirm it paints. Navigate to a page with a custom view, call a custom module, render a bundled font and load a resource from resfile/rawfile. Rebuild after changing the Kotlin product and confirm the new behavior appears.

---
{
  "schema_version": 1,
  "id": "kmp-ohos-compose-host",
  "title": "Mount a Kotlin/Compose shared library in an OH HAP",
  "topic_key": "native.kmp-ohos-compose-host",
  "area": "native",
  "scope": "framework-specific",
  "frameworks": [
    "kotlin-multiplatform"
  ],
  "summary": "Export a C entrypoint from Kotlin/Native, link it with the Compose bridge into a NAPI module, and mount it through the ArkTS Compose component with lifecycle forwarding.",
  "when_to_use": "The Kotlin/Native Compose library builds for OH, but the HAP cannot create the UI controller, the screen stays blank, or the native link fails on the main controller or shared-library symbols.",
  "keywords": [
    "libkn.so",
    "libkn_api.h",
    "MainArkUIViewController",
    "ComposeArkUIViewController",
    "NAPI",
    "skikobridge",
    "ArkUIViewController",
    "rendererBackend",
    "Compose Resources",
    "Kotlin/Native"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH 2in1/tablet HAPs using a Kotlin/Native Compose shared library and the adapter ArkTS component. The renderer backend, ABI and generated header must match the adapter.",
  "related": [
    "kmp-ohos-compose-target",
    "native-dependency-closure",
    "native-code-load-location"
  ]
}
---

# Mount a Kotlin/Compose shared library in an OH HAP

## When to use

The Kotlin/Native Compose library builds for OH, but the HAP cannot create the UI controller, the screen stays blank, or the native link fails on `MainArkUIViewController` or `libkn` symbols.

## Recommended approach

Export a C entrypoint from Kotlin/Native, link its shared library and the adapter's Compose bridge into the NAPI module, then create and mount the Compose controller from ArkTS with lifecycle forwarding.

## Implementation steps

1. Build the shared library and its generated C header from one Kotlin/Native link. Copy the header into the native include directory for the ABI and the `.so` into the matching `libs/<abi>` directory; a header from another build has mismatched symbols.
2. In CMake, add the header directory, link the shared library first and the adapter's Compose bridge second, and link the NAPI, hilog, deviceinfo, rawfile, EGL/GLES, libuv and C++ runtime libraries the module uses.
3. In Kotlin/Native, expose a `@CName("MainArkUIViewController")` function that initializes the main handler with `env` and returns a Compose ArkUI view controller wrapping the shared `App` composable.
4. In the NAPI module, include the generated C header, call the adapter's ArkUI initialization with `env` and `exports`, define the `MainArkUIViewController` property, and register the module name used by the ArkTS import.
5. In ArkTS, create the controller from the native module and mount the adapter's `Compose` component with the matching library name. Forward page show/hide and back events to the controller.
6. Copy Compose Resources into `rawfile/composeResources/<generated-package>`. The package directory is derived from the project and module names, lowercased; a mismatch renders missing strings and images.
7. Verify the module symbol table and the packaged library before installing.

## Conditions and alternatives

The renderer backend must match the libraries linked into the shared library; a fusion-renderer build needs its drawing, image, pixel-map, window and uv dependencies. The header, shared library and HAP must come from the same build. The adapter's HAR and native bridge are separate dependencies that must be available in the OH module.

## Common mistakes

- Mixing a header and shared library from different builds.
- Omitting the export artifact or the Compose bridge link, then chasing an unresolved controller symbol.
- Linking the bridge before the shared library or missing a renderer dependency, producing missing-symbol errors at load time.
- Registering the NAPI module under one name and importing another in ArkTS.
- Forgetting to sync Compose Resources, so the UI renders but strings and images are empty.

## Check it works

Install the HAP and confirm the Compose screen renders. Exercise back navigation, page hide/show, a string resource and an image resource. Inspect the packaged library's dynamic symbols and `DT_NEEDED` entries to confirm the header, library and bridge versions agree.

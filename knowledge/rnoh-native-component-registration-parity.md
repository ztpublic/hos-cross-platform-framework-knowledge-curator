---
{
  "schema_version": 1,
  "id": "rnoh-native-component-registration-parity",
  "title": "Register each RNOH native view across descriptor and binder maps",
  "topic_key": "integration.rnoh-native-component-registration-parity",
  "area": "integration",
  "scope": "framework-specific",
  "frameworks": [
    "react-native"
  ],
  "summary": "Register the supported component name in its descriptor provider and both native binder maps, then include that package and library in the actual host.",
  "when_to_use": "An RNOH package and TurboModule load, but a specific native canvas/view name is missing, blank or unbound.",
  "applicability": "RNOH C-API adapter exposing createComponentDescriptorProviders, createComponentJSIBinderByName and createComponentNapiBinderByName. PC canvas surface, mouse input and rendering require separate validation.",
  "keywords": [
    "ComponentDescriptorProvider",
    "ComponentJSIBinderByString",
    "ComponentNapiBinderByString",
    "SkiaDomView",
    "SkiaPictureView",
    "RNSkiaModule",
    "PackageProvider"
  ],
  "related": [
    "rnoh-native-host-and-codegen",
    "native-registration-and-handler-coverage"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Register each RNOH native view across descriptor and binder maps

## When to use

An RNOH package and TurboModule load, but a specific native canvas/view name is missing, blank or unbound.

## Recommended approach

Register the supported component name in its descriptor provider and both native binder maps, then include that package and library in the actual host.

## Implementation steps

1. Inventory the JS/native component names actually used by the app; module-level installation and component-level registration are separate checks.
2. For each supported view, provide its component descriptor and matching exact-name entries in the JSI binder and NAPI binder maps required by the selected RNOH version.
3. Return the native package from the host PackageProvider, compile/link its native library, and register the corresponding ArkTS/TurboModule package where that adapter requires it.
4. Check the adapter's supported subset. The reviewed source registers SkiaDomView while SkiaPictureView entries are commented out; do not infer support from upstream package exports.
5. When adding a missing view, implement its actual binder/surface contract and regenerate required descriptors before enabling map entries. A name-only placeholder is not a renderer.

## Conditions and alternatives

This entry covers registration parity, not a complete Skia OH port. Skia native dependencies, surface ownership and thread scheduling remain separate requirements. A TurboModule returning successfully does not prove a component renders.

## Common mistakes

Do not treat package registration as proof every upstream view exists, enable an unsupported descriptor without binders, or declare a blank canvas successful because the HAP starts.

## Check it works

Render each advertised native view in a minimal OH page, exercise property updates and unmount/remount, and confirm unsupported names fail explicitly. On PC, also verify resize, DPI and pointer input.

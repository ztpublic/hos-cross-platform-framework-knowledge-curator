---
{
  "schema_version": 1,
  "id": "rnoh-native-host-and-codegen",
  "title": "Wire an RNOH native host into the OH HAP",
  "topic_key": "integration.rnoh-native-host-and-codegen",
  "area": "integration",
  "scope": "framework-specific",
  "frameworks": [
    "react-native"
  ],
  "summary": "Add the RNOH HAR and generated package, register every OH package in both C++ and ArkTS, and mount RNApp behind an RNAbility subclass.",
  "when_to_use": "A React Native app has no HarmonyOS target, or the HAP builds but RNOH cannot find generated packages, a TurboModule or the registered root component.",
  "keywords": [
    "RNOH",
    "react-native-harmony",
    "codegen-harmony",
    "TurboModule",
    "RNAbility",
    "RNApp",
    "PackageProvider",
    "RNPackagesFactory",
    "HAR",
    "C-API"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH 2in1/tablet HAPs with React Native and RNOH versions that match the adapter. The C-API/New Architecture mode and manual package registration are required when autolink is unavailable.",
  "related": [
    "rnoh-js-bundle-platform",
    "rnoh-ohos-package-adaptation",
    "native-registration-and-handler-coverage"
  ]
}
---

# Wire an RNOH native host into the OH HAP

## When to use

A React Native app has no HarmonyOS target, or the HAP builds but RNOH cannot find generated packages, a TurboModule or the registered root component.

## Recommended approach

Add the RNOH HAR and its codegen output, register every OH package in both C++ and ArkTS, and mount `RNApp` behind an `RNAbility` subclass.

## Implementation steps

1. Add the RNOH runtime and CLI packages, create a native OH project, and list the RNOH HAR plus each OH module's HAR in the entry module's `oh-package.json5` using `file:` paths relative to the project.
2. Run the RNOH codegen command with the generated C++ output inside the native module and the RNOH module path inside the installed HAR. Include the generated package in the C++ package provider.
3. In CMake, add RNOH's native source directory and each module's native source directory, link the corresponding `rnoh_*` libraries, and build the host library with the RNOH NAPI bridge.
4. Keep the C++ package provider and the ArkTS package factory in sync. Every package constructed in one must be represented in the other, with the same module names.
5. Subclass `RNAbility`, return the page path that hosts the RN view, and mount `RNApp` with a JS bundle provider chain. The app key must equal the name passed to `AppRegistry.registerComponent`.
6. Declare the target device types and permissions in the entry module, then rebuild codegen and the native project after any dependency change.

## Conditions and alternatives

The React Native and RNOH versions must be compatible, and the C-API/New Architecture mode must match the adapter's expectations. Autolink may be unavailable for a package, in which case the HAR and native registration are manual. Source-linked HARs and bytecode HARs are not interchangeable.

## Common mistakes

- Building with a stale generated directory after adding a native module.
- Registering a package in C++ but not in ArkTS, or the reverse.
- Using an app key that differs from the `AppRegistry` name.
- Forgetting to install the HAR into `oh_modules` before configuring CMake.
- Assuming autolink will add a package that declares no OH autolink support.

## Check it works

Build and install the HAP. Confirm the root component renders, a generated TurboModule responds, and a package registered only in the ArkTS factory is rejected at startup rather than silently disappearing. Re-run codegen in a clean build to prove the generated package is reproducible.

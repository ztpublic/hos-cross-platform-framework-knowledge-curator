---
{
  "schema_version": 1,
  "id": "rnoh-ohos-package-adaptation",
  "title": "Replace native React Native dependencies with OH-compatible packages",
  "topic_key": "plugins.rnoh-ohos-package-adaptation",
  "area": "plugins",
  "scope": "framework-specific",
  "frameworks": [
    "react-native"
  ],
  "summary": "Map native dependencies to RNOH-compatible forks, install and register their HARs, and repair packages whose shipped HAR cannot resolve at runtime.",
  "when_to_use": "A third-party React Native library has no HarmonyOS implementation, its native module is missing at runtime, or its HAR fails with an unresolved /ts record.",
  "keywords": [
    "@react-native-ohos",
    "HAR",
    "oh-package.json5",
    "patch-package",
    "repack",
    "Autolink",
    "TurboModule",
    "React Native",
    "RNOH"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH 2in1/tablet React Native apps using RNOH with OH fork packages. Fork support, autolink behavior and HAR packaging differ per package.",
  "related": [
    "rnoh-native-host-and-codegen",
    "native-dependency-completeness"
  ]
}
---

# Replace native React Native dependencies with OH-compatible packages

## When to use

A third-party React Native library has no HarmonyOS implementation, its native module is missing at runtime, or its HAR fails with an unresolved `/ts` record.

## Recommended approach

Map each native dependency to an OH-compatible fork, install and register its HAR, and repair packages whose shipped HAR is source-only or otherwise unusable at runtime.

## Implementation steps

1. Inventory native dependencies and choose an OH fork that declares support for the project's React Native version. Alias JS-only packages when the fork keeps the original import name.
2. Add each fork's HAR path to the entry module's `oh-package.json5`, run the OH package install, and register the package's ArkTS class and native CMake target.
3. If a package ships a source HAR that references `/ts` at runtime and the loader cannot resolve it, repack the HAR with the patched ETS sources or use a bytecode HAR. Direct source links can crash at startup with a missing record.
4. Keep local patches version-pinned and apply them from a postinstall step; a patch written for one fork version will not silently apply to another.
5. Review the permissions and runtime behavior of each replacement. A package that compiles does not prove its OH implementation supports the same API surface.
6. Check for duplicate native classes and verify that the registration list matches the installed HARs.

## Conditions and alternatives

Fork support, autolink behavior and HAR packaging differ per package and RN version. A package with no OH fork should be replaced, stubbed behind a JS interface, or removed. Source and bytecode HARs are not interchangeable; neither is a generic C ABI a substitute for a TurboModule implementation.

## Common mistakes

- Importing the original package name while installing only the fork, or the reverse.
- Adding the HAR but omitting native registration in C++ and ArkTS.
- Mixing forks built for different RN versions.
- Shipping a source HAR whose `/ts` subpath the runtime cannot resolve.
- Assuming a successful compile proves the library's behavior on OH.

## Check it works

Open a screen that uses each replaced library and exercise one representative API. Confirm the app starts without a missing-record or duplicate-class error, the native package is registered, and permissions are requested by the target ability. Keep a list of features the fork does not support.

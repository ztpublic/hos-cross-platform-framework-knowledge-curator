---
{
  "schema_version": 1,
  "id": "rnoh-har-reactcommon-extraction",
  "title": "Restore missing ReactCommon from an RNOH HAR",
  "topic_key": "build.rnoh-har-reactcommon-extraction",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "react-native"
  ],
  "summary": "When ohpm installs the RNOH HAR but omits third-party/rn/ReactCommon, extract that tree from the local HAR into the installed native source directory and make the fix reproducible.",
  "when_to_use": "The native RNOH build fails with add_subdirectory errors for third-party/rn/ReactCommon/yoga, jsi, cxxreact or callinvoker after installing the RNOH HAR.",
  "keywords": [
    "RNOH",
    "HAR",
    "ohpm",
    "ReactCommon",
    "yoga",
    "jsi",
    "cxxreact",
    "CMake",
    "add_subdirectory",
    "native build"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH RNOH projects that vendor the RNOH HAR locally and build its C++ sources. The missing subtree and HAR layout depend on the RNOH/ohpm version.",
  "related": [
    "rnoh-native-host-and-codegen",
    "rnoh-offline-bundle-and-fonts",
    "native-dependency-completeness"
  ]
}
---

# Restore missing ReactCommon from an RNOH HAR

## When to use

The native RNOH build fails with `add_subdirectory` errors for `third-party/rn/ReactCommon/yoga`, `jsi`, `cxxreact` or `callinvoker` after installing the RNOH HAR.

## Recommended approach

When ohpm installs the RNOH HAR but omits `third-party/rn/ReactCommon`, extract that tree from the local HAR into the installed native source directory and make the fix reproducible.

## Implementation steps

1. Locate the installed RNOH native directory under `oh_modules/.ohpm/**/@rnoh/react-native-openharmony/src/main/cpp/third-party/rn` and confirm that `ReactCommon` is absent while its siblings (boost, folly, glog, hermes) exist.
2. Confirm the local HAR contains the source tree; a Git LFS pointer or a truncated archive will not work.
3. Extract only the missing subtree from the HAR into the installed directory, stripping the HAR's leading path components (for a `package/src/main/cpp/third-party/rn/` prefix, six components).
4. Verify `yoga`, `jsi`, `cxxreact`, `callinvoker` and `cmake-utils` are present before rebuilding.
5. Rebuild the native module and confirm the `add_subdirectory` errors are gone and the RNOH libraries link.
6. Make the repair reproducible: commit the restored source tree alongside a pinned HAR, or add a postinstall/extraction step. Editing only the generated `oh_modules` directory disappears on the next install.
7. Re-check the strip prefix and target path when the RNOH HAR layout changes.

## Conditions and alternatives

This addresses an incomplete HAR extraction, not a general CMake path error. A different packaging tool may extract correctly; verify the actual missing directory before patching. Committing a large `ReactCommon` tree increases repository size, so a scripted restore may be preferable.

## Common mistakes

- Re-running install and assuming the missing directory will appear.
- Editing only `oh_modules` without a reproducible postinstall or committed source.
- Using the wrong strip-components count and flattening the tree.
- Treating an LFS pointer as a real HAR.
- Rebuilding without checking that yoga/jsi/cxxreact are actually present.

## Check it works

From a clean install, apply the restore, run the native build and confirm no `add_subdirectory ... not an existing directory` errors remain. Check the packaged HAP for the RNOH native libraries. Remove `oh_modules`, reinstall, apply the reproducible fix, and confirm the same successful build.

---
{
  "schema_version": 1,
  "id": "electron-ohos-private-hnp",
  "title": "Scope a bundled native runtime with a private HNP",
  "topic_key": "build.electron-ohos-private-hnp",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Register the bundled .hnp as private so it is application-scoped instead of device-global, avoiding install conflicts with a sibling app that publishes the same package name.",
  "when_to_use": "Two OH apps bundle the same native package name, and installing the second fails because the public HNP is already registered on the device.",
  "keywords": [
    "HNP",
    "hnpPackages",
    "private",
    "native runtime",
    "install conflict",
    "9568407",
    "Python",
    "bundled runtime",
    "Electron"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH Electron apps that bundle a native runtime as an HNP and may coexist with another app shipping the same package name. Private HNP behavior depends on the API/product.",
  "related": [
    "electron-ohos-process-isolation",
    "electron-ohos-builder-packaging",
    "electron-ohos-local-static-server"
  ]
}
---

# Scope a bundled native runtime with a private HNP

## When to use

Two OH apps bundle the same native package name, and installing the second fails because the public HNP is already registered on the device.

## Recommended approach

Declare the bundled `.hnp` as `private` in the module's `hnpPackages`, so it is application-scoped instead of device-global, and rebuild the HAP so the metadata is baked in.

## Implementation steps

1. Package the native runtime as an `.hnp` under the module's `hnp/<abi>/` directory.
2. In the entry module manifest, add `hnpPackages` with the package name and `type: "private"`.
3. Avoid registering a device-global package name when a sibling app already publishes it publicly; private scope isolates the same name per application.
4. Rebuild the full HAP after changing the manifest; module metadata is baked into the package and an incremental build may not update it.
5. Install both apps in both orders and verify the second install does not fail with the public-HNP conflict code.
6. Document the runtime path, version and ABI; update the HNP when the interpreter or native ABI changes.

## Conditions and alternatives

Private HNP support depends on the API/product and packaging tool. If private scope is unavailable, use a distinct package name per app. A shared public runtime is appropriate only when a single owner and version policy exist.

## Common mistakes

- Leaving the package public and then debugging the install conflict as a signing problem.
- Editing `module.json5` without a full HAP rebuild.
- Assuming a private HNP is visible to other apps.
- Shipping the HNP without matching ABI/runtime files.
- Forgetting the HNP when the Python or native runtime version changes.

## Check it works

Install the sibling app first, then install this app; both installs succeed and neither overwrites the other. Launch both and confirm the bundled runtime works in each. Rebuild from a clean output directory and repeat the install-order test.

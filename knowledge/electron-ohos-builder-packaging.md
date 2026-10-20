---
{
  "schema_version": 1,
  "id": "electron-ohos-builder-packaging",
  "title": "Package an Electron app for OH with the OH builder adapter",
  "topic_key": "build.electron-ohos-builder-packaging",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Use an OH-aware electron-builder adapter with a supported prebuilt engine, explicit DevEco tool paths and tablet/2in1-only device classes; restore the engine and OH dependencies idempotently before packaging.",
  "when_to_use": "An Electron app builds for desktop but there is no HAP, or the OH package fails because the Electron engine, OH dependencies or DevEco tool paths are missing.",
  "keywords": [
    "Electron",
    "electron-builder",
    "OHOS",
    "HAP",
    "libelectron.so",
    "app.asar",
    "ohpm",
    "hvigor",
    "2in1",
    "tablet",
    "packaging",
    "prebuilt engine"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH 2in1/tablet HAPs built from an Electron project with a supported OH packaging adapter and engine. Phone targets are not supported by this engine distribution.",
  "related": [
    "electron-ohos-runtime-and-jit",
    "electron-ohos-native-addon-abi",
    "electron-ohos-platform-capability-gates"
  ]
}
---

# Package an Electron app for OH with the OH builder adapter

## When to use

An Electron app builds for desktop but there is no HAP, or the OH package fails because the Electron engine, OH dependencies or DevEco tool paths are missing.

## Recommended approach

Use an OH-aware electron-builder adapter with a supported prebuilt Electron engine, explicit DevEco tool paths, and device classes limited to tablet/2in1. Restore the engine and OH dependency tree idempotently before packaging.

## Implementation steps

1. Pin the Electron version and the adapter version together; the adapter and engine must agree on the Electron ABI and packaging layout.
2. Configure the OH target: `target: ["hap"]`, the OH project directory, app id/product name, asar inclusion, and the DevEco `hvigor`, `ohpm` and SDK paths.
3. Keep the large Electron engine as a compressed prebuilt artifact. In a prepare step, extract `libelectron.so` into the Electron module's `libs/arm64-v8a` and run `ohpm install` only when `oh_modules` is absent. Make both steps idempotent and fail loudly on missing inputs.
4. Declare only the OH permissions the app actually needs. The adapter should inject them into the generated module manifest; review the result instead of assuming.
5. Set the module device types to `tablet` and `2in1`. The ported engine does not support phone, and advertising it produces installs that cannot work.
6. Build the HAP with the adapter, then sign it in DevEco Studio with a profile valid for the target device. A debug signature is device/account-bound; a release package needs store distribution.
7. If the repository ships an OH project snapshot, sync the built `app.asar`, version metadata and icons back into that template so it opens and runs without a prior build.

## Conditions and alternatives

The packaging adapter, engine binary and Electron version are a matched set. The prebuilt engine may be large and license-bound; verify redistribution terms. DevEco paths are machine-specific. If the adapter is unavailable, the manual runtime-copy route is the alternative, but the engine/ABI/permission contract remains.

## Common mistakes

- Using stock `electron-builder` without an OH target.
- Omitting `libelectron.so` or skipping `ohpm install`, then debugging the resulting missing-library error as an app bug.
- Listing `phone` in device types because the UI happens to scale.
- Committing the uncompressed engine and hitting repository file-size limits instead of storing a compressed artifact.
- Building a new `app.asar` but shipping a stale OH template.

## Check it works

Build the HAP and inspect that it contains the engine library, `app.asar`, the declared permissions and tablet/2in1 device types. Install the signed package, confirm the app window opens and the packaged JS runs, and verify the template snapshot matches the built version.

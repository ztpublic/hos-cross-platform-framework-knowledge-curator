---
{
  "schema_version": 1,
  "id": "electron-ohos-platform-binding-loader-shim",
  "title": "Provide the missing OH platform package behind a Node native-binding loader",
  "topic_key": "native-addon.electron-ohos-platform-binding-loader-shim",
  "area": "native-addon",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "When a package selects its addon through an optional per-platform package that has no openharmony-arm64 release, materialize that package at build time and bypass any product check your self-built addon cannot pass.",
  "when_to_use": "The window stays about:blank and the log reports No usable native binding found for ...-openharmony-arm64, or native binding product mismatch: expected X, got openharmony-arm64, even though the .node file loads.",
  "applicability": "Electron OH apps depending on npm packages that resolve prebuilt bindings by platform/arch package name. Reviewed for one internal-API addon under Node 22.17 / Node-API 10 with a self-built napi-v9 binding. The shim bypasses a validation step, so it is only appropriate for an addon you build and verify yourself.",
  "keywords": [
    "No usable native binding found",
    "native binding product mismatch",
    "openharmony-arm64",
    "optional platform package",
    "about:blank",
    "pnpm deploy",
    "custom loader"
  ],
  "related": [
    "electron-ohos-native-addon-abi",
    "native-code-load-location",
    "electron-ohos-older-runtime-shims"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Provide the missing OH platform package behind a Node native-binding loader

## When to use

The window stays about:blank and the log reports No usable native binding found for ...-openharmony-arm64, or native binding product mismatch: expected X, got openharmony-arm64, even though the .node file loads.

## Recommended approach

When a package selects its addon through an optional per-platform package that has no openharmony-arm64 release, materialize that package at build time and bypass any product check your self-built addon cannot pass.

## Implementation steps

1. Read the loader to learn the package name it requires and what it validates. Unpublished platform packages are absent after pnpm deploy, so the failure appears only at runtime.
2. At collect time, create node_modules/<package>-openharmony-arm64 with package.json, lib/index.js and prebuilt/<addon>.node, using the addon built for the OH ABI. Fail the build if that binary is missing.
3. If the loader rejects your binding because of self-reported metadata, either make the addon report what the loader expects, or replace the wrapper package entry with a re-export of the platform package. Keep the same exported API.
4. Place the same .node where the OH linker namespace can reach it, such as the HAP native libs directory; a copy under the app files sandbox is not dlopen-able in the reviewed environment.
5. Pin the upstream package version and repeat the check when it changes, since each regeneration of node_modules removes the shim.

## Conditions and alternatives

Prefer an upstream openharmony package or a matching product field when you control the addon: the wrapper replacement also removes the upstream availability check. The addon's ABI still has to match the runtime; see the addon ABI entry.

## Common mistakes

Treating the about:blank window as a renderer problem instead of a boot failure in the main process. Shipping the shim without the libs copy, leaving the addon unloadable. Applying a re-export to a package whose exports differ per platform.

## Check it works

After a clean collect, confirm the platform package and the wrapper shim exist in the packaged tree, the loader resolves each exported function on the device, and the host boots with a visible window. Repeat after updating the upstream package.

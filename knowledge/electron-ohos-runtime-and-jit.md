---
{
  "schema_version": 1,
  "id": "electron-ohos-runtime-and-jit",
  "title": "Run Electron on OH with a ported runtime and authorized JIT",
  "topic_key": "host.electron-ohos-runtime-and-jit",
  "area": "host",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Ship an OH-built Chromium/Node/Electron runtime, declare only supported device classes, and verify writable-executable-memory authorization in both the manifest and the signing profile.",
  "when_to_use": "A desktop Electron application must run on OH PC/2in1, but its runtime does not load, Chromium starts without a window, or V8 aborts while initializing executable memory.",
  "keywords": [
    "Electron",
    "Electron on HarmonyOS",
    "libelectron.so",
    "Chromium",
    "Node.js",
    "JIT",
    "ALLOW_WRITABLE_CODE_MEMORY",
    "ACL",
    "2in1",
    "tablet",
    "HAP",
    "libc++_shared"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH 2in1/tablet devices with an Electron runtime actually ported to the platform and a JIT permission approved for the signing profile. Desktop Electron binaries and phone targets are out of scope.",
  "related": [
    "jit-executable-memory-authorization",
    "electron-ohos-native-addon-abi",
    "electron-ohos-non-pty-exec"
  ]
}
---

# Run Electron on OH with a ported runtime and authorized JIT

## When to use

A desktop Electron application must run on OH PC/2in1, but its runtime does not load, Chromium starts without a window, or V8 aborts while initializing executable memory.

## Recommended approach

Treat Electron as a platform port: ship a Chromium/Node/Electron runtime built for OH, declare only the supported device classes, and verify writable executable memory in both the manifest and the signing profile. If the JIT authorization is unavailable, switch to a non-JIT host instead of shipping a runtime that cannot initialize.

## Implementation steps

1. Inventory the runtime as native artifacts: the Electron/Chromium library, the host adapter, FFmpeg, and the C++ runtime. Confirm they are AArch64 OH ELF files; a desktop Electron bundle is not a substitute.
2. Package the runtime and its ArkTS host as modules of the HAP. Declare `deviceTypes` as the classes the port actually supports (`2in1`, possibly `tablet`); do not advertise `phone` unless the runtime supports it.
3. Declare `ohos.permission.kernel.ALLOW_WRITABLE_CODE_MEMORY` in the runtime module's `requestPermissions` (and `definePermissions` when the module defines it), then add the same permission to the signing profile's `allowed-acls`. A manifest entry alone does not grant JIT memory.
4. Build with a profile that contains the ACL, install on a 2in1 device, and verify the window, Node version/ABI, preload scripts, and JIT-dependent code paths.
5. If the ACL cannot be approved, use an ArkWeb plus Node host or another non-JIT renderer. Record the lost renderer/Node fidelity rather than enabling the permission indiscriminately.
6. Keep the Electron, Chromium, Node and native-addon versions pinned as one runtime set.

## Conditions and alternatives

The permission is restricted and product/certificate-dependent, and the device reach of a ported runtime is narrower than desktop Electron. Electron features that depend on Linux services (for example PTY) may remain unavailable even when Chromium and Node start.

## Common mistakes

- Renaming or repacking a desktop Electron binary and expecting it to load.
- Declaring the JIT permission only in `module.json5` while the signed profile lacks the ACL.
- Targeting phones with a runtime that only supports 2in1/tablet.
- Omitting the C++ runtime or FFmpeg, then treating the resulting loader error as a permission problem.

## Check it works

Install the signed HAP on a 2in1 device. Confirm the window opens, `process.versions` reports the ported Electron/Node versions, a JIT-dependent code path completes, and the installed profile lists the ACL. When the ACL is absent, verify that the documented non-JIT fallback starts without crashing.

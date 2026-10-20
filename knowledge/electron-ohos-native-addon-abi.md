---
{
  "schema_version": 1,
  "id": "electron-ohos-native-addon-abi",
  "title": "Match Electron's Node ABI for OH native addons",
  "topic_key": "native.electron-ohos-native-addon-abi",
  "area": "native",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Read the ABI and architecture from the target runtime, keep only matching AArch64 OH artifacts, and stub or disable addons whose native dependency cannot be built for the platform.",
  "when_to_use": "Electron starts on OH, but a native addon fails with a NODE_MODULE_VERSION mismatch, a missing .node file, an ELF loader error, or a module-registration failure.",
  "keywords": [
    "NODE_MODULE_VERSION",
    "ABI",
    "native addon",
    ".node",
    "prebuilds",
    "better-sqlite3",
    "sharp",
    "libvips",
    "node-pty",
    "AArch64",
    "Electron"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Electron runtimes on OH where native Node addons are packaged with the app. The ABI and loader path come from the runtime in use; the examples are one Electron generation, not platform constants.",
  "related": [
    "electron-ohos-runtime-and-jit",
    "native-dependency-closure",
    "ffi-library-and-symbol-availability"
  ]
}
---

# Match Electron's Node ABI for OH native addons

## When to use

Electron starts on OH, but a native addon fails with a `NODE_MODULE_VERSION` mismatch, a missing `.node` file, an ELF loader error, or a module-registration failure.

## Recommended approach

Read the ABI and architecture from the target runtime, keep only matching AArch64 OH artifacts, and stub or disable addons whose native dependency cannot be built for the platform.

## Implementation steps

1. On the target, record `process.versions.modules`, `process.arch` and `process.platform` (`openharmony` on OH). Derive the ABI from the runtime in use; do not copy an ABI number from another Electron release.
2. Enumerate every `.node` file and every `prebuilds/` directory in the packaged dependency tree. Remove directories for non-target platforms and architectures before packaging; packagers may reject foreign binaries, and keeping them inflates the package.
3. Supply each addon as an AArch64 OH artifact compiled against the runtime's Node ABI. Verify the file type and machine with the toolchain (`file`, `readelf -h`), not by its filename.
4. When an addon depends on a library unavailable on OH (for example libvips for image processing), replace its entry with a pure-JS stub or disable the feature. Make unsupported operations fail with an explicit error instead of pretending to succeed.
5. Place the `.node` files where the Node loader resolves them for the packaged app, and ensure the app sandbox can read and map them.
6. Rebuild and re-run the ABI inventory whenever Electron or Node changes.

## Conditions and alternatives

An ABI value such as 138 identifies one runtime generation; it is not a platform constant. Addons may also need source changes for OH-specific APIs. A stub is only acceptable when the affected feature can degrade honestly.

## Common mistakes

- Shipping a desktop build and diagnosing the resulting ELF error as a permission issue.
- Keeping every `prebuilds/` variant "just in case".
- Loading an addon built for a different Node ABI and interpreting the mismatch as a missing module.
- Letting a stubbed image pipeline report conversion success.

## Check it works

For each remaining addon, `require` it in the packaged app, call one representative operation, and compare `process.versions.modules` with the ABI recorded at build time. Confirm the packaged HAP contains only AArch64 `.node` files.

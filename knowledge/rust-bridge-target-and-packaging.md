---
{
  "schema_version": 1,
  "id": "rust-bridge-target-and-packaging",
  "topic_key": "native.rust-bridge-target-and-packaging",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "title": "Build and load a Rust library in a Flutter OH app",
  "area": "native",
  "summary": "Align the Rust target, OH native toolchain, bridge generator/runtime and application packaging as one integration.",
  "when_to_use": "Your Dart/Rust bridge works on another platform but OH cannot compile, package or initialize its native library.",
  "keywords": [
    "Rust",
    "flutter_rust_bridge",
    "Cargokit",
    "aarch64-unknown-linux-ohos",
    "FFI",
    "HAR",
    "ABI",
    "Rust OHOS build",
    "FRB initialization",
    "Cargokit target",
    "Rust库加载失败"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Flutter OH with the selected embedding/plugin versions; PC behavior requires the stated acceptance checks.",
  "related": [
    "native-ffi-shared-buffer-ownership",
    "rust-build-host-discovery"
  ]
}
---

# Build and load a Rust library in a Flutter OH app

## When to use

Your Dart/Rust bridge works on another platform but OH cannot compile, package or initialize its native library.

## Recommended approach

Align the Rust target, OH native toolchain, bridge generator/runtime and application packaging as one integration.

## Implementation steps

1. Select the intended OH CPU/target combination and an OH-compatible native toolchain. Confirm the bridge/build integration actually supports that target.
2. Align Dart and Rust bridge versions and regenerate both sides from the same interface definition.
3. Configure native compilation with the OH linker/sysroot and build the expected shared artifact plus required native dependencies.
4. If the build helper has no OH runner, add an explicit build-and-package step at the native build boundary rather than assuming the desktop step runs.
5. Ensure the resulting artifact is included in the app's native module and selected by its Dart loader.
6. Initialize the bridge before use and surface actionable load/initialization errors to the app.

## Conditions and alternatives

A target such as `aarch64-unknown-linux-ohos` applies to its intended architecture; it does not establish support for every device architecture. Generated bridge code and native binaries must agree.

If the bridge introduces a shared-buffer ABI, align its version and allocator/release function table as well as its serialized types. Native port delivery and asynchronous work need an explicit [shared-buffer ownership contract](native-ffi-shared-buffer-ownership.md); exporting a pointer does not make its allocation live for a later Dart consumer.

See [Preserve the real Cargo and rustup environment](rust-build-host-discovery.md) for this independent contract.

## Common mistakes

A local development dependency or cached binary can hide missing build inputs. Reusing Rust business logic does not remove target ABI, symbol or packaging work.

## Check it works

Build from reproducible inputs, inspect architecture/exported symbols, then initialize and call the bridge in a release app, including error and large-data paths.

Check toolchain discovery with both a rustup-managed install and a distribution-provided shim, including explicit home variables and a missing managed directory. Confirm which compiler the hook actually invokes before attributing a failure to the OH target.

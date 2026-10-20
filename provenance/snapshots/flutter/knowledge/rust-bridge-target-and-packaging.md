---
{
  "id": "rust-bridge-target-and-packaging",
  "title": "Build and load a Rust library in a Flutter OH app",
  "topic_key": "native.rust-bridge-target-and-packaging",
  "area": "native",
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
  "summary": "Align the Rust target, OH native toolchain, bridge generator/runtime and application packaging as one integration.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "Your Dart/Rust bridge works on another platform but OH cannot compile, package or initialize its native library."
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

### Preserve the real Cargo and rustup environment

Honor deliberately configured `CARGO_HOME`, `RUSTUP_HOME` and toolchain selection. If a build hook derives these values from the Cargo executable, require a verified rustup-managed directory layout before doing so. A system-distribution shim next to `rustup` can resolve to a system bin directory; deriving Cargo home from its parent can redirect toolchain discovery to an unrelated system prefix. Leave the inherited environment intact when the full managed layout is absent, and report the toolchain lookup failure rather than inventing a home directory. This is build-host discovery, separate from selecting the OH target linker/sysroot.

## Common mistakes

A local development dependency or cached binary can hide missing build inputs. Reusing Rust business logic does not remove target ABI, symbol or packaging work.

## Check it works

Build from reproducible inputs, inspect architecture/exported symbols, then initialize and call the bridge in a release app, including error and large-data paths.

Check toolchain discovery with both a rustup-managed install and a distribution-provided shim, including explicit home variables and a missing managed directory. Confirm which compiler the hook actually invokes before attributing a failure to the OH target.

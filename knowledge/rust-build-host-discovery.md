---
{
  "schema_version": 1,
  "id": "rust-build-host-discovery",
  "title": "Preserve the real Cargo and rustup environment",
  "topic_key": "native.rust-build-host-discovery",
  "area": "native",
  "scope": "common",
  "frameworks": [],
  "summary": "Honor explicit Cargo/rustup configuration and derive a home only from a verified rustup-managed layout.",
  "when_to_use": "A build hook derives Cargo home from a distribution shim and loses the intended toolchain.",
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
  "applicability": "Applies to build hosts invoking Cargo/rustup; independent of which UI framework consumes the OH target library.",
  "related": [
    "rust-bridge-target-and-packaging"
  ]
}
---

# Preserve the real Cargo and rustup environment

## When to use

A build hook derives Cargo home from a distribution shim and loses the intended toolchain.

## Recommended approach

Honor explicit Cargo/rustup configuration and derive a home only from a verified rustup-managed layout.

## Implementation steps

Honor deliberately configured `CARGO_HOME`, `RUSTUP_HOME` and toolchain selection. If a build hook derives these values from the Cargo executable, require a verified rustup-managed directory layout before doing so. A system-distribution shim next to `rustup` can resolve to a system bin directory; deriving Cargo home from its parent can redirect toolchain discovery to an unrelated system prefix. Leave the inherited environment intact when the full managed layout is absent, and report the toolchain lookup failure rather than inventing a home directory. This is build-host discovery, separate from selecting the OH target linker/sysroot.

## Conditions and alternatives

Applies to build hosts invoking Cargo/rustup; independent of which UI framework consumes the OH target library.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Exercise a rustup-managed install, distribution shim, explicit CARGO_HOME/RUSTUP_HOME and missing managed directories; verify the actual compiler and unchanged explicit environment.

---
{
  "schema_version": 1,
  "id": "ohos-native-toolchain-inputs",
  "title": "Keep native target inputs separate from host build tools",
  "topic_key": "build.ohos-native-toolchain-inputs",
  "area": "build",
  "scope": "common",
  "frameworks": [],
  "summary": "Use a coherent OH target compiler/sysroot/runtime and resolve host generators separately from target libraries.",
  "when_to_use": "Cross compilation finds host libraries or produces a binary for the wrong platform.",
  "keywords": [
    "NDK",
    "sysroot",
    "Clang",
    "cross compile",
    "交叉编译",
    "Cargo",
    "CC_aarch64_unknown_linux_ohos",
    "CARGO_TARGET_AARCH64_UNKNOWN_LINUX_OHOS_LINKER",
    "host build.rs",
    "CMAKE_TOOLCHAIN_FILE"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "The exact CMake/toolchain variable names belong to the selected SDK. Android or desktop Linux binaries are not interchangeable with OH artifacts.",
  "related": [
    "native-dependency-completeness"
  ]
}
---

# Keep native target inputs separate from host build tools

## When to use

Cross compilation finds host libraries or produces a binary for the wrong platform.

## Recommended approach

Use a coherent OH target compiler/sysroot/runtime and resolve host generators separately from target libraries.

## Implementation steps

1. Select the SDK-provided OH toolchain and target ABI/API intentionally.
2. Record the actual compiler, linker and sysroot used by configure probes and final compile/link commands.
3. Build code generators for the host; build shipped libraries for the target. Check CMake package discoveries for accidental host paths.
4. Inspect resulting ELF architecture, RPATH, exported ABI and dependencies before integrating it into the framework package.

## Conditions and alternatives

For Rust builds with native dependencies, scope `CC_<target_with_underscores>`, `CXX_<target_with_underscores>`, `AR_<target_with_underscores>` and `CARGO_TARGET_<UPPERCASE_TARGET_WITH_UNDERSCORES>_LINKER` to the OH target. Use an SDK Clang wrapper with the correct `--target` and `--sysroot`, and pass target-specific bindgen/CMake inputs where the consuming crate supports them. Avoid globally replacing `CC`/`CXX`, which can cross-compile host `build.rs` dependencies by accident. Keep the host frontend Node environment separate from any SDK Node required for Hvigor; switch only for the consuming build phase. These variable names follow the reviewed Cargo/native-crate workflow and must be checked against the actual dependency build scripts.

The exact CMake/toolchain variable names belong to the selected SDK. Android or desktop Linux binaries are not interchangeable with OH artifacts.

## Common mistakes

Arm64 architecture alone does not prove the OS ABI or runtime compatibility.

## Check it works

Rebuild with clean declared inputs, inspect tool commands and final ELF metadata, then initialize the library in the target app.

---
{
  "schema_version": 1,
  "id": "kmp-ohos-libcxx-shared-staging",
  "title": "Package the Kotlin/Native toolchain libc++_shared with an OH library",
  "topic_key": "packaging.kmp-ohos-libcxx-shared-staging",
  "area": "packaging",
  "scope": "framework-specific",
  "frameworks": [
    "kotlin-multiplatform"
  ],
  "summary": "Copy the Kotlin/Native .so together with its target libc++_shared.so from the konan dependencies into the HAP libs, and link the NAPI wrapper with an $ORIGIN rpath.",
  "when_to_use": "The NAPI wrapper cannot load the Kotlin/Native shared library on OH, or the build succeeds and the HAP fails at runtime with a missing C++ runtime.",
  "applicability": "Kotlin/Native OH targets producing a shared library wrapped by a CMake NAPI module, for ohosArm64 and ohosX64. The failure is inferred from the reviewed build script rather than observed in a log. The toolchain's copy can conflict with the SDK's, so use one consistently.",
  "keywords": [
    "libc++_shared.so",
    "konan",
    "Kotlin/Native",
    "IMPORTED_NO_SONAME",
    "$ORIGIN",
    "rpath",
    "linkDebugShared",
    "dlopen",
    "NAPI wrapper"
  ],
  "related": [
    "kmp-ohos-compose-host",
    "native-dependency-closure",
    "qt-ohos-system-lib-duplication"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Package the Kotlin/Native toolchain libc++_shared with an OH library

## When to use

The NAPI wrapper cannot load the Kotlin/Native shared library on OH, or the build succeeds and the HAP fails at runtime with a missing C++ runtime.

## Recommended approach

Copy the Kotlin/Native .so together with its target libc++_shared.so from the konan dependencies into the HAP libs, and link the NAPI wrapper with an $ORIGIN rpath.

## Implementation steps

1. In Gradle, find ~/.konan/dependencies/*/lib/<triple>/libc++_shared.so for aarch64-linux-ohos or x86_64-linux-ohos, and fail if nothing is found.
2. Register a copy task for each ABI that puts the Kotlin .so and libc++_shared.so in entry/libs/<abi>, and attach it with finalizedBy to the matching linkShared task.
3. In CMake, declare the Kotlin library as a SHARED IMPORTED target and set IMPORTED_NO_SONAME TRUE if it has no SONAME, or the wrapper records an absolute path.
4. Link with -Wl,--no-undefined and -Wl,-rpath,$ORIGIN, setting BUILD_RPATH and INSTALL_RPATH to $ORIGIN, and copy both libraries next to the wrapper post-build.
5. List the packaged HAP and check DT_NEEDED and RUNPATH on the wrapper.

## Conditions and alternatives

Check whether the SDK or another module already packages a libc++_shared and keep a single compatible copy. Selecting a different dependency directory when several konan versions are installed must match the compiler in use.

## Common mistakes

Copying from an arbitrary konan directory when multiple versions exist. Packaging only the Kotlin library. Hard-coded host paths in DT_NEEDED.

## Check it works

Clean build each ABI, unpack the HAP, inspect dynamic sections with llvm-readelf, and load the module on a device or matching-ABI emulator.

---
{
  "schema_version": 1,
  "id": "kmp-ohos-dependency-and-renderer",
  "title": "Choose OH-built KMP libraries and a Compose renderer",
  "topic_key": "build.kmp-ohos-dependency-and-renderer",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "kotlin-multiplatform"
  ],
  "summary": "Pin OH-compatible KMP artifacts, select a platform-specific Ktor engine in ohosArm64Main, and choose exactly one Compose renderer (Skia self-render or unified).",
  "when_to_use": "A shared Compose Multiplatform app builds for Android and iOS but OH fails to resolve the Ktor engine, Coil or navigation artifacts, or renders incorrectly because the Compose renderer mode is wrong.",
  "keywords": [
    "Kotlin Multiplatform",
    "Ktor",
    "curl-ohosarm64",
    "Coil",
    "navigation",
    "Compose renderer",
    "Skia",
    "ohrender",
    "ohosArm64",
    "dependency variants"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "KMP/Compose apps using OH-adapted artifacts from an additional Maven repository. Artifact versions and renderer modes are adapter-specific.",
  "related": [
    "kmp-ohos-compose-target",
    "kmp-ohos-rawfile-resources",
    "kmp-ohos-compose-host"
  ]
}
---

# Choose OH-built KMP libraries and a Compose renderer

## When to use

A shared Compose Multiplatform app builds for Android and iOS but OH fails to resolve the Ktor engine, Coil or navigation artifacts, or renders incorrectly because the Compose renderer mode is wrong.

## Recommended approach

Pin OH-compatible KMP artifacts, select a platform-specific Ktor engine in `ohosArm64Main`, and choose exactly one Compose renderer (Skia self-render or unified rendering).

## Implementation steps

1. Add the adapter's Maven repository to both `pluginManagement` and `dependencyResolutionManagement`, and pin Kotlin, Compose, coroutines, serialization, Coil and navigation to the same OH-compatible generation.
2. Keep shared HTTP/JSON/image/navigation dependencies in `commonMain`; put the Android engine in `androidMain`, the Apple engine in the iOS source set, and `ktor-client-curl-ohosarm64` in `ohosArm64Main`.
3. Do not mix upstream and OH-adapted versions of the same library family; version drift shows up as missing symbols or runtime crashes at the OH boundary.
4. Choose the Compose renderer explicitly: a Skia self-render version or the unified renderer version. Keep only one active; the two are not selected at runtime.
5. Configure the OH shared library with the expected base name, C API export and linker options; add cinterop for any raw resource APIs.
6. Publish the shared library/header and Compose resources into the OH app module as part of the build.
7. Verify networking, image loading and navigation on the OH target, and rebuild Android/iOS to confirm the platform-specific dependencies did not leak into shared code.

## Conditions and alternatives

The adapter artifacts and private repository must be reachable. Renderer selection depends on the Compose/OH adapter version. A library without an OH artifact needs a platform-specific alternative or a stub.

## Common mistakes

- Using the Android/iOS Ktor engine on OH.
- Mixing upstream Kotlin/Compose versions with OH-adapted artifacts.
- Enabling both renderer modes or leaving the choice implicit.
- Forgetting the C API export on the OH shared library.
- Testing only one target after changing common dependencies.

## Check it works

On OH, perform an HTTP request, load a network image and navigate between screens. Confirm the expected renderer is active from the build configuration and logs. Rebuild Android and iOS and confirm their engine selections and behavior are unchanged.

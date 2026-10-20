---
{
  "schema_version": 1,
  "id": "kmp-ohos-compose-target",
  "title": "Add an OHOS Compose target without breaking Android and desktop",
  "topic_key": "build.kmp-ohos-compose-target",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "kotlin-multiplatform"
  ],
  "summary": "Keep the Compose plugin on the OH-adapted version, add ohosArm64, and redirect only the OH-excluded target classpaths to the official artifacts.",
  "when_to_use": "A shared Compose Multiplatform project builds for Android and desktop, but adding a HarmonyOS target fails because ohosArm64 is unresolved, desktop artifacts disappear, or Android fails with duplicate androidx classes.",
  "keywords": [
    "Compose Multiplatform",
    "ohosArm64",
    "Kotlin Multiplatform",
    "dependency redirection",
    "resolutionStrategy",
    "libkn",
    "export",
    "androidx",
    "HarmonyOS",
    "compose resources"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "KMP projects using OH-adapted Compose Multiplatform artifacts from an additional Maven repository. The version suffix, export artifact and available variants are adapter-specific.",
  "related": [
    "kmp-ohos-compose-host",
    "native-dependency-completeness"
  ]
}
---

# Add an OHOS Compose target without breaking Android and desktop

## When to use

A Compose Multiplatform project builds for Android and desktop, but adding a HarmonyOS target fails because `ohosArm64()` is unresolved, desktop artifacts disappear, or Android fails with duplicate `androidx` classes.

## Recommended approach

Keep the Compose Gradle plugin on the OH-adapted version that provides the target and C API, add `ohosArm64()`, and redirect only the OH-excluded target classpaths to the official artifacts. Leave the OH and metadata classpaths on the adapted version.

## Implementation steps

1. Add the adapter's Maven repository to both `pluginManagement` and `dependencyResolutionManagement`. Pin the Compose plugin, Kotlin, coroutines and atomic-fu versions to the same adapter release.
2. Declare `ohosArm64()` and a shared library with a stable base name. Export the adapter's export artifact from the shared library so the build produces both the `.so` and the C header used by ArkTS. Add `-lz` and, for a fusion renderer, the renderer's native libraries.
3. Keep `compose.*` dependency accessors in `commonMain`. They also register Compose Resources generation; replacing them with string coordinates can leave `Res` unresolved.
4. Add a `configurations.configureEach` rule that rewrites only `*CompileClasspath` and `*RuntimeClasspath`, skipping any configuration containing `ohos` or `metadata`. Map `org.jetbrains.compose.*` to the official version, with a separate material3 line when its version differs.
5. Apply the same rule in a consumer application module. Gradle resolution strategy is module-local; a project dependency does not carry the publisher's redirect.
6. Create an `ohosMain` source set and make the OH target depend on it. Align the JVM target for Android and desktop with the Compose Desktop bytecode version.
7. Build the OH shared library, the desktop package and the Android app together. Check for duplicate classes before adding more shared code.

## Conditions and alternatives

The adapter publishes only the variants it supports, and its repository must be reachable at configuration time. The version suffix, export artifact and material3 line are adapter-specific. If a fork is unavailable, a separate OH UI module avoids mixing official and adapted artifacts in one module.

## Common mistakes

- Replacing `compose.*` accessors with hand-written coordinates and losing resource generation.
- Applying the redirect to `metadata*` or `ohos*` configurations, so the shared source set loses its OH implementation.
- Adding the rule only to the shared module and not to the Android application module.
- Mixing Kotlin, coroutines and Compose versions from different adapter releases.
- Forgetting the export artifact, then looking for a C header that was never generated.

## Check it works

Build the OH shared library and confirm both the `.so` and its generated header exist. Build and run the desktop target and assemble the Android target; neither should resolve the adapted artifacts. Search the dependency graph for duplicate `androidx` groups, and compile a `commonMain` resource reference to prove resource generation still works.

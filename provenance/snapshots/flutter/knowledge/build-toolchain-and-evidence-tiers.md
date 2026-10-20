---
{
  "id": "build-toolchain-and-evidence-tiers",
  "title": "Create a reproducible OH build and verify a usable application",
  "topic_key": "build.build-toolchain-and-evidence-tiers",
  "area": "build",
  "keywords": [
    "CI",
    "SDK",
    "HAP",
    "signing",
    "unsigned",
    "生成宿主",
    "构建",
    "release",
    "analyze passes HAP fails",
    "unsigned HAP",
    "CI reproducibility",
    "构建成功但不能安装"
  ],
  "summary": "Pin the complete OH build environment and verify each stage separately: analysis, native build, packaging, signing, installation and feature execution.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "Analysis passes but native packaging fails, CI publishes an unsigned artifact, or another machine cannot reproduce the build."
}
---

# Create a reproducible OH build and verify a usable application

## When to use

Analysis passes but native packaging fails, CI publishes an unsigned artifact, or another machine cannot reproduce the build.

## Recommended approach

Pin the complete OH build environment and verify each stage separately: analysis, native build, packaging, signing, installation and feature execution.

## Implementation steps

1. Record compatible Flutter OH, Dart, native SDK/API, CLI and dependency versions as build inputs.
2. Make host generation and dependency overrides explicit if CI prepares them dynamically.
3. Check real SDK metadata/tool availability before native compilation. Keep lightweight analysis setup separate from the native build job.
4. Treat native build exit status and packaging status explicitly; the existence of an artifact is not proof that the build completed successfully.
5. Identify whether the artifact is signed for the intended application/device and perform installation as a separate check.
6. Launch the real app and run acceptance checks for the native-backed features.

## Conditions and alternatives

A lower declared compatible API than the build SDK requires runtime checks on the supported device range. A workflow definition is a recipe, not a record of a successful run.

### Diagnose mixed compiler/engine caches before changing application semantics

A matching Flutter version string or cache stamp does not prove that OH Dart platform kernels, AOT compiler, engine and embedding HAR belong to the same pinned distribution. Compare the selected framework/engine/Dart/HAR revisions and trusted artifact hashes where available. Repair artifacts through the selected distribution's supported cache preparation, remove incompatible derived output, and rebuild before replacing native resolver behavior with a constant.

Pin a CI checkout to a verified revision rather than only a moving branch, and inspect the resulting SDK revision. Keep SDK-component API checks separate from a successful unsigned HAP build, signing and runtime acceptance. When a compatible SDK removes custom source/embedding overrides, remove their build hooks and parallel lock-generation paths together; retain one intentional dependency resolution path. Test clean input resolution and release startup independently.

## Common mistakes

Empty SDK markers cannot compile native code. A checksum does not make an unsigned artifact installable. Do not equate static analysis with device compatibility.

## Check it works

Reproduce from clean inputs; inspect application identity/signature and install output; verify startup and required feature flows on target hardware.

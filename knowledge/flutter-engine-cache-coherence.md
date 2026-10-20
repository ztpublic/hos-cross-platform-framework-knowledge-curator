---
{
  "schema_version": 1,
  "id": "flutter-engine-cache-coherence",
  "title": "Diagnose mixed compiler/engine caches before changing application semantics",
  "topic_key": "build.flutter-engine-cache-coherence",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "summary": "Align compiler, platform kernels, engine and embedding artifacts from the same pinned distribution.",
  "when_to_use": "Flutter startup fails despite matching version strings.",
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
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "A lower declared compatible API than the build SDK requires runtime checks on the supported device range. A workflow definition is a recipe, not a record of a successful run.",
  "related": [
    "build-toolchain-and-evidence-tiers"
  ]
}
---

# Diagnose mixed compiler/engine caches before changing application semantics

## When to use

Flutter startup fails despite matching version strings.

## Recommended approach

Align compiler, platform kernels, engine and embedding artifacts from the same pinned distribution.

## Implementation steps

A matching Flutter version string or cache stamp does not prove that OH Dart platform kernels, AOT compiler, engine and embedding HAR belong to the same pinned distribution. Compare the selected framework/engine/Dart/HAR revisions and trusted artifact hashes where available. Repair artifacts through the selected distribution's supported cache preparation, remove incompatible derived output, and rebuild before replacing native resolver behavior with a constant.

Pin a CI checkout to a verified revision rather than only a moving branch, and inspect the resulting SDK revision. Keep SDK-component API checks separate from a successful unsigned HAP build, signing and runtime acceptance. When a compatible SDK removes custom source/embedding overrides, remove their build hooks and parallel lock-generation paths together; retain one intentional dependency resolution path. Test clean input resolution and release startup independently.

## Conditions and alternatives

A lower declared compatible API than the build SDK requires runtime checks on the supported device range. A workflow definition is a recipe, not a record of a successful run.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Rebuild from a clean compatible SDK cache, verify component identities/hashes and test release startup. Confirm the original first failure is gone without a semantic workaround.

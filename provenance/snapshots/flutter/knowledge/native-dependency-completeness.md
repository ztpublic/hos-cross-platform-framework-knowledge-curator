---
{
  "id": "native-dependency-completeness",
  "title": "Make native dependencies reproducible outside a developer machine",
  "topic_key": "build.native-dependency-completeness",
  "area": "build",
  "keywords": [
    "HAR",
    "gitlink",
    "submodule",
    "local path",
    "reproducibility",
    "源码完整性",
    "license",
    "missing HAR",
    "local path dependency",
    "submodule missing",
    "干净环境构建失败"
  ],
  "summary": "Inventory every native build input and replace unexplained local dependencies with explicit, retrievable sources or documented artifact-generation steps.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "A Flutter OH app references a local HAR, missing submodule, ignored bridge package or cached native binary and fails on a clean checkout."
}
---

# Make native dependencies reproducible outside a developer machine

## When to use

A Flutter OH app references a local HAR, missing submodule, ignored bridge package or cached native binary and fails on a clean checkout.

## Recommended approach

Inventory every native build input and replace unexplained local dependencies with explicit, retrievable sources or documented artifact-generation steps.

## Implementation steps

1. Trace Dart path/Git dependencies, native modules, bridge generators, submodules and bundled libraries to their actual inputs.
2. Distinguish normal SDK-generated artifacts from project-owned dependencies that are missing.
3. Record compatible revisions, retrieval/build steps, target ABI and licensing for each required component.
4. Remove assumptions about another developer's directory layout. Pass environment/toolchain locations through deliberate configuration.
5. Keep generated bridge code reproducible with a pinned generator, whether it is checked in or generated during the build.
6. Verify that the final package includes all required native dependencies, not only the top-level library.

## Conditions and alternatives

An externally supplied binary can be a deliberate dependency, but its supported ABI/version and reproducible delivery still need to be defined.

When replacing a checked-in generator with a Git dependency, preserve its package subdirectory and resolved revision in the dependency lock, then regenerate all outputs from the retained contract. Verify that the fresh environment can retrieve the generator and that it still emits the OH language/backend you need before removing the local source. A committed generated file is useful for review but does not establish reproducible generation or a registered native implementation.

## Common mistakes

A Dart wrapper proves neither that the native implementation is present nor that its license permits reuse. Do not treat a missing generated SDK artifact as the same problem as an undocumented private library.

## Check it works

Build in an environment without developer-local caches/paths, inspect required artifacts and initialize each native dependency in the resulting app.

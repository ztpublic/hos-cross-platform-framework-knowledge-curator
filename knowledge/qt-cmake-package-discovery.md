---
{
  "schema_version": 1,
  "id": "qt-cmake-package-discovery",
  "title": "Find target Qt packages without importing host libraries",
  "topic_key": "build.qt-cmake-package-discovery",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Inspect root-path search policy and explicitly select the target package prefix before relaxing package lookup.",
  "when_to_use": "CMake cannot find Qt or a native dependency outside the OH toolchain root.",
  "keywords": [
    "find_package",
    "CMAKE_FIND_ROOT_PATH_MODE_PACKAGE",
    "CMAKE_PREFIX_PATH"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to Qt/CMake OH integrations with externally installed target packages; a blanket BOTH rule is unsafe without path verification.",
  "related": []
}
---

# Find target Qt packages without importing host libraries

## When to use

CMake cannot find Qt or a native dependency outside the OH toolchain root.

## Recommended approach

Inspect root-path search policy and explicitly select the target package prefix before relaxing package lookup.

## Implementation steps

1. Confirm the package config and library belong to the OH target SDK.
2. Inspect CMAKE_FIND_ROOT_PATH_MODE_PACKAGE and the actual search trace.
3. Where the SDK layout requires it, use a scoped BOTH policy and explicit target prefixes before find_package.
4. Inspect every imported target path to ensure the relaxed search did not select host libraries.

## Conditions and alternatives

Applies to Qt/CMake OH integrations with externally installed target packages; a blanket BOTH rule is unsafe without path verification.

## Common mistakes

Finding a package is not proof it is built for OH.

## Check it works

Configure from clean inputs and inspect imported targets plus final ELF architecture.

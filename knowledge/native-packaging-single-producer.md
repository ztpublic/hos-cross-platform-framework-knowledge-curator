---
{
  "schema_version": 1,
  "id": "native-packaging-single-producer",
  "title": "Give each packaged native library one build producer",
  "topic_key": "build.native-packaging-single-producer",
  "area": "build",
  "scope": "common",
  "frameworks": [],
  "summary": "Choose either integrated native build output or an explicit prebuilt input for each logical library.",
  "when_to_use": "Hvigor ProcessLibs reports duplicate .so files from native intermediates and the manual libs input.",
  "keywords": [
    "00306049",
    "Duplicated files",
    "ProcessLibs",
    "abiFilters"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to projects using Hvigor native integration; exact packaging paths and dependency deployment are framework-specific.",
  "related": []
}
---

# Give each packaged native library one build producer

## When to use

Hvigor ProcessLibs reports duplicate .so files from native intermediates and the manual libs input.

## Recommended approach

Choose either integrated native build output or an explicit prebuilt input for each logical library.

## Implementation steps

1. Trace both conflicting files to their producers and compare ABI/version/hash.
2. Keep the intended producer and remove only the redundant generated/copy rule, preserving separately supplied runtime dependencies.
3. Check ABI filters against every shipped dependency rather than enabling unsupported architectures from a template.
4. Rebuild the package and inspect that each intended library appears once.

## Conditions and alternatives

Applies to projects using Hvigor native integration; exact packaging paths and dependency deployment are framework-specific.

## Common mistakes

Do not delete the only valid prebuilt library or resolve a duplicate by packaging two different versions under arbitrary names.

## Check it works

Reproduce the conflict, regenerate from clean derived output and verify one consistent library per ABI.

---
{
  "schema_version": 1,
  "id": "native-code-load-location",
  "title": "Load native libraries from supported code locations",
  "topic_key": "native.native-code-load-location",
  "area": "native",
  "scope": "common",
  "frameworks": [],
  "summary": "Package fixed native code through the supported HAP distribution path and diagnose loader policy separately from ABI and dependency failures.",
  "when_to_use": "dlopen rejects a library copied into cache or another writable data directory.",
  "keywords": [
    "dlopen",
    "可写路径",
    "loader",
    "QLibrary",
    "QPluginLoader"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies when the target product enforces code-location/integrity policy. Ordinary writable storage is not automatically an executable-code location.",
  "related": [
    "ffi-library-and-symbol-availability"
  ]
}
---

# Load native libraries from supported code locations

## When to use

dlopen rejects a library copied into cache or another writable data directory.

## Recommended approach

Package fixed native code through the supported HAP distribution path and diagnose loader policy separately from ABI and dependency failures.

## Implementation steps

1. Capture the resolved path and native loader error inside the application process.
2. Inspect ELF architecture, required symbols and NEEDED dependencies before attributing the error to permission policy.
3. Place fixed libraries in the target ABI packaging inputs and rebuild/sign through the application distribution workflow.
4. If dynamic code delivery is a real requirement, use a supported platform delivery mechanism and validate its signing and lifecycle contract.

## Conditions and alternatives

Applies when the target product enforces code-location/integrity policy. Ordinary writable storage is not automatically an executable-code location.

## Common mistakes

Changing file mode or moving a library into private cache does not establish loader permission. Do not replace missing dependencies with empty ABI stubs.

## Check it works

Confirm the library loads from the installed package in a clean app and that each required symbol/dependency resolves; separately observe the rejected-path case.

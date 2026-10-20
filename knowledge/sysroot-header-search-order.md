---
{
  "schema_version": 1,
  "id": "sysroot-header-search-order",
  "title": "Diagnose C++ header failures from injected sysroot include order",
  "topic_key": "build.sysroot-header-search-order",
  "area": "build",
  "scope": "common",
  "frameworks": [],
  "summary": "Compare the actual failing include search order with a minimal toolchain build before adding type or header shims.",
  "when_to_use": "C++ standard headers fail with nullptr_t or math overload diagnostics in one OH build configuration.",
  "keywords": [
    "nullptr_t",
    "__promote",
    "isystem",
    "sysroot",
    "libc++"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "This is a diagnosis for a matching include-order failure, not a universal OH compiler flag recommendation. The original evidence included an isolated probe and a multi-variable workaround.",
  "related": []
}
---

# Diagnose C++ header failures from injected sysroot include order

## When to use

C++ standard headers fail with nullptr_t or math overload diagnostics in one OH build configuration.

## Recommended approach

Compare the actual failing include search order with a minimal toolchain build before adding type or header shims.

## Implementation steps

1. Capture the full compile command and include trace for a minimal standard-library source.
2. Compare explicit -isystem sysroot/usr/include injection with the toolchain's normal --sysroot ordering.
3. If the injected path reproduces shadowing, correct the build rule that adds it and regenerate build files.
4. Retest full targets; if host compiler, SDK and build route changed together, record a working combination without claiming which one caused the fix.

## Conditions and alternatives

This is a diagnosis for a matching include-order failure, not a universal OH compiler flag recommendation. The original evidence included an isolated probe and a multi-variable workaround.

## Common mistakes

Hand-editing generated Ninja files is not a durable fix. Removing unrelated include paths or changing several toolchains at once does not isolate a cause.

## Check it works

Compare minimal probe and full build commands after regeneration; verify no undesired include injection returns.

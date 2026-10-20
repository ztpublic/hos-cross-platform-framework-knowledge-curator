---
{
  "schema_version": 1,
  "id": "native-dependency-closure",
  "title": "Package the complete native dependency closure",
  "topic_key": "native.native-dependency-closure",
  "area": "native",
  "scope": "common",
  "frameworks": [],
  "summary": "Trace NEEDED entries and required exports recursively, then package supported ABI-compatible dependencies using the names the loader requests.",
  "when_to_use": "A packaged native library links successfully but fails to load because a versioned SONAME or indirect dependency is absent.",
  "keywords": [
    "SONAME",
    "NEEDED",
    "FFmpeg",
    "symbol not found",
    "传递依赖"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to native libraries consumed by any framework; module directory conventions and library versions remain backend-specific.",
  "related": [
    "ffi-library-and-symbol-availability",
    "native-code-load-location"
  ]
}
---

# Package the complete native dependency closure

## When to use

A packaged native library links successfully but fails to load because a versioned SONAME or indirect dependency is absent.

## Recommended approach

Trace NEEDED entries and required exports recursively, then package supported ABI-compatible dependencies using the names the loader requests.

## Implementation steps

1. Inspect ELF machine, SONAME, NEEDED and unresolved symbols with the selected toolchain utilities.
2. Resolve every dependency to an allowed system API or a redistributable matching native artifact. System-wide existence does not establish app-namespace visibility.
3. Preserve required versioned names through packaging; verify actual HAP contents rather than assuming symlinks or copy globs survive.
4. Rebuild unavailable dependencies or disable the feature requiring them. For static linkage into shared libraries, verify PIC and licensing.
5. Test optional codecs and late-used symbols as well as startup.

## Conditions and alternatives

Applies to native libraries consumed by any framework; module directory conventions and library versions remain backend-specific.

## Common mistakes

Empty stubs can have incompatible signatures and silently remove functionality. Pulling a device system library is not a portable general packaging solution.

## Check it works

Load from a clean package and exercise each enabled feature; inspect the installed dependency chain and diagnostics.

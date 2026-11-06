---
{
  "schema_version": 1,
  "id": "native-bridge-weak-optional-runtime-symbol",
  "title": "Bind a newer runtime symbol weakly in an OH native bridge",
  "topic_key": "native-bridge.native-bridge-weak-optional-runtime-symbol",
  "area": "native-bridge",
  "scope": "common",
  "frameworks": [],
  "summary": "Declare an optional API added to a separately shipped native runtime as a weak symbol and test it for null before use, so the bridge still loads against older runtimes.",
  "when_to_use": "A bridge .so built against a newer header fails to load with an undefined symbol on older runtime libraries, disabling every feature for the sake of one optional call.",
  "applicability": "C or C++ bridges on OH linked against a native library versioned or delivered separately from the bridge. Applies to direct dynamic linking; dlopen/dlsym consumers already have their own lookup. Linker settings that reject undefined symbols must be checked.",
  "keywords": [
    "#pragma weak",
    "weak symbol",
    "undefined symbol",
    "version skew",
    "optional API",
    "native runtime compatibility",
    "dlopen"
  ],
  "related": [
    "ffi-library-and-symbol-availability",
    "native-packaging-single-producer",
    "native-dependency-closure"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Bind a newer runtime symbol weakly in an OH native bridge

## When to use

A bridge .so built against a newer header fails to load with an undefined symbol on older runtime libraries, disabling every feature for the sake of one optional call.

## Recommended approach

Declare an optional API added to a separately shipped native runtime as a weak symbol and test it for null before use, so the bridge still loads against older runtimes.

## Implementation steps

1. Limit this to additive, optional functions. Compatibility-breaking ABI changes need a version check instead.
2. Add #pragma weak symbol_name after the declaration, or an equivalent attribute, in the bridge translation unit.
3. Before each call test the symbol address, and return a typed unsupported error naming the required update.
4. Add a test that links the bridge against a stub runtime lacking the symbol, to catch accidental hard references, including from any other translation unit.
5. Expose capability to the caller so the UI can hide the feature rather than failing when it is used.

## Conditions and alternatives

Weak references only help if the final link tolerates them and the dynamic loader resolves them to null; verify on the target toolchain and device. Prefer a single versioned package when you control both libraries.

## Common mistakes

Marking the symbol weak in one translation unit and calling it unguarded elsewhere. Using this to hide mismatched struct layouts between versions.

## Check it works

Load the bridge with old and new runtime libraries on a device. Core playback or other base features must work with the old one, and the optional call must return the unsupported error instead of crashing.

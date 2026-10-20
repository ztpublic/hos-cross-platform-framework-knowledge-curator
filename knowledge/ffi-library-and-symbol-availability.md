---
{
  "schema_version": 1,
  "id": "ffi-library-and-symbol-availability",
  "topic_key": "native.ffi-library-and-symbol-availability",
  "scope": "common",
  "frameworks": [],
  "title": "Diagnose FFI library and missing-symbol failures on OH",
  "area": "native",
  "summary": "Validate the actual packaged library and every required symbol, including symbols resolved only when an optional feature is used.",
  "when_to_use": "An FFI call fails to load a library, or an optional operation crashes even though native initialization appeared successful.",
  "keywords": [
    "DynamicLibrary",
    "QuickJS",
    "jsSetMemoryLimit",
    "symbol",
    "动态库",
    "FFI",
    "undefined symbol",
    "DynamicLibrary open failed",
    "找不到动态库符号"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": []
}
---

# Diagnose FFI library and missing-symbol failures on OH

## When to use

An FFI call fails to load a library, or an optional operation crashes even though native initialization appeared successful.

## Recommended approach

Validate the actual packaged library and every required symbol, including symbols resolved only when an optional feature is used.

## Implementation steps

1. Capture the loader error and identify the artifact selected on OH rather than assuming the desktop/Android branch is appropriate.
2. Inspect the artifact's architecture, exported symbols and native dependencies. Confirm it is included where the app loader expects it.
3. Compare wrapper signatures and versions with the library. Exercise optional settings that can trigger lazy symbol lookup.
4. If a missing optional symbol has an acceptable fallback, disable that operation explicitly and document the lost guarantee. Otherwise rebuild/update the native library.
5. Use a process-symbol lookup fallback only when the function is genuinely exported by the process; keep diagnostics for each attempted load.
6. For a wrapper over another native core, package both artifacts for the same target ABI. Align the OH native build output with the module's native-library packaging input and verify the installed dependency chain; a wrapper that compiles can still fail when its core is absent.

## Conditions and alternatives

If an exported start function spawns work and returns immediately, retain the library handle and ownership state until that work has actually stopped. The start call returning is not a safe point for `dlclose` or for clearing a running-session marker. Correlate stop/completion with the active launch generation so an old completion cannot release a newer session.

For example, omitting an unsupported memory-limit setter may avoid lazy symbol lookup, but it also removes that particular resource bound. It is not an equivalent implementation of the limit.

## Common mistakes

Do not infer complete compatibility from one successful initialization call or from an arm64 binary existing in the checkout. Avoid hiding every load failure behind an empty catch.

## Check it works

Test clean installation, release startup, optional native calls, invalid inputs and missing-library cases. Verify any safety/resource limit lost by a fallback.

---
{
  "schema_version": 1,
  "id": "ffi-c-char-target-abi",
  "title": "Use the target c_char type in native bindings",
  "topic_key": "native.ffi-c-char-target-abi",
  "area": "native",
  "scope": "common",
  "frameworks": [],
  "summary": "Model C ABI types with the toolchain's c_char definitions and regenerate or narrowly repair mismatched bindings.",
  "when_to_use": "A native binding fails on OH because it hard-codes signed character pointers.",
  "keywords": [
    "c_char",
    "u8",
    "i8",
    "E0308",
    "ABI"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to native Rust/C interop where target signedness differs; c_char is not guaranteed signed or unsigned across all targets.",
  "related": []
}
---

# Use the target c_char type in native bindings

## When to use

A native binding fails on OH because it hard-codes signed character pointers.

## Recommended approach

Model C ABI types with the toolchain's c_char definitions and regenerate or narrowly repair mismatched bindings.

## Implementation steps

1. Confirm the selected target's c_char signedness and pointer/function ABI.
2. Use std::os::raw::c_char or the matching libc/generated C type at the boundary.
3. Keep encoding and byte interpretation explicit when crossing to strings or arbitrary binary buffers.
4. Patch only the affected dependency boundary or disable an unused dependent feature, then test original and OH targets.

## Conditions and alternatives

Applies to native Rust/C interop where target signedness differs; c_char is not guaranteed signed or unsigned across all targets.

## Common mistakes

A pointer cast can hide a type error without proving the called ABI or string encoding is correct.

## Check it works

Cross-compile bindings and round-trip ASCII, non-ASCII and bytes above 127 with correct ownership.

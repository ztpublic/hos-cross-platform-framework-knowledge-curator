---
{
  "schema_version": 1,
  "id": "native-c-wrapper-optional-string-lifetime",
  "title": "Audit optional string lifetimes before native storage calls",
  "topic_key": "native.native-c-wrapper-optional-string-lifetime",
  "area": "native",
  "scope": "common",
  "frameworks": [],
  "summary": "Keep each optional std::string owner alive through the native call; treat the reviewed dangling-pointer path as unresolved until corrected and exercised.",
  "when_to_use": "An OH C wrapper passes optional encryption-key or root-directory pointers into a native storage API after the temporary std::string objects have left scope.",
  "applicability": "Native C++ storage wrappers called from KMP or another OH runtime. Relevant to PC native storage arguments; successful compilation does not validate optional-key/root-path behavior.",
  "keywords": [
    "MMKV",
    "mmkv_c_default_mmkv",
    "mmkv_c_mmkv_with_id",
    "std::string",
    "crypt_key",
    "root_dir",
    "dangling pointer",
    "use after scope"
  ],
  "related": [
    "kmp-napi-string-allocator-ownership",
    "native-bridge-stale-handle-identity"
  ],
  "status": "unresolved",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Audit optional string lifetimes before native storage calls

## When to use

An OH C wrapper passes optional encryption-key or root-directory pointers into a native storage API after the temporary std::string objects have left scope.

## Recommended approach

Keep each optional std::string owner alive through the native call; treat the reviewed dangling-pointer path as unresolved until corrected and exercised.

## Implementation steps

1. Trace pointer construction to the actual native call, including optional branches. In the reviewed pattern key_ptr/root_dir_ptr outlive the inner block that declares their std::string owners.
2. As a candidate correction, declare the string owners in the enclosing call scope and select either their addresses or nullptr without letting them leave scope early.
3. Verify whether the native storage API consumes or retains the pointed-to data. If it retains it, extend ownership to the documented retention lifetime instead of only to function return.
4. Keep null and empty-string behavior distinct and review error-return handling before exposing the result as a usable storage instance.
5. Keep this path unresolved until a corrected wrapper is compiled and exercised with non-null keys and explicit root directories.

## Conditions and alternatives

This is a source-level lifetime observation, not a reproduced storage failure or a verified patch. The active target/build and cinterop exist, but they do not establish that encryption, custom-root handling or every wrapper method is safe.

## Common mistakes

Do not point at block-local strings and call the native API after the block, assume an ordinary caller-owned const char* has transferred ownership, or report a no-key demo as validation of optional branches.

## Check it works

Use a focused native lifetime checker where available, then exercise default storage, encrypted storage, custom roots and reopen with matching/mismatched keys on the intended PC. Preserve user data during verification.

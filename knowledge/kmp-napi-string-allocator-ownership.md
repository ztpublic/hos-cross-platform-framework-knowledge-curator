---
{
  "schema_version": 1,
  "id": "kmp-napi-string-allocator-ownership",
  "title": "Free KMP bridge strings with their producing allocator",
  "topic_key": "native.kmp-napi-string-allocator-ownership",
  "area": "native",
  "scope": "framework-specific",
  "frameworks": [
    "kotlin-multiplatform"
  ],
  "summary": "Copy the native UTF-8 result into a JS string, then release the original with the exact producer-provided disposal function on every result path.",
  "when_to_use": "Repeated ArkTS calls into a Kotlin/Native library leak native strings, crash during release, or expose pointers after the original buffer is freed.",
  "applicability": "Kotlin/Native OH shared libraries wrapped by a C++ NAPI module. The generated C adapter and its header must match the producing library for the PC target ABI.",
  "keywords": [
    "DisposeString",
    "nativeHeap",
    "IoFree",
    "napi_create_string_utf8",
    "CName",
    "UTF-8",
    "double free",
    "allocator",
    "Kotlin/Native"
  ],
  "related": [
    "kmp-ohos-compose-host",
    "native-ffi-shared-buffer-ownership"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Free KMP bridge strings with their producing allocator

## When to use

Repeated ArkTS calls into a Kotlin/Native library leak native strings, crash during release, or expose pointers after the original buffer is freed.

## Recommended approach

Copy the native UTF-8 result into a JS string, then release the original with the exact producer-provided disposal function on every result path.

## Implementation steps

1. Inspect the generated header and exported implementation before deciding ownership. A const char* signature alone does not identify the allocator.
2. For generated Kotlin String results, use that library's exported symbol table DisposeString. For an explicit nativeHeap allocation, call its exported free function instead; never substitute the generated disposer or an unrelated C allocator.
3. Validate null results, then call napi_create_string_utf8 to copy the bytes into a JS-owned value. Release the original whether JS conversion succeeds or fails.
4. Read JS input length first, allocate length+1 bytes, and copy UTF-8 with the NAPI-reported byte count. Keep input storage alive for the complete synchronous native call.
5. Use structured errors at the boundary and scope cleanup with RAII where practical. Move blocking native work to a worker without passing napi_env or napi_value objects between threads.

## Conditions and alternatives

The reviewed adapters use two different ownership contracts: generated String/DisposeString and explicit UTF-8 buffer/IoFree. They are alternatives, not interchangeable APIs. Returning a copied JS string does not make a base64 bridge zero-copy; large payloads need a bounded binary transport.

## Common mistakes

Do not free generated Kotlin strings with free(), return stack-backed UTF-8 pointers, release before the NAPI copy, or claim JVM tests validate the native ownership boundary.

## Check it works

Exercise empty, non-ASCII, error and long results repeatedly. Instrument native allocation/release counts, force JS conversion failure where feasible, and check that every owned result is released once.

---
{
  "schema_version": 1,
  "id": "jit-executable-memory-authorization",
  "title": "Separate JIT memory authorization from ordinary allocation failure",
  "topic_key": "native.jit-executable-memory-authorization",
  "area": "native",
  "scope": "common",
  "frameworks": [],
  "summary": "Confirm the allocation failure and verify both the declared capability and the actual signing/profile authorization.",
  "when_to_use": "A JIT runtime aborts while initializing executable memory or its code cache.",
  "keywords": [
    "JIT",
    "V8",
    "HotSpot",
    "CodeCache",
    "ALLOW_WRITABLE_CODE_MEMORY",
    "ACL"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to runtimes requiring writable/executable memory under the target PC policy; importing this diagnosis does not establish an Electron or JVM port.",
  "related": []
}
---

# Separate JIT memory authorization from ordinary allocation failure

## When to use

A JIT runtime aborts while initializing executable memory or its code cache.

## Recommended approach

Confirm the allocation failure and verify both the declared capability and the actual signing/profile authorization.

## Implementation steps

1. Locate the failing executable-memory allocation in the native crash log; distinguish it from ordinary memory exhaustion.
2. Where required by the selected product, check ohos.permission.kernel.ALLOW_WRITABLE_CODE_MEMORY and the matching profile ACL.
3. Verify that the application type and certificate are eligible; a manifest permission name alone is insufficient.
4. If authorization is unavailable, evaluate a runtime-supported interpreter or non-JIT mode and measure its changed functionality/performance.

## Conditions and alternatives

Applies to runtimes requiring writable/executable memory under the target PC policy; importing this diagnosis does not establish an Electron or JVM port.

## Common mistakes

Do not treat all ENOMEM as a permission error, assume automatic signing grants system ACLs, or reuse unrelated profiles.

## Check it works

Inspect granted capability and crash logs, then test startup, code generation and shutdown using the intended release/debug signing path.

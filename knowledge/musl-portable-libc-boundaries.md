---
{
  "schema_version": 1,
  "id": "musl-portable-libc-boundaries",
  "title": "Replace glibc-private names at the native ABI boundary",
  "topic_key": "native.musl-portable-libc-boundaries",
  "area": "native",
  "scope": "common",
  "frameworks": [],
  "summary": "Use public POSIX types and explicit byte-layout handling instead of reproducing glibc internals.",
  "when_to_use": "OH native compilation rejects __pid_t, __uid_t or a glibc-only in6_addr member.",
  "keywords": [
    "musl",
    "glibc",
    "__pid_t",
    "__uid_t",
    "s6_addr32"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to C/C++ native dependencies built against an OH musl-based SDK; exact extension availability must be checked in that SDK.",
  "related": []
}
---

# Replace glibc-private names at the native ABI boundary

## When to use

OH native compilation rejects __pid_t, __uid_t or a glibc-only in6_addr member.

## Recommended approach

Use public POSIX types and explicit byte-layout handling instead of reproducing glibc internals.

## Implementation steps

1. Replace private typedef references with pid_t/uid_t and include their defining public headers explicitly.
2. For IPv6 data, use the portable byte representation and validate the input and endianness; preserve the intended wire/parser semantics.
3. Keep platform restrictions separate from compilation: an existing POSIX symbol can still fail under the application sandbox.
4. Limit compatibility changes to diagnosed symbols and test both original and OH targets.

## Conditions and alternatives

Applies to C/C++ native dependencies built against an OH musl-based SDK; exact extension availability must be checked in that SDK.

## Common mistakes

Do not add a reverse shim depending on the missing private type or blindly copy integer bytes without checking parser length and byte order.

## Check it works

Compile public-header probes and verify IPv6 byte sequences and formatting on the intended architectures.

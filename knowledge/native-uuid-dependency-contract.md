---
{
  "schema_version": 1,
  "id": "native-uuid-dependency-contract",
  "title": "Replace a missing UUID dependency without changing identifier semantics",
  "topic_key": "native.native-uuid-dependency-contract",
  "area": "native",
  "scope": "common",
  "frameworks": [],
  "summary": "Determine the UUID operations actually required and use a compatible target implementation or a narrowly defined adapter.",
  "when_to_use": "Native compilation cannot find uuid/uuid.h or link libuuid in the selected OH SDK.",
  "keywords": [
    "libuuid",
    "uuid_generate",
    "QUuid",
    "musl"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Common dependency mechanism. Qt QUuid can be one adapter; other frameworks need their own equivalent implementation. UUID generation is not automatically suitable for secret tokens.",
  "related": []
}
---

# Replace a missing UUID dependency without changing identifier semantics

## When to use

Native compilation cannot find uuid/uuid.h or link libuuid in the selected OH SDK.

## Recommended approach

Determine the UUID operations actually required and use a compatible target implementation or a narrowly defined adapter.

## Implementation steps

1. List used generation, parsing, formatting and comparison functions; inspect return/error conventions.
2. Prefer a supported target library or the framework's UUID implementation when it preserves the needed format and semantics.
3. If providing a compatibility adapter, verify the 16-byte representation, textual format and generation guarantees instead of implementing names only.
4. Remove the unavailable link dependency only after all required consumers use the verified replacement.

## Conditions and alternatives

Common dependency mechanism. Qt QUuid can be one adapter; other frameworks need their own equivalent implementation. UUID generation is not automatically suitable for secret tokens.

## Common mistakes

Do not claim complete libuuid compatibility for a shim implementing only three functions or assume every SDK lacks the library permanently.

## Check it works

Test binary/text round trips, invalid input, formatting, generation behavior and all functions actually used by callers.

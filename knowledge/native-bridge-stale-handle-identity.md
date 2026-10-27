---
{
  "schema_version": 1,
  "id": "native-bridge-stale-handle-identity",
  "title": "Reject stale bridge handles even when native addresses are reused",
  "topic_key": "native.native-bridge-stale-handle-identity",
  "area": "native",
  "scope": "common",
  "frameworks": [],
  "summary": "Expose generation-safe IDs backed by a live-object registry instead of using a native pointer address as the public object identity.",
  "when_to_use": "A late ArkTS or channel callback controls or destroys a newly created native object after an older object was released.",
  "applicability": "In-process OH native bridges that create and destroy objects asynchronously. Relevant to PC media/window recreation; adapters must preserve their own thread and numeric-transport limits.",
  "keywords": [
    "stale handle",
    "ABA",
    "pointer reuse",
    "generation",
    "monotonic ID",
    "native registry",
    "NAPI",
    "unknown player"
  ],
  "related": [
    "flutter-ohos-texture-native-window-lifetime",
    "native-event-loop-handle-teardown"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Reject stale bridge handles even when native addresses are reused

## When to use

A late ArkTS or channel callback controls or destroys a newly created native object after an older object was released.

## Recommended approach

Expose generation-safe IDs backed by a live-object registry instead of using a native pointer address as the public object identity.

## Implementation steps

1. Allocate a new ID for each object lifetime. Use a monotonic counter that cannot wrap during the process lifetime or an index plus checked generation.
2. Store ID-to-live-object mappings internally. Resolve every invoke, render, event, detach and destroy request through that registry.
3. After destruction invalidate the mapping; a repeated destroy can be a documented no-op, while other stale requests return an explicit unknown-object error.
4. Never recover an object by casting a caller-supplied integer back to a pointer. Keep the pointer private to the registry.
5. Serialize registry mutation or add the synchronization required by the calling model. An atomic ID counter alone does not make an unordered_map thread-safe.
6. Keep IDs exact across JS transport: stay within Number.MAX_SAFE_INTEGER when using a number, or choose a supported string/BigInt representation with explicit bounds.

## Conditions and alternatives

This addresses lifetime identity, not permissions or ownership retention. A valid handle still needs an object lifetime guard while work is in flight. The reviewed implementation uses a process-wide monotonic int64 counter; its map assumes the bridge calling model supplies serialization.

## Common mistakes

Do not use an object address as a stable public ID, reuse a free-list slot without a generation, or treat atomic ID allocation as complete thread safety.

## Check it works

Create and destroy many objects while retaining old IDs. Force allocator-address reuse in a controlled harness and send late invoke/destroy calls; the new object must survive. Check counter bounds and concurrent registry access separately.

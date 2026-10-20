---
{
  "schema_version": 1,
  "id": "flutter-player-async-teardown",
  "title": "Await native teardown before replacing a backend",
  "topic_key": "media.flutter-player-async-teardown",
  "area": "media",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "summary": "Use one idempotent teardown Future, cancel subscriptions and await native release before replacement.",
  "when_to_use": "A player replacement races asynchronous native disposal or late stream events.",
  "keywords": [
    "Texture",
    "video_player",
    "media_kit",
    "FVP",
    "MDK",
    "Erika",
    "OHNativeWindow",
    "黑屏",
    "video black screen",
    "audio without video",
    "Texture issue",
    "有声音没画面"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Different media backends have different OH support and integration requirements. A backend excluded by one dependency graph may be viable through another supported implementation.",
  "related": [
    "video-surface-backend-contract"
  ]
}
---

# Await native teardown before replacing a backend

## When to use

A player replacement races asynchronous native disposal or late stream events.

## Recommended approach

Use one idempotent teardown Future, cancel subscriptions and await native release before replacement.

## Implementation steps

A fork may expose `onMediaStatus` as a stream while another version registers a callback, and its `dispose` may return a Future. Adapt the actual resolved API, retain/cancel the stream subscription, mark the owner disposed before late callbacks can reach notifiers, and await native teardown before creating a conflicting replacement. Coalesce synchronous disposal and explicit asynchronous disposal into one teardown Future. Preserve teardown errors for the caller; a fire-and-forget wrapper should report them rather than claim release completed.

Test repeated dispose calls, a pending status event during teardown, delayed/rejected native disposal and rapid backend swaps. Cancellation of a Dart subscription is distinct from completion of native resource release.

## Conditions and alternatives

Different media backends have different OH support and integration requirements. A backend excluded by one dependency graph may be viable through another supported implementation.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Test repeated dispose, late stream events, delayed/rejected native disposal and rapid replacement; verify one release and no replacement before required release completion.

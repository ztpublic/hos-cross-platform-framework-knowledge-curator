---
{
  "schema_version": 1,
  "id": "incoming-wants-before-runtime-ready",
  "topic_key": "lifecycle.incoming-wants-before-runtime-ready",
  "scope": "common",
  "frameworks": [],
  "title": "Deliver incoming shares or deep actions during cold startup",
  "area": "lifecycle",
  "summary": "Buffer native launch payloads until the application runtime explicitly becomes ready, then consume each payload with a defined delivery policy.",
  "when_to_use": "An external share, card action or native launch event is lost before the application subscribes, or is replayed twice after startup.",
  "keywords": [
    "Want",
    "cold start",
    "warm start",
    "EventChannel",
    "分享接收",
    "mailbox",
    "lost share on cold start",
    "Want before Dart ready",
    "event handoff",
    "schedule delivery failure",
    "malformed activation",
    "retention failure",
    "冷启动分享丢失"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": [
    "scheduled-event-delivery-stages"
  ]
}
---

# Deliver incoming shares or deep actions during cold startup

## When to use

An external share, card action or native launch event is lost before the application subscribes, or is replayed twice after startup.

## Recommended approach

Buffer native launch payloads until the application runtime explicitly becomes ready, then consume each payload with a defined delivery policy.

## Implementation steps

1. Normalize cold-start and warm-start native entrypoints into the same payload schema with an event identifier.
2. Store pending payloads while the engine, application subscriptions or required app services are unavailable. Persist them only when delivery must survive process loss.
3. Add a readiness handshake or drain operation after handlers and routing are installed.
4. Define acknowledgement/removal and deduplication semantics. Mark an event consumed at the point appropriate to the app's retry policy.
5. Resolve incoming URI access while it is valid, copying content when it must remain usable later.
6. Clear stale or invalid payloads instead of replaying them during unrelated launches.

## Conditions and alternatives

An in-memory queue may suffice for a running process; a persisted mailbox requires explicit expiry and crash/retry behavior.

See [Separate schedule consumption from completed delivery](scheduled-event-delivery-stages.md) for this independent contract.

## Common mistakes

Changing native startup order does not guarantee asynchronous payload preparation finishes before the application runtime reads it. Do not navigate before dependencies needed by the destination are ready.

## Check it works

Test cold and warm launches, delayed engine/service setup, duplicate events, malformed payloads and process death before acknowledgement.

Also test consumption followed by runtime-start, executor-submission or handler failure; failed diagnostic persistence; bounded-record eviction; and ordinary launch with invalid activation fields. Report submitted, delivered and retained-for-diagnosis as different outcomes.

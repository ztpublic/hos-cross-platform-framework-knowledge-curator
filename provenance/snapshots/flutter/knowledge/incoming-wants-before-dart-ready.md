---
{
  "id": "incoming-wants-before-dart-ready",
  "title": "Deliver incoming shares or deep actions during cold startup",
  "topic_key": "lifecycle.incoming-wants-before-dart-ready",
  "area": "lifecycle",
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
  "summary": "Buffer native launch payloads until Dart explicitly becomes ready, then consume each payload with a defined delivery policy.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "An external share, card action or native launch event is lost before Dart subscribes, or is replayed twice after startup."
}
---

# Deliver incoming shares or deep actions during cold startup

## When to use

An external share, card action or native launch event is lost before Dart subscribes, or is replayed twice after startup.

## Recommended approach

Buffer native launch payloads until Dart explicitly becomes ready, then consume each payload with a defined delivery policy.

## Implementation steps

1. Normalize cold-start and warm-start native entrypoints into the same payload schema with an event identifier.
2. Store pending payloads while the engine, Dart subscriptions or required app services are unavailable. Persist them only when delivery must survive process loss.
3. Add a readiness handshake or drain operation after handlers and routing are installed.
4. Define acknowledgement/removal and deduplication semantics. Mark an event consumed at the point appropriate to the app's retry policy.
5. Resolve incoming URI access while it is valid, copying content when it must remain usable later.
6. Clear stale or invalid payloads instead of replaying them during unrelated launches.

## Conditions and alternatives

An in-memory queue may suffice for a running process; a persisted mailbox requires explicit expiry and crash/retry behavior.

### Separate schedule consumption from completed delivery

If a native schedule or notification starts an application event, distinguish parsing, consuming/re-arming the firing, starting the runtime, submitting work and actual handler acknowledgement. Failure after the schedule record was consumed can otherwise lose the event permanently. Retain a bounded record of the event identity, failed stage and reason when recovery or diagnosis must survive process exit, and surface retention failure separately from the original delivery failure.

A retained failure record is not automatically a retry queue. Define replay, expiry and deduplication separately before re-delivering an action. Malformed activation data should be rejected without aborting an otherwise valid ordinary launch. Adapt the protocol to OH Want/ability entrypoints and the selected scheduler; an Android broadcast receiver implementation does not provide an OH scheduling backend.

## Common mistakes

Changing native startup order does not guarantee asynchronous payload preparation finishes before Dart reads it. Do not navigate before dependencies needed by the destination are ready.

## Check it works

Test cold and warm launches, delayed engine/service setup, duplicate events, malformed payloads and process death before acknowledgement.

Also test consumption followed by runtime-start, executor-submission or handler failure; failed diagnostic persistence; bounded-record eviction; and ordinary launch with invalid activation fields. Report submitted, delivered and retained-for-diagnosis as different outcomes.

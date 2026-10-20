---
{
  "schema_version": 1,
  "id": "scheduled-event-delivery-stages",
  "title": "Separate schedule consumption from completed delivery",
  "topic_key": "lifecycle.scheduled-event-delivery-stages",
  "area": "lifecycle",
  "scope": "common",
  "frameworks": [],
  "summary": "Track consumption, submission and acknowledgement separately, retaining bounded failure records without implying automatic retry.",
  "when_to_use": "A scheduled action is consumed but runtime startup or its handler fails.",
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
  "applicability": "An in-memory queue may suffice for a running process; a persisted mailbox requires explicit expiry and crash/retry behavior.",
  "related": [
    "incoming-wants-before-runtime-ready"
  ]
}
---

# Separate schedule consumption from completed delivery

## When to use

A scheduled action is consumed but runtime startup or its handler fails.

## Recommended approach

Track consumption, submission and acknowledgement separately, retaining bounded failure records without implying automatic retry.

## Implementation steps

If a native schedule or notification starts an application event, distinguish parsing, consuming/re-arming the firing, starting the runtime, submitting work and actual handler acknowledgement. Failure after the schedule record was consumed can otherwise lose the event permanently. Retain a bounded record of the event identity, failed stage and reason when recovery or diagnosis must survive process exit, and surface retention failure separately from the original delivery failure.

A retained failure record is not automatically a retry queue. Define replay, expiry and deduplication separately before re-delivering an action. Malformed activation data should be rejected without aborting an otherwise valid ordinary launch. Adapt the protocol to OH Want/ability entrypoints and the selected scheduler; an Android broadcast receiver implementation does not provide an OH scheduling backend.

## Conditions and alternatives

An in-memory queue may suffice for a running process; a persisted mailbox requires explicit expiry and crash/retry behavior.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Fail after schedule consumption, runtime startup and submission. Test missing acknowledgement, diagnostic-persistence failure, duplicate delivery and invalid activation during ordinary launch.

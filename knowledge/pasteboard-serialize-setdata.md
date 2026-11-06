---
{
  "schema_version": 1,
  "id": "pasteboard-serialize-setdata",
  "title": "Serialize OH pasteboard writes through one queue",
  "topic_key": "clipboard.pasteboard-serialize-setdata",
  "area": "clipboard",
  "scope": "common",
  "frameworks": [],
  "summary": "Run every SystemPasteboard setData and clear call on one promise chain that survives failures, because overlapping writes can be rejected.",
  "when_to_use": "Rapid copy or clear actions fail intermittently with Another setData is being processed, or the wrong value wins.",
  "applicability": "ArkTS and adapter code using @kit.BasicServicesKit pasteboard. The error text comes from a source comment; concurrency limits should be confirmed on the target API. PC boundary: keyboard copy and paste is frequent and can repeat quickly.",
  "keywords": [
    "pasteboard",
    "setData",
    "clearData",
    "Another setData is being processed",
    "clipboard",
    "promise queue",
    "concurrent copy"
  ],
  "related": [
    "media-save-and-clipboard-permissions",
    "pasteboard-image-pixelmap-plus-encoded-record",
    "picker-cancellation-concurrency"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Serialize OH pasteboard writes through one queue

## When to use

Rapid copy or clear actions fail intermittently with Another setData is being processed, or the wrong value wins.

## Recommended approach

Run every SystemPasteboard setData and clear call on one promise chain that survives failures, because overlapping writes can be rejected.

## Implementation steps

1. Keep one private last-write promise per module instance, initialized to a resolved promise.
2. Route every write and clear through an enqueue function that chains on the last promise and returns the task's own result to the caller.
3. Replace the stored chain with one that swallows failure, so a rejected write does not block later ones. Callers still see their own rejection.
4. Check for destruction both on enqueue and when the task starts, and run a cleanup callback for skipped work so owned resources are released.

## Conditions and alternatives

This orders writes within a single instance. Use one shared queue if several modules write, and other processes can still overwrite the pasteboard.

## Common mistakes

Letting one rejection break the chain permanently. Swallowing the error for the caller too. Leaking a PixelMap when a queued write is skipped.

## Check it works

Trigger bursts of mixed text, image and clear operations, force one to fail, and confirm later writes still run in order and the final value is the last one requested.

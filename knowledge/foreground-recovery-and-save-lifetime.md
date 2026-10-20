---
{
  "schema_version": 1,
  "id": "foreground-recovery-and-save-lifetime",
  "topic_key": "lifecycle.foreground-recovery-and-save-lifetime",
  "scope": "common",
  "frameworks": [],
  "title": "Recover connections and save state around app suspension",
  "area": "lifecycle",
  "summary": "Separate transport health, application readiness and durable save completion. Use lifecycle callbacks as additional recovery/save opportunities rather than guarantees.",
  "when_to_use": "Resuming the app unnecessarily reconnects a healthy transport, or recent edits disappear after the app is suspended or terminated.",
  "keywords": [
    "resume",
    "pause",
    "flush",
    "unawaited",
    "SSH",
    "恢复",
    "持久化",
    "data lost on suspend",
    "reconnect on resume",
    "保存未完成",
    "恢复连接"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": [
    "transport-reply-watchdog"
  ]
}
---

# Recover connections and save state around app suspension

## When to use

Resuming the app unnecessarily reconnects a healthy transport, or recent edits disappear after the app is suspended or terminated.

## Recommended approach

Separate transport health, application readiness and durable save completion. Use lifecycle callbacks as additional recovery/save opportunities rather than guarantees.

## Implementation steps

1. Keep pending edits in an explicit queue and save incrementally during normal interaction, not only when leaving the foreground.
2. On suspension-related callbacks, request a flush without assuming the process will remain alive until it finishes.
3. For file-backed state, use a suitable temporary-write/close/rename strategy and keep enough recovery data for startup validation.
4. On resume, distinguish a slow/rejected application probe from a confirmed transport disconnection. Reconnect only for the condition the connection contract defines as broken.
5. Cancel obsolete reconnect/probe tasks and use a generation or session identifier to ignore late results.
6. Restore state after its storage and dependent services signal readiness. A fixed startup delay can race initialization. Keep a validated recovery snapshot if loading remote content fails transiently instead of deleting it as corrupt data.

Coalesce concurrent reconnect requests around one in-flight operation, with an explicit success/error result shared by every waiter. Release the in-flight marker in `finally`, but do not complete all waiters successfully if the underlying connection threw. After transport recovery, run application initialization and state refresh in order; socket connection alone does not restore the feature.

## Conditions and alternatives

Use an actual native background mechanism when a legitimate activity must continue after suspension. Atomic file replacement reduces partial-update risk but its durability still depends on the filesystem and flush behavior.

See [Keep reply watchdogs and native protection separate](transport-reply-watchdog.md) for this independent contract.

## Common mistakes

Do not interpret every server error as a dead socket. Starting an asynchronous flush from a lifecycle callback does not prove it completed before termination.

Debouncing every position event can postpone a playback checkpoint indefinitely while events keep arriving. Use a bounded checkpoint interval and await the actual write/flush for explicit save operations; an `immediate` flag that merely calls an asynchronous store without awaiting it does not establish durability.

## Check it works

Test rapid pause/resume, slow servers, confirmed socket loss, termination during saving, corrupt-state recovery and stale reconnect completion.

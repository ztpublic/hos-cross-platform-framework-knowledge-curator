---
{
  "id": "foreground-recovery-and-save-lifetime",
  "title": "Recover connections and save state around app suspension",
  "topic_key": "lifecycle.foreground-recovery-and-save-lifetime",
  "area": "lifecycle",
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
  "summary": "Separate transport health, application readiness and durable save completion. Use lifecycle callbacks as additional recovery/save opportunities rather than guarantees.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "Resuming the app unnecessarily reconnects a healthy transport, or recent edits disappear after the app is suspended or terminated."
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

### Keep reply watchdogs and native protection separate

For a reply-based SSH health check, keep one pending ping Future and let its subscribers share the outcome. After a timeout, a short resume grace can await that same request so buffered replies can run; sending another ping can grow an outstanding reply queue on a blackholed connection. On final failure, close the owned transport and settle pending operations. Do not classify a busy application RPC server as a failed SSH transport.

Coalesce noisy network-change events and probe current transport/application readiness before reconnecting. A cached native keepalive flag should be reconciled against actual service state where a backend supports it. Bind native channels only after Flutter binding setup. An Android service/network observer cannot supply OH background protection; the shared recovery policy still needs an OH backend or an honest unsupported result.

## Common mistakes

Do not interpret every server error as a dead socket. Starting an asynchronous flush from a lifecycle callback does not prove it completed before termination.

Debouncing every position event can postpone a playback checkpoint indefinitely while events keep arriving. Use a bounded checkpoint interval and await the actual write/flush for explicit save operations; an `immediate` flag that merely calls an asynchronous store without awaiting it does not establish durability.

## Check it works

Test rapid pause/resume, slow servers, confirmed socket loss, termination during saving, corrupt-state recovery and stale reconnect completion.

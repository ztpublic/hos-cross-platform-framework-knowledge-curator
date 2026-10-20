---
{
  "schema_version": 1,
  "id": "transport-reply-watchdog",
  "title": "Keep reply watchdogs and native protection separate",
  "topic_key": "lifecycle.transport-reply-watchdog",
  "area": "lifecycle",
  "scope": "common",
  "frameworks": [],
  "summary": "Share one pending transport probe, classify its result and coalesce reconnect work separately from native background eligibility.",
  "when_to_use": "A resume or network change starts redundant health probes and reconnects.",
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
  "applicability": "Use an actual native background mechanism when a legitimate activity must continue after suspension. Atomic file replacement reduces partial-update risk but its durability still depends on the filesystem and flush behavior.",
  "related": [
    "foreground-recovery-and-save-lifetime"
  ]
}
---

# Keep reply watchdogs and native protection separate

## When to use

A resume or network change starts redundant health probes and reconnects.

## Recommended approach

Share one pending transport probe, classify its result and coalesce reconnect work separately from native background eligibility.

## Implementation steps

For a reply-based SSH health check, keep one pending ping Future and let its subscribers share the outcome. After a timeout, a short resume grace can await that same request so buffered replies can run; sending another ping can grow an outstanding reply queue on a blackholed connection. On final failure, close the owned transport and settle pending operations. Do not classify a busy application RPC server as a failed SSH transport.

Coalesce noisy network-change events and probe current transport/application readiness before reconnecting. A cached native keepalive flag should be reconciled against actual service state where a backend supports it. Bind native bridges only after the owning runtime initializes. An Android service/network observer cannot supply OH background protection; the shared recovery policy still needs an OH backend or an honest unsupported result.

## Conditions and alternatives

Use an actual native background mechanism when a legitimate activity must continue after suspension. Atomic file replacement reduces partial-update risk but its durability still depends on the filesystem and flush behavior.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Test one delayed ping with multiple subscribers, timeout followed by a buffered reply, a blackholed connection, busy application RPC and network-event bursts. Confirm one probe and honest shared failure.

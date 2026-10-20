---
{
  "schema_version": 1,
  "id": "async-bridge-shutdown-wait",
  "title": "Avoid waiting for a stopped event loop during shutdown",
  "topic_key": "lifecycle.async-bridge-shutdown-wait",
  "area": "lifecycle",
  "scope": "common",
  "frameworks": [],
  "summary": "Complete cross-runtime work before teardown and use bounded cancellable waits only where the supplying event loop can progress.",
  "when_to_use": "Shutdown hangs waiting for an asynchronous callback after the native JS environment stops processing work.",
  "keywords": [
    "TSFN",
    "deadlock",
    "shutdown",
    "Promise",
    "appfreeze"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to bridges where the environment may stop pumping callbacks during destruction. A timeout must not destroy state still used by a late callback.",
  "related": [
    "native-event-loop-handle-teardown"
  ]
}
---

# Avoid waiting for a stopped event loop during shutdown

## When to use

Shutdown hangs waiting for an asynchronous callback after the native JS environment stops processing work.

## Recommended approach

Complete cross-runtime work before teardown and use bounded cancellable waits only where the supplying event loop can progress.

## Implementation steps

1. Identify the queue/event loop that will settle the pending result.
2. Prefer asynchronous completion; never block that same queue or a UI thread waiting for itself.
3. If adapting a synchronous API on a worker, use a bounded wait and shared completion ownership that survives timeout without publishing stale results.
4. During teardown invalidate pending requests, avoid new work depending on a destroyed runtime and release native resources through their own supported stop protocol.

## Conditions and alternatives

Applies to bridges where the environment may stop pumping callbacks during destruction. A timeout must not destroy state still used by a late callback.

## Common mistakes

A nested event loop or unbounded future wait is not a reliable shutdown protocol; callback rejection and native resource release are separate.

## Check it works

Test unresolved promises, environment teardown, late callback after timeout and repeated shutdown without blocking the UI.

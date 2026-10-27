---
{
  "schema_version": 1,
  "id": "native-ohos-worker-stack-budget",
  "title": "Set a native worker stack budget for deep Kotlin calls",
  "topic_key": "native.native-ohos-worker-stack-budget",
  "area": "native",
  "scope": "common",
  "frameworks": [],
  "summary": "Set an explicit measured stack size before pthread creation, check each creation failure, and preserve deterministic thread ownership and join behavior.",
  "when_to_use": "A deep Kotlin/Native call chain overflows an OH pthread worker stack although the same work succeeds on another host thread.",
  "applicability": "Native OH worker threads that run deep generated/runtime call chains, including PC render workers. Stack defaults and memory budgets are implementation-dependent; no fixed size is universally required.",
  "keywords": [
    "pthread_attr_setstacksize",
    "stack overflow",
    "Kotlin/Native",
    "8 MiB",
    "pthread_create",
    "std::thread",
    "worker stack"
  ],
  "related": [
    "native-event-loop-handle-teardown",
    "async-bridge-shutdown-wait"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Set a native worker stack budget for deep Kotlin calls

## When to use

A deep Kotlin/Native call chain overflows an OH pthread worker stack although the same work succeeds on another host thread.

## Recommended approach

Set an explicit measured stack size before pthread creation, check each creation failure, and preserve deterministic thread ownership and join behavior.

## Implementation steps

1. Confirm stack exhaustion from the crash evidence and call depth. Exclude use-after-free and unbounded recursion before increasing the budget.
2. Initialize pthread_attr_t and set the stack size before pthread_create. The reviewed worker uses 8 * 1024 * 1024 bytes; choose a budget for the actual runtime and concurrent worker count.
3. Check pthread_attr_init, pthread_attr_setstacksize and pthread_create return codes. Destroy attributes and free the heap-owned callable on every failed creation path.
4. Transfer the callable to the worker only after successful creation. Join it before destroying state captured by the callable.
5. Reject self-join and double join. Preserve the surrounding loop's startup readiness handshake and stop/close sequence when replacing std::thread.

## Conditions and alternatives

The source reports an approximately 1 MiB default for its HarmonyOS build and uses 8 MiB for a deep Kotlin/Native workload. Neither value is a product-wide default or a benchmark. Retaining joinable-thread failure semantics avoids silently detaching code that still uses owner state.

## Common mistakes

Do not increase every worker stack without considering memory use, lose callable ownership on pthread_create failure, or join the worker from itself.

## Check it works

Reproduce the deep workload on the intended PC, measure stack headroom and total memory at maximum worker count, and exercise creation failure and shutdown with queued work. Run long enough to exclude merely delaying an unbounded recursion failure.

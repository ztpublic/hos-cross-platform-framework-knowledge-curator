---
{
  "schema_version": 1,
  "id": "napi-thread-and-handle-lifetime",
  "title": "Keep Node-API values on their owning environment and handle lifetime",
  "topic_key": "native.napi-thread-and-handle-lifetime",
  "area": "native",
  "scope": "common",
  "frameworks": [],
  "summary": "Marshal JS work to the owning environment and pass ordinary native values across threads; retain JS objects with the supported persistent-reference mechanism when needed.",
  "when_to_use": "Native callbacks crash when a JS value crosses threads or is reused in a later asynchronous turn.",
  "keywords": [
    "Node-API",
    "napi_value",
    "napi_ref",
    "GC",
    "线程"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Common native Node-API integration rule; concrete dispatchers and reference wrappers depend on the selected OH runtime/framework.",
  "related": []
}
---

# Keep Node-API values on their owning environment and handle lifetime

## When to use

Native callbacks crash when a JS value crosses threads or is reused in a later asynchronous turn.

## Recommended approach

Marshal JS work to the owning environment and pass ordinary native values across threads; retain JS objects with the supported persistent-reference mechanism when needed.

## Implementation steps

1. Identify the environment/thread owning each napi_value or framework wrapper.
2. Convert returned data to an owned native representation before leaving the JS callback/thread.
3. For later callbacks, create values in the callback's current environment or retain/release a valid persistent reference according to the runtime API.
4. Invalidate pending work at environment destruction and finish release while its owner remains valid.

## Conditions and alternatives

Common native Node-API integration rule; concrete dispatchers and reference wrappers depend on the selected OH runtime/framework.

## Common mistakes

Capturing a transient wrapper by value does not make its underlying JS handle persistent or cross-thread safe.

## Check it works

Exercise delayed callbacks, garbage collection, worker delivery and environment teardown; verify values and references remain valid only for their declared lifetime.

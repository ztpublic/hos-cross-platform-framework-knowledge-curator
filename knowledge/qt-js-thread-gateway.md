---
{
  "schema_version": 1,
  "id": "qt-js-thread-gateway",
  "title": "Dispatch Qt native calls through the owning JS gateway",
  "topic_key": "native.qt-js-thread-gateway",
  "area": "native",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Use the selected integration's JS gateway and return copied native values instead of JS handles.",
  "when_to_use": "Qt event-loop code needs an ArkTS/system API but cannot safely access Node-API objects directly.",
  "keywords": [
    "QOhosJsThreadGateway",
    "CorePrivate",
    "ace_napi.z",
    "QNapi"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "The gateway is a private API with no general version/ABI promise. Qt 6 may reorganize it.",
  "related": [
    "napi-thread-and-handle-lifetime"
  ]
}
---

# Dispatch Qt native calls through the owning JS gateway

## When to use

Qt event-loop code needs an ArkTS/system API but cannot safely access Node-API objects directly.

## Recommended approach

Use the selected integration's JS gateway and return copied native values instead of JS handles.

## Implementation steps

1. Prefer a public Qt API when it implements the needed operation.
2. For a pinned Qt 5 private integration, inspect QOhosJsThreadGateway in qcore_ohos_p.h and its asynchronous/result-returning variants.
3. Supply matching private headers, node-addon-api include inputs and required native Node-API linkage where the SDK does not transitively provide them.
4. Keep JS-facing work inside the gateway callback and deliver ordinary values back to Qt; dispatch reverse-direction UI work to the Qt owner.

## Conditions and alternatives

The gateway is a private API with no general version/ABI promise. Qt 6 may reorganize it.

## Common mistakes

Do not use the backend registration interface as an application facade or return transient QNapi values from another thread.

## Check it works

Build a minimal call, verify thread identities and error propagation, and test delayed completion plus shutdown.

---
{
  "id": "network-adapter-policy-propagation",
  "title": "Preserve HTTP behavior when changing the OH transport adapter",
  "topic_key": "network.network-adapter-policy-propagation",
  "area": "network",
  "keywords": [
    "dart:io",
    "rhttp",
    "WebDAV",
    "TLS",
    "certificate",
    "HTTP adapter",
    "网络",
    "WebDAV test succeeds upload fails",
    "TLS option ignored",
    "HTTP适配器",
    "上传失败"
  ],
  "summary": "Build all client operations from one explicit transport policy and propagate it consistently into the OH adapter.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "Connection testing succeeds but uploads/downloads fail, or switching away from a native HTTP library changes TLS, timeout, proxy or cancellation behavior."
}
---

# Preserve HTTP behavior when changing the OH transport adapter

## When to use

Connection testing succeeds but uploads/downloads fail, or switching away from a native HTTP library changes TLS, timeout, proxy or cancellation behavior.

## Recommended approach

Build all client operations from one explicit transport policy and propagate it consistently into the OH adapter.

## Implementation steps

1. List every client construction path: connectivity test, metadata/list requests, upload, download and retry.
2. Define shared policy fields for authentication, certificates, proxy, timeouts and cancellation. Separate endpoint-specific values from global defaults.
3. Centralize adapter creation so the OH route receives the same intended policy for every operation.
4. Preserve strict certificate validation by default. If the product exposes an exception for a specific endpoint, scope and label it explicitly rather than using it to mask unexplained TLS failures.
5. Close old clients and cancel stale operations when policy or session changes.

## Conditions and alternatives

Pure Dart socket/HTTP/SSH logic can remain shared when it satisfies the feature contract; an OH port does not inherently require rewriting the protocol in ArkTS.

## Common mistakes

A successful probe does not verify the transfer clients. Do not allow per-endpoint policy to leak into unrelated clients or assume two adapters implement every option identically.

## Check it works

Exercise valid/invalid certificates, cancellation, timeout, upload/download and retries against the same endpoint. Confirm default validation remains strict.

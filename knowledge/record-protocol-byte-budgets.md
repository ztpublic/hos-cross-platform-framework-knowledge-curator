---
{
  "schema_version": 1,
  "id": "record-protocol-byte-budgets",
  "title": "Bound record pages by encoded bytes and explicit progress",
  "topic_key": "integration.record-protocol-byte-budgets",
  "area": "integration",
  "scope": "common",
  "frameworks": [],
  "summary": "Budget encoded bytes including envelope overhead and treat exhaustion separately from page length.",
  "when_to_use": "A record synchronization page exceeds the message ceiling or stalls after a short byte-limited page.",
  "keywords": [
    "ArkWeb",
    "JavaScript bridge",
    "chunk",
    "backpressure",
    "atomic rename",
    "short write",
    "Uint8Array",
    "大文件",
    "分块传输",
    "临时文件"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to framed record/page protocols; transport message limits, native bridge limits and database transaction limits are separate.",
  "related": [
    "chunked-native-file-transfer"
  ]
}
---

# Bound record pages by encoded bytes and explicit progress

## When to use

A record synchronization page exceeds the message ceiling or stalls after a short byte-limited page.

## Recommended approach

Budget encoded bytes including envelope overhead and treat exhaustion separately from page length.

## Implementation steps

For record/page protocols, bound encoded bytes as well as item count. Reserve space for the actual envelope/framing and keep indivisible records whole. Return a specific capacity error for one record larger than the connection ceiling; silently dropping it or returning an empty page can stall synchronization. A short byte-limited page does not imply end of data: advance through acknowledged progress until explicit exhaustion, with a bounded work budget per pass. Native bridge limits, transport limits and database commit limits are separate constraints.

## Conditions and alternatives

Applies to framed record/page protocols; transport message limits, native bridge limits and database transaction limits are separate.

## Common mistakes

Do not drop oversized records, split an indivisible record silently, or infer end-of-data from a short page.

## Check it works

Test Unicode expansion, envelope overhead, one oversized record, a short nonterminal page and retries after partial acknowledged progress.

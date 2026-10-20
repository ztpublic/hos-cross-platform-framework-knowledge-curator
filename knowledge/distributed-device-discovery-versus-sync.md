---
{
  "schema_version": 1,
  "id": "distributed-device-discovery-versus-sync",
  "topic_key": "integration.distributed-device-discovery-versus-sync",
  "scope": "common",
  "frameworks": [],
  "title": "Retrieve and apply data after distributed-device discovery",
  "area": "integration",
  "summary": "Treat discovery, authorization, publish, synchronization, retrieval and application as separate states with explicit completion/error handling.",
  "when_to_use": "The device list appears and synchronization is requested, but configuration/data never arrives or is never applied.",
  "keywords": [
    "distributed KV",
    "device_list",
    "PUSH_PULL",
    "分布式",
    "shared_config",
    "device collaboration",
    "device found no data",
    "distributed sync returns null",
    "分布式同步无数据"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": []
}
---

# Retrieve and apply data after distributed-device discovery

## When to use

The device list appears and synchronization is requested, but configuration/data never arrives or is never applied.

## Recommended approach

Treat discovery, authorization, publish, synchronization, retrieval and application as separate states with explicit completion/error handling.

## Implementation steps

1. Define the shared-data schema, scope and which devices/users may receive it.
2. Establish device/trust/permission prerequisites before offering transfer as ready.
3. Publish the intended data and track the synchronization request's completion; discovery alone does not transfer the value.
4. Implement an actual read/result path after pull, or a clearly owned remote-change callback. Validate the retrieved schema before applying it.
5. Handle empty values, conflicts, stale versions and unavailable devices without claiming success.
6. Review native storage protection, automatic synchronization settings and logging for sensitive configuration fields.

## Conditions and alternatives

Native distributed storage/device services depend on product capabilities and trust conditions. A callback delivery route and an explicit pull route can coexist but must not lose or double-apply data.

## Common mistakes

Do not leave a retrieval stub returning an empty value while the UI reports transfer completion. A storage option does not by itself establish transport security; avoid logging secret configuration.

## Check it works

Test each state separately, including denied access, empty pull results, remote updates, conflicts, device disappearance and successful application of the received value.

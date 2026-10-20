---
{
  "schema_version": 1,
  "id": "system-metric-unavailability",
  "title": "Represent unavailable system metrics separately from zero",
  "topic_key": "integration.system-metric-unavailability",
  "area": "integration",
  "scope": "common",
  "frameworks": [],
  "summary": "Check each metric's real data source under app privileges and use an explicit unavailable result or supported native backend.",
  "when_to_use": "A Linux-oriented system-information library returns empty CPU, disk or process statistics on OH.",
  "keywords": [
    "procfs",
    "sysinfo",
    "CPU",
    "unavailable",
    "系统信息"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Access depends on device/API/application privileges. A source report about one procfs file is not a permanent availability table.",
  "related": []
}
---

# Represent unavailable system metrics separately from zero

## When to use

A Linux-oriented system-information library returns empty CPU, disk or process statistics on OH.

## Recommended approach

Check each metric's real data source under app privileges and use an explicit unavailable result or supported native backend.

## Implementation steps

1. Map each metric to its procfs or native API source.
2. Inspect access and parsing in the application process, not a privileged shell.
3. Distinguish denial, unimplemented API, parser failure, empty collection and true zero.
4. Choose a product-supported API or visible feature degradation; review target cfg that selected a desktop Linux backend.

## Conditions and alternatives

Access depends on device/API/application privileges. A source report about one procfs file is not a permanent availability table.

## Common mistakes

Do not fabricate zero or assume one readable metric proves every metric is available.

## Check it works

Exercise each required metric with known workload changes and with denied/unavailable data sources.

---
{
  "schema_version": 1,
  "id": "ohpm-network-diagnosis",
  "title": "Diagnose OHPM registry failures before changing dependencies",
  "topic_key": "build.ohpm-network-diagnosis",
  "area": "build",
  "scope": "common",
  "frameworks": [],
  "summary": "Check the configured client, registry and network/proxy path before altering lockfiles or dependency versions.",
  "when_to_use": "ohpm install times out or cannot reach its registry.",
  "keywords": [
    "ohpm",
    "registry",
    "timeout",
    "代理"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to any generated or handwritten OH native host using OHPM.",
  "related": []
}
---

# Diagnose OHPM registry failures before changing dependencies

## When to use

ohpm install times out or cannot reach its registry.

## Recommended approach

Check the configured client, registry and network/proxy path before altering lockfiles or dependency versions.

## Implementation steps

1. Record the actual OHPM executable/version and active registry.
2. Differentiate DNS, TLS, proxy and temporary endpoint failures using the same environment as the build.
3. Use an approved reachable registry or repair the network configuration without baking a temporary mirror into framework code.
4. Read back configuration and repeat dependency installation, preserving the intended lock.

## Conditions and alternatives

Applies to any generated or handwritten OH native host using OHPM.

## Common mistakes

Deleting lockfiles or repeated cache clearing does not fix a registry network failure.

## Check it works

Verify retrieval in the actual build environment and confirm the resolved dependency identities remain intentional.

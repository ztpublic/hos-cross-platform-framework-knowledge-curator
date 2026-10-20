---
{
  "schema_version": 1,
  "id": "hap-signing-identity",
  "title": "Keep product signing and application identity consistent",
  "topic_key": "build.hap-signing-identity",
  "area": "build",
  "scope": "common",
  "frameworks": [],
  "summary": "Align application/module identity with the selected product's signingConfig and authorized profile.",
  "when_to_use": "A HAP is unsigned, rejected, or launches under an unexpected bundle identity.",
  "keywords": [
    "bundleName",
    "signingConfig",
    "签名",
    "profile"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to HAP-based integrations across frameworks. Changing bundle identity may create a different storage owner and require an explicit data migration.",
  "related": [
    "sandbox-paths-and-app-identity"
  ]
}
---

# Keep product signing and application identity consistent

## When to use

A HAP is unsigned, rejected, or launches under an unexpected bundle identity.

## Recommended approach

Align application/module identity with the selected product's signingConfig and authorized profile.

## Implementation steps

1. Use a stable bundleName accepted by the selected SDK schema and carry it into generated configuration and launch targets.
2. Check that the active product references its intended signingConfig; defining the config alone does not select it.
3. Verify profile identity, device eligibility and permission authorization without publishing credential material.
4. Inspect the actual output artifact, then separately verify installation, launch and required features.

## Conditions and alternatives

Applies to HAP-based integrations across frameworks. Changing bundle identity may create a different storage owner and require an explicit data migration.

## Common mistakes

A signed artifact is not automatically authorized for every device or system permission; changing bundleName is not a storage migration.

## Check it works

Inspect selected product identity/signature and verify the installed app's identity and data location.

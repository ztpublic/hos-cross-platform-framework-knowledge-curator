---
{
  "schema_version": 1,
  "id": "privileged-capability-gating",
  "title": "Check product and signing eligibility before promising a system capability",
  "topic_key": "integration.privileged-capability-gating",
  "area": "integration",
  "scope": "common",
  "frameworks": [],
  "summary": "Gate the capability on API level, product scope, application type, actual grants and device support.",
  "when_to_use": "An SDK exposes an API or permission, but an ordinary application cannot invoke the feature.",
  "keywords": [
    "hotspot",
    "system permission",
    "ACL",
    "热点",
    "权限"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "For example, an API for programmatic hotspot management can be restricted to particular non-general products; verify the intended PC policy rather than promising access from an API name.",
  "related": [
    "capability-gated-backend-initialization"
  ]
}
---

# Check product and signing eligibility before promising a system capability

## When to use

An SDK exposes an API or permission, but an ordinary application cannot invoke the feature.

## Recommended approach

Gate the capability on API level, product scope, application type, actual grants and device support.

## Implementation steps

1. Inspect the target SDK declaration and product restrictions, including deprecated or system-only status.
2. Check whether the application can request the permission and whether its signed profile actually grants it.
3. Query the device capability and keep rejection distinct from an empty successful result.
4. Provide a supported chooser/settings/user-mediated alternative or mark the feature unavailable.

## Conditions and alternatives

For example, an API for programmatic hotspot management can be restricted to particular non-general products; verify the intended PC policy rather than promising access from an API name.

## Common mistakes

A stub, permission string or Linux backend does not grant privileged capability.

## Check it works

Test eligible and denied configurations and verify the UI follows the actual capability result.

---
{
  "schema_version": 1,
  "id": "native-form-current-data-provider",
  "title": "Read current domain data when a saved display snapshot is insufficient",
  "topic_key": "integration.native-form-current-data-provider",
  "area": "integration",
  "scope": "common",
  "frameworks": [],
  "summary": "Use a verified native read-only data provider and derive fresh display state without starting the main runtime.",
  "when_to_use": "A home-screen card shows stale daily data while the application runtime is absent.",
  "keywords": [
    "FormExtensionAbility",
    "formBindingData",
    "卡片",
    "home widget",
    "refresh",
    "JSON",
    "home screen widget",
    "Form card",
    "卡片更新",
    "桌面卡片"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Check refresh API units and scheduling constraints for the selected SDK. A relative interval must not be replaced with an absolute epoch timestamp.",
  "related": [
    "native-form-snapshot-bridge"
  ]
}
---

# Read current domain data when a saved display snapshot is insufficient

## When to use

A home-screen card shows stale daily data while the application runtime is absent.

## Recommended approach

Use a verified native read-only data provider and derive fresh display state without starting the main runtime.

## Implementation steps

An alternative to the application runtime publishing today's rendered items is a native read-only provider over a genuinely shared database. Verify the actual OH plugin database location, schema/version, store configuration and cross-process access before using `relationalStore` to derive a typed card payload. Let one layer own schema migrations; the Form process should not create a new empty store at a guessed path. Close result sets and the native store in finally, and tolerate a missing/uninitialized database with a usable empty/error presentation.

Return initial form binding data promptly, then refresh asynchronously when the instance is registered. Keep 64-bit Form IDs as strings throughout. Per-instance storage avoids one process rewriting another process's form-index list, but refresh/removal still need race handling. Compute day/week freshness natively from persisted domain data, isolate failure of one form from others, and use documented relative refresh units. A fixed delay before update is an observed workaround rather than proof of registration; use a supported readiness event or bounded retry when available.

Test a closed app at a day boundary, multiple instances, removal during refresh, an uninitialized database and a schema upgrade. Check the plugin's real path mapping rather than trusting a same-named database file.

## Conditions and alternatives

Check refresh API units and scheduling constraints for the selected SDK. A relative interval must not be replaced with an absolute epoch timestamp.

For countdowns, persist the absolute end time and total duration rather than only a previously rendered remaining value. On a native refresh, calculate the new remaining time and bounded progress from the current clock, and clear the active state after expiry. This can refresh an existing countdown without the application runtime; computing the next day's items still requires sufficiently complete persisted domain data or a separate data provider. Scheduled card refresh is not a per-minute delivery guarantee.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Test the app closed across a day boundary, multiple form IDs, removal during refresh, missing database and schema upgrade. Confirm current payloads and closed native cursors/stores.

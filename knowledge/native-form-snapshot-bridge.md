---
{
  "schema_version": 1,
  "id": "native-form-snapshot-bridge",
  "topic_key": "integration.native-form-snapshot-bridge",
  "scope": "common",
  "frameworks": [],
  "title": "Build an OH home-screen card backed by application data",
  "area": "integration",
  "summary": "Keep card rendering and its initial data response native; publish stable snapshots from the application runtime and route card actions through a readiness-aware bridge.",
  "when_to_use": "You want a home-screen Form card that shows application data or opens an application action while the app may not be running.",
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
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": [
    "native-form-current-data-provider"
  ]
}
---

# Build an OH home-screen card backed by application data

## When to use

You want a home-screen Form card that shows application data or opens an application action while the app may not be running.

## Recommended approach

Keep card rendering and its initial data response native; publish stable snapshots from the application runtime and route card actions through a readiness-aware bridge.

## Implementation steps

1. Define a typed, versioned payload containing only what the card needs, such as a title, current items and action identifiers.
2. Have the application runtime compute the payload and send it to native storage when relevant data changes.
3. In FormExtensionAbility, return usable initial data promptly and update registered form instances from the persisted snapshot.
4. Track form creation/removal and keep per-instance IDs so updates reach the intended cards.
5. Define where fresh data comes from when the application runtime is absent. Native refresh of yesterday's saved payload does not calculate today's data.
6. Route card actions through the launch/event buffer until application handlers are ready.

## Conditions and alternatives

Check refresh API units and scheduling constraints for the selected SDK. A relative interval must not be replaced with an absolute epoch timestamp.

For countdowns, persist the absolute end time and total duration rather than only a previously rendered remaining value. On a native refresh, calculate the new remaining time and bounded progress from the current clock, and clear the active state after expiry. This can refresh an existing countdown without the application runtime; computing the next day's items still requires sufficiently complete persisted domain data or a separate data provider. Scheduled card refresh is not a per-minute delivery guarantee.

See [Read current domain data when a saved display snapshot is insufficient](native-form-current-data-provider.md) for this independent contract.

## Common mistakes

Do not attempt to render the shared framework widget tree directly inside the native card. Disabling one framework home-widget package does not mean native OH Forms are unavailable.

## Check it works

Add a card with the app closed; test multiple instances, removal, schema upgrades, day-boundary freshness, scheduled refresh and cold-start action delivery.

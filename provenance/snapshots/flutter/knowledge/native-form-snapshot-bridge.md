---
{
  "id": "native-form-snapshot-bridge",
  "title": "Build an OH home-screen card backed by Flutter data",
  "topic_key": "integration.native-form-snapshot-bridge",
  "area": "integration",
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
  "summary": "Keep card rendering and its initial data response native; publish stable snapshots from Flutter and route card actions through a readiness-aware bridge.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "You want a home-screen Form card that shows application data or opens a Flutter action while the app may not be running."
}
---

# Build an OH home-screen card backed by Flutter data

## When to use

You want a home-screen Form card that shows application data or opens a Flutter action while the app may not be running.

## Recommended approach

Keep card rendering and its initial data response native; publish stable snapshots from Flutter and route card actions through a readiness-aware bridge.

## Implementation steps

1. Define a typed, versioned payload containing only what the card needs, such as a title, current items and action identifiers.
2. Have Flutter compute the payload and send it to native storage when relevant data changes.
3. In FormExtensionAbility, return usable initial data promptly and update registered form instances from the persisted snapshot.
4. Track form creation/removal and keep per-instance IDs so updates reach the intended cards.
5. Define where fresh data comes from when Flutter is absent. Native refresh of yesterday's saved payload does not calculate today's data.
6. Route card actions through the launch/event buffer until Flutter handlers are ready.

## Conditions and alternatives

Check refresh API units and scheduling constraints for the selected SDK. A relative interval must not be replaced with an absolute epoch timestamp.

For countdowns, persist the absolute end time and total duration rather than only a previously rendered remaining value. On a native refresh, calculate the new remaining time and bounded progress from the current clock, and clear the active state after expiry. This can refresh an existing countdown without Flutter; computing the next day's items still requires sufficiently complete persisted domain data or a separate data provider. Scheduled card refresh is not a per-minute delivery guarantee.

### Read current domain data when a saved display snapshot is insufficient

An alternative to Flutter publishing today's rendered items is a native read-only provider over a genuinely shared database. Verify the actual OH plugin database location, schema/version, store configuration and cross-process access before using `relationalStore` to derive a typed card payload. Let one layer own schema migrations; the Form process should not create a new empty store at a guessed path. Close result sets and the native store in finally, and tolerate a missing/uninitialized database with a usable empty/error presentation.

Return initial form binding data promptly, then refresh asynchronously when the instance is registered. Keep 64-bit Form IDs as strings throughout. Per-instance storage avoids one process rewriting another process's form-index list, but refresh/removal still need race handling. Compute day/week freshness natively from persisted domain data, isolate failure of one form from others, and use documented relative refresh units. A fixed delay before update is an observed workaround rather than proof of registration; use a supported readiness event or bounded retry when available.

Test a closed app at a day boundary, multiple instances, removal during refresh, an uninitialized database and a schema upgrade. Check the plugin's real path mapping rather than trusting a same-named database file.

## Common mistakes

Do not attempt to render the Flutter widget tree directly inside the native card. Disabling a Flutter home-widget package does not mean native OH Forms are unavailable.

## Check it works

Add a card with the app closed; test multiple instances, removal, schema upgrades, day-boundary freshness, scheduled refresh and cold-start action delivery.

---
{
  "schema_version": 1,
  "id": "flutter-native-material-state",
  "title": "Synchronize native material settings as effective state",
  "topic_key": "integration.flutter-native-material-state",
  "area": "integration",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "summary": "Validate one typed setting map and replay desired material state after native readiness, preserving failed requests for retry.",
  "when_to_use": "A Flutter setting claims a native HDS material level is applied before the shell accepts it.",
  "keywords": [
    "ArkUI",
    "HDS",
    "native tabs",
    "color mode",
    "theme",
    "原生外壳",
    "windowWillClose",
    "native tabs out of sync",
    "native dark mode mismatch",
    "ArkUI Flutter主题同步"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Add a native shell only for a product capability or experience that justifies its lifecycle cost. A minimal native host around Flutter remains a valid alternative.",
  "related": [
    "native-shell-state-synchronization"
  ]
}
---

# Synchronize native material settings as effective state

## When to use

A Flutter setting claims a native HDS material level is applied before the shell accepts it.

## Recommended approach

Validate one typed setting map and replay desired material state after native readiness, preserving failed requests for retry.

## Implementation steps

For optional native HDS material levels, define one typed value map across Dart and ArkTS, persist the desired choice, and replay it after native shell readiness. Validate allowed values rather than passing arbitrary integers to AppStorage. Gate the native feature by the selected SDK/product and expose unsupported distinctly; a no-op success on an older API must not make Flutter display the request as applied. Keep the latest desired setting when a native send fails so it can be retried.

Test changes before shell creation, unsupported devices, recreation and an invalid persisted value. Material intensity and theme/navigation ownership are separate settings even if they share a channel.

## Conditions and alternatives

Add a native shell only for a product capability or experience that justifies its lifecycle cost. A minimal native host around Flutter remains a valid alternative.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Change material before shell readiness, recreate the shell, reject invalid stored values and simulate unsupported SDK/send failure. Verify desired and applied states stay distinct.

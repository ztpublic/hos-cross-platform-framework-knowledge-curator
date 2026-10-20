---
{
  "schema_version": 1,
  "id": "qt-tray-effective-capability",
  "title": "Verify each Qt tray operation instead of trusting availability flags",
  "topic_key": "integration.qt-tray-effective-capability",
  "area": "integration",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Trace the selected QPA's implemented status-bar operations and limit UI promises to observed capabilities.",
  "when_to_use": "A Qt tray availability check succeeds but geometry, menu state or notification callbacks do not behave as expected.",
  "keywords": [
    "QSystemTrayIcon",
    "StatusBarExtensionKit",
    "geometry",
    "messageClicked",
    "托盘"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Imported behavior concerns a particular Qt 5 OH status-bar implementation; check API gates and actual device behavior for the chosen version.",
  "related": []
}
---

# Verify each Qt tray operation instead of trusting availability flags

## When to use

A Qt tray availability check succeeds but geometry, menu state or notification callbacks do not behave as expected.

## Recommended approach

Trace the selected QPA's implemented status-bar operations and limit UI promises to observed capabilities.

## Implementation steps

1. Check required ArkTS status-bar/display/notification module registration for the same QPA template generation.
2. Verify add/remove/update and click delivery independently; some historical availability/geometry methods returned constants.
3. Design icons for the backend's monochrome-mask behavior where applicable.
4. Validate each menu feature and notification result rather than assuming desktop QMenu or messageClicked parity.

## Conditions and alternatives

Imported behavior concerns a particular Qt 5 OH status-bar implementation; check API gates and actual device behavior for the chosen version.

## Common mistakes

A true supportsMessages flag or constant geometry is not proof of notification authorization or real icon bounds.

## Check it works

Test show/hide, icon change, menu activation, disabled/checkable requirements, tooltip gate and notification interaction separately.

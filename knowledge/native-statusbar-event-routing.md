---
{
  "schema_version": 1,
  "id": "native-statusbar-event-routing",
  "title": "Route OH PC status-bar clicks without an extra popup",
  "topic_key": "window.native-statusbar-event-routing",
  "area": "window",
  "scope": "common",
  "frameworks": [],
  "summary": "Use the status-bar event path for restoring an existing window, while giving notify-only menu actions a resolvable Ability identity and an explicit menu code.",
  "when_to_use": "Clicking a PC status-bar icon opens an empty popup, or a notifyOnly right-click action reports no opening method instead of invoking the running app.",
  "applicability": "Huawei Desktop Extension Kit statusBarManager on products exposing SystemCapability.PCService.StatusBarManager. SDK/API and service authorization must be checked; this is not generic OpenHarmony tray availability.",
  "keywords": [
    "statusBarManager",
    "quickOperation",
    "statusBarIconClick",
    "rightMenuClick",
    "notifyOnly",
    "menuCode",
    "StatusBarMenuAction"
  ],
  "related": [
    "electron-ohos-statusbar-openurl",
    "qt-tray-effective-capability",
    "privileged-capability-gating"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Route OH PC status-bar clicks without an extra popup

## When to use

Clicking a PC status-bar icon opens an empty popup, or a notifyOnly right-click action reports no opening method instead of invoking the running app.

## Recommended approach

Use the status-bar event path for restoring an existing window, while giving notify-only menu actions a resolvable Ability identity and an explicit menu code.

## Implementation steps

1. Check the status-bar syscap without making an optional PC surface an install requirement for every device. Keep the main window usable when registration fails.
2. For the reviewed event route, provide the required quickOperation fields including positive height and use an empty quickOperation abilityName; subscribe to statusBarIconClick and restore/focus the existing window.
3. For right-menu actions, set a real moduleName/abilityName even with notifyOnly=true, plus a unique menuCode. Dispatch rightMenuClick by that code.
4. Tie registration, event handlers and any menu-refresh timer to the host lifetime. Roll back partial installation and release image resources; record a failed removal rather than assuming the item disappeared.

## Conditions and alternatives

The two Ability-name rules apply to different fields. The reviewed service uses API-21-compatible menu labels instead of the selected property introduced later. Source icon cleanup and partial-registration rollback are incomplete, so the whole service is not a drop-in lifecycle implementation.

## Common mistakes

Using the same empty Ability name for a right-menu action can prevent resolution. Launching a statusBarView extension for every icon click can create a visible intermediate window.

## Check it works

Test left/right clicks, repeat install/remove, missing capability, registration failure, foreground/background shutdown and a recreated main window. Confirm one live target receives each event.

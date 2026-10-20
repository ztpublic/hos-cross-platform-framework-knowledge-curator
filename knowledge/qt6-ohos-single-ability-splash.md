---
{
  "schema_version": 1,
  "id": "qt6-ohos-single-ability-splash",
  "title": "Show a Qt splash in the same Ability",
  "topic_key": "window.qt6-ohos-single-ability-splash",
  "area": "window",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Hide the system starting window and host the splash as a frameless ArkUI subwindow inside the same UIAbility, then reveal the Qt main window when the app reports GUI-ready.",
  "when_to_use": "A Qt desktop app on OH shows a tiny system logo, a blank large window or a window-switch animation during startup, or a separate QSplashScreen becomes a second Ability and is minimized behind the main window.",
  "keywords": [
    "splash screen",
    "startWindowType",
    "REQUIRED_HIDE",
    "subwindow",
    "UIAbility",
    "WindowStage",
    "XComponent",
    "GUI ready",
    "2in1",
    "Qt 6"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Qt 6 OH PC apps using the QPA UIAbility template. Window-manager error codes and subwindow behavior vary by API level.",
  "related": [
    "qt6-ohos-gl4es-context-state",
    "qt-ohos-secondary-ability-process",
    "qt6-ohos-subwindow-surfaces"
  ]
}
---

# Show a Qt splash in the same Ability

## When to use

A Qt desktop app on OH shows a tiny system logo, a blank large window or a window-switch animation during startup, or a separate `QSplashScreen` becomes a second Ability and is minimized behind the main window.

## Recommended approach

Hide the system starting window and host the splash as a frameless ArkUI subwindow inside the same UIAbility as the Qt main window. Keep the host and splash at identical geometry, hand the WindowStage to Qt only after the splash is ready, and reveal the main window when the app reports GUI-ready.

## Implementation steps

1. Set the ability's `startWindow` profile to `startWindowType: REQUIRED_HIDE` and use a transparent start-window icon; disable the desktop `QSplashScreen` path.
2. In `onWindowStageCreate`, save the original main-window geometry and choose a splash size that fits the display. Resize and move the host window to exactly the splash rectangle before showing it.
3. Load a transparent host page and show the main window. Create a `decorEnabled: false` subwindow with the same rectangle, set its content, show it, then set the background transparent. Set shadow radius and corner radius to zero so the splash PNG alpha is the only silhouette.
4. Delay handing the WindowStage to Qt/QPA until the splash and runtime are ready. Inject the create info into the already-loaded host page instead of calling `loadContent` twice.
5. Keep the Qt XComponent attached but translated off-screen while the splash covers the host; hiding it can destroy the native surface.
6. When the app writes a GUI-ready marker, restore the main window's pre-splash geometry and decoration (best-effort), translate the XComponent back, wait briefly, raise the main window, then destroy the splash subwindow.
7. Guard every window-manager call and keep decoration/shadow failures outside the splash creation path; otherwise the splash can be skipped entirely.

## Conditions and alternatives

The system starting window can be used when an application icon is sufficient. A separate ability can host a splash only if the platform maps it to the same task without an animation and the main window stays behind it; the single-Ability path avoids that risk. Exact window-manager error codes and subwindow behavior vary by API level.

## Common mistakes

- Creating a second Ability for the splash and having it minimized behind the main window.
- Showing the parent after creating the child, or setting the child's background before it is shown.
- Host and splash rectangles differing, so the transparent PNG exposes a second window edge.
- Hiding the XComponent during startup and destroying the native surface.
- Restoring decoration before the first `showWindow()` or letting that failure abort the splash.

## Check it works

Cold-start on a 2in1 device and capture frames: only the splash should be visible initially, followed by a full main window with no white flash or window-switch animation. Confirm the final main-window geometry matches the pre-splash geometry, decorations are restored, and startup failure still reveals a usable window.

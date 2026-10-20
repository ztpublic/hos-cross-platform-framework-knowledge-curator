---
{
  "schema_version": 1,
  "id": "qt-ohos-arkts-chrome-insets",
  "title": "Reserve ArkTS chrome space around a Qt content view",
  "topic_key": "window.qt-ohos-arkts-chrome-insets",
  "area": "window",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Measure ArkTS chrome in physical pixels, inset the whole Qt main window, send an explicit bottom limit for inline menus, and route only content-area input to Qt.",
  "when_to_use": "A hybrid OH PC app draws menus/toolbars in ArkTS over a Qt XComponent, but Qt content is occluded, docks are clipped, inline menus slide under the bottom bar, or tapping the hidden Qt menu bar crashes the app.",
  "keywords": [
    "ArkTS chrome",
    "top inset",
    "bottom inset",
    "QMainWindow",
    "MDI",
    "dock widget",
    "menu bottom limit",
    "XComponent",
    "hit test",
    "hybrid UI",
    "2in1"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH PC hybrid apps that overlay ArkTS menus/toolbars on a single Qt XComponent. Pure-Qt or multi-window QAAs use a different layout and input contract.",
  "related": [
    "qt-ohos-embedded-file-dialogs",
    "qt6-ohos-subwindow-input",
    "qt-ohos-secondary-ability-process"
  ]
}
---

# Reserve ArkTS chrome space around a Qt content view

## When to use

A hybrid OH PC app draws menus/toolbars in ArkTS over a Qt XComponent, but Qt content is occluded, docks are clipped, inline menus slide under the bottom bar, or tapping the hidden Qt menu bar crashes the app.

## Recommended approach

Measure each ArkTS chrome band in physical pixels, inset the whole Qt main window, and send an explicit bottom limit for inline menus. Route only content-area input to Qt; never forward taps from the chrome bands to the hidden Qt widgets.

## Implementation steps

1. Decide ownership: ArkTS owns the top/bottom chrome, Qt owns the content. Keep one source of truth for each band height.
2. Measure top and bottom chrome in vp, convert with the UI context's `vp2px` (not a global conversion), and send the top/bottom values in physical pixels to Qt.
3. Apply the insets to the whole main window, not only the central MDI area, so docks and the central widget all clear the overlay.
4. Send the measured bottom-toolbar top as an explicit global-Y menu limit; use the Qt content bottom only as a fallback.
5. Make the content XComponent transparent to hit-testing so unhandled input reaches it; do not forward touches from the chrome bands.
6. Never let a tap on the hidden Qt menu bar pass through: it can open a QMenu and create a new QPA window, which can crash a single-XComponent integration.
7. Retry the inset push after Qt's event loop is ready; the first call can race startup. Re-push on toolbar wrapping or height changes.

## Conditions and alternatives

This design assumes ArkTS chrome over a single Qt XComponent. A pure-Qt window does not need the insets; a multi-window QPA may route the hidden menu differently. Physical-pixel measurements depend on display density and window decor.

## Common mistakes

- Insetting only the MDI area and leaving docks under the overlay.
- Forwarding chrome-band touches to Qt and opening a hidden menu.
- Using a fixed toolbar height after groups wrap to multiple rows.
- Converting vp with a global function that uses the wrong display density.
- Pushing insets once before the Qt event loop is ready and never retrying.

## Check it works

Resize the window and toggle toolbar groups; confirm the Qt content and docks stay inside the visible area and inline menus do not slide under the bottom bar. Tap the ArkTS chrome and verify no Qt menu opens; tap the content and verify normal interaction. Confirm no crash from a hidden-menu popup.

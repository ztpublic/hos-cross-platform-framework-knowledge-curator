---
{
  "schema_version": 1,
  "id": "qt6-ohos-subwindow-input",
  "title": "Route touch, mouse and keyboard input to Qt subwindows",
  "topic_key": "input.qt6-ohos-subwindow-input",
  "area": "input",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Use the subwindow's ArkTS window filter for touch and axis events, the XComponent mouse callback for pointer input, and the XComponent key callback with an explicitly focused component for keys.",
  "when_to_use": "Qt menus and dialogs render on OH PC but clicks pass through to the main window, keyboard shortcuts do nothing, or wheel/trackpad events are ignored.",
  "keywords": [
    "touch",
    "mouse",
    "keyboard",
    "wheel",
    "XComponent",
    "DispatchMouseEvent",
    "window filter",
    "focusable",
    "click passes through",
    "input passthrough",
    "QMenu",
    "QDialog",
    "Qt 6"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Qt 6 OH PC apps with separate menu/dialog subwindows. Callback paths and focus behavior depend on the OH API level and input device.",
  "related": [
    "qt6-ohos-subwindow-surfaces",
    "qt6-ohos-highdpi-single-count",
    "qt-ohos-secondary-ability-process"
  ]
}
---

# Route touch, mouse and keyboard input to Qt subwindows

## When to use

Qt menus and dialogs render on OH PC but clicks pass through to the main window, keyboard shortcuts do nothing, or wheel/trackpad events are ignored.

## Recommended approach

Use the subwindow's ArkTS window filter for touch and axis events, the XComponent mouse callback for pointer input, and the XComponent key callback with an explicitly focused component for keys. Do not redirect popup touches back to the main window.

## Implementation steps

1. Make the subwindow focusable after its content loads; a non-focusable window never runs the ArkTS window-level event filter, so touch goes to the main window.
2. Route touch through the window filter to the platform's `onTouchEventFromArkUi`, then to Qt. Normalize using native geometry: physical point minus the native window origin, divided by the native window size.
3. Route mouse input through the XComponent `DispatchMouseEvent` path. Disable node-API mouse handlers on nodes with hit-test disabled; those callbacks never fire.
4. Register the XComponent key callback (`OH_NativeXComponent_RegisterKeyEventCallback`) and make the component focusable with default focus. The node-API key path does not receive focus when the ArkUI focus is on the XComponent.
5. Forward wheel/axis values from the window mouse filter (`MOUSE_ACTION_AXIS_*`) or the ArkTS axis callback. If the platform consumes axis events, provide touch-drag scrolling as the fallback.
6. Skip the embedded-popup touch redirection for `Qt::Popup` and `Qt::Dialog`; only touches arriving from the main window's XComponent need redirection.
7. If both the window filter and XComponent deliver the same touch, consume one copy in the filter.

## Conditions and alternatives

Injected wheel or right-click events may bypass some filters, so verify with real hardware when possible. The key/mouse callback paths and focus behavior depend on the OH API level. Touch-drag scrolling is a functional fallback, not a wheel replacement.

## Common mistakes

- Leaving the subwindow non-focusable and wondering why its filter never runs.
- Sending mouse or key input through node-API handlers on a hit-test-disabled node.
- Redirecting popup touches back to the main window after they were already addressed correctly.
- Normalizing coordinates with logical sizes or without subtracting the native origin.
- Relying on injected wheel events to prove the trackpad path.

## Check it works

Click a menu item and confirm the action fires, open a dialog and click a button, type a character and press a shortcut, and scroll a list with a wheel or touch drag. Repeat with a real mouse/trackpad if available, and check that the main window does not receive duplicate clicks.

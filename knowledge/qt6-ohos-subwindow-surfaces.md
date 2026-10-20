---
{
  "schema_version": 1,
  "id": "qt6-ohos-subwindow-surfaces",
  "title": "Render Qt menus and dialogs in their own OH subwindows",
  "topic_key": "rendering.qt6-ohos-subwindow-surfaces",
  "area": "rendering",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Give each Qt popup/dialog its own system subwindow surface using a dual XComponent page: NODE for creation and input, SURFACE as the render target captured by the platform plugin.",
  "when_to_use": "Qt menus or dialogs are clipped to the main window, hidden behind an opaque white subwindow, or open with no visible content on OH PC.",
  "keywords": [
    "QMenu",
    "QDialog",
    "popup",
    "subwindow",
    "XComponent",
    "SURFACE",
    "NODE",
    "OHNativeWindow",
    "QXComponentRegistry",
    "2in1",
    "Qt 6"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Qt 6 OH PC apps whose menus, popups or dialogs are separate system subwindows. The XComponent and surface-holder behavior is API/build dependent.",
  "related": [
    "qt6-ohos-subwindow-input",
    "qt6-ohos-highdpi-single-count",
    "qt-dialog-window-parenting"
  ]
}
---

# Render Qt menus and dialogs in their own OH subwindows

## When to use

Qt menus or dialogs are clipped to the main window, hidden behind an opaque white subwindow, or open with no visible content on OH PC.

## Recommended approach

Give every Qt popup/dialog its own system subwindow surface. In its ArkTS page, use two XComponents: a NODE component for creation and input, and a SURFACE component as the render target captured by the platform plugin.

## Implementation steps

1. Create the menu/dialog as a real OH subwindow rather than compositing it into the main window's buffer; an opaque system subwindow will cover composited content.
2. In the subwindow page, place a NODE XComponent for `onAppear`, input and lifecycle, and a SURFACE XComponent with a distinct `_surf`-style id as the transparent render target.
3. Register a surface callback for the SURFACE component in the platform's XComponent registry. Store the `OHNativeWindow` on surface create/change and clear it on destroy.
4. In the platform window, adopt the captured surface for the subwindow, with a short bounded retry; if the ArkUI surface holder is unavailable, fall back to the registry surface. Flush the backing store directly to that surface.
5. Do not attach the Qt node tree to the subwindow's NODE XComponent. The node tree becomes the hit-test target and the XComponent stops receiving touch events.
6. Make the subwindow focusable after content loads; a non-focusable subwindow lets input pass through. Use `focusOnShow=false` when the window should not steal focus.
7. Verify that menu items, shortcut columns and dialog controls are fully visible at 1:1.

## Conditions and alternatives

On some OH builds the C-API child XComponent never allocates a surface and the surface-holder API returns null, so the ArkTS SURFACE component is the reliable source. The exact ids and retry counts are integration details. A floating window follows the same pattern.

## Common mistakes

- Using a single SURFACE XComponent for creation; it may never fire `onAppear` while the subwindow is hidden.
- Attaching the node tree to the NODE XComponent and losing input.
- Relying on a C-API child surface that is never created.
- Leaving the subwindow non-focusable.
- Compositing the menu into the main surface and expecting it to appear above the system subwindow.

## Check it works

Open a menu and a dialog and confirm they render inside their own system subwindows at native resolution. Screenshot the full menu, including shortcut text, and click a dialog button. The subwindow surface size should match its platform geometry, with no white cover or clipping.

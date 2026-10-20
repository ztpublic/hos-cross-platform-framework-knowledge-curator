---
{
  "schema_version": 1,
  "id": "qt6-ohos-highdpi-single-count",
  "title": "Keep Qt High-DPI scaling single-counted on OH",
  "topic_key": "rendering.qt6-ohos-highdpi-single-count",
  "area": "rendering",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Let the platform screen's logical-DPI ratio own logical-to-native conversion; return 1.0 from the platform window DPR and keep platform geometry in native pixels.",
  "when_to_use": "A Qt app on OH PC renders at 2x or 4x, shows only the top-left quarter, clips menus, or positions popups incorrectly on a scaled display.",
  "keywords": [
    "High-DPI",
    "devicePixelRatio",
    "logicalDpi",
    "QHighDpiScaling",
    "native pixels",
    "backing store",
    "QPlatformWindow",
    "2in1",
    "Qt 6",
    "scaling"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Qt 6 on OH PC with QHighDpiScaling enabled. A Qt 5 integration or a build with scaling disabled may use a different composition rule.",
  "related": [
    "qt6-ohos-subwindow-surfaces",
    "qt6-ohos-subwindow-input",
    "qt-ohos-resource-dir"
  ]
}
---

# Keep Qt High-DPI scaling single-counted on OH

## When to use

A Qt app on OH PC renders at 2x or 4x, shows only the top-left quarter, clips menus, or positions popups incorrectly on a scaled display.

## Recommended approach

Let the platform screen's logical-DPI ratio own logical-to-native conversion. Return 1.0 from the platform window's `devicePixelRatio()` and keep platform geometry in native pixels; Qt composes the high-DPI factor separately.

## Implementation steps

1. Confirm high-DPI scaling is active and that `QOhosPlatformScreen::logicalDpi()/logicalBaseDpi()` equals the display density (for example 2). That ratio is the single logical-to-native factor.
2. Keep `QOhosPlatformWindow::devicePixelRatio()` at 1.0. Qt computes `QWindow::devicePixelRatio()` as the platform window value times the high-DPI factor; returning the screen DPR there counts the factor twice.
3. Keep `QPlatformWindow::setGeometry()` and `geometry()` in native pixels. Qt converts at the `QWindow` boundary; converting again in the plugin shifts or scales windows.
4. Compute offsets between windows on a shared surface from platform-window geometry directly (native minus native). Do not derive them from `framePosition()` multiplied by DPR; stub frame margins add another error.
5. Check that the raster backing-store image size equals the native surface buffer size. A larger image clipped to the buffer is the signature of double scaling.
6. Re-test menu, popup and dialog placement after a DPI change, not only the main window.

## Conditions and alternatives

This contract is for a Qt 6 OH QPA with `QHighDpiScaling` enabled. A Qt 5 integration may use different composition rules. If scaling is disabled, the platform may instead need to report the real DPR; verify the active mode before changing values.

## Common mistakes

- Returning `screen()->devicePixelRatio()` from the platform window.
- Multiplying geometry by DPR in both `setGeometry()` and the renderer.
- Using logical coordinates for a native shared-surface offset.
- Treating `frameMargins()` as real decorations when they are stubs.
- Fixing only the main window and leaving menus/popups at the wrong scale.

## Check it works

Run on a scaled PC display and compare the backing-store image, surface buffer and platform geometry for the main window and a menu. All should be native-sized and 1:1. Open a popup at a known cursor position and confirm it lands there without clipping.

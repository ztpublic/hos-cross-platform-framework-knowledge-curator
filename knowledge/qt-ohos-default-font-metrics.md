---
{
  "schema_version": 1,
  "id": "qt-ohos-default-font-metrics",
  "title": "Diagnose oversized default fonts in Qt OH menus",
  "topic_key": "ui.qt-ohos-default-font-metrics",
  "area": "ui",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Measure the QPA font metrics, logical DPI and scaling policy before overriding fonts; oversized menu and toolbar text can come from the font database, point-vs-pixel sizing or high-DPI policy.",
  "when_to_use": "Qt menus or toolbars on OH PC render noticeably larger than the desktop build, causing clipped menu items or distorted layout even when the application stylesheet has no large font size.",
  "keywords": [
    "QApplication font",
    "QFontInfo",
    "logicalDpi",
    "devicePixelRatio",
    "high-DPI",
    "font database",
    "menu bar",
    "point size",
    "pixel size",
    "Qt"
  ],
  "status": "unresolved",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Qt OH apps comparing widget metrics across desktop and PC. The font database and high-DPI policy are QPA/version specific; no general fix is established here.",
  "related": [
    "qt6-ohos-highdpi-single-count",
    "qt-ohos-arkts-chrome-insets",
    "text-weight-versus-scale-policy"
  ]
}
---

# Diagnose oversized default fonts in Qt OH menus

## When to use

Qt menus or toolbars on OH PC render noticeably larger than the desktop build, causing clipped menu items or distorted layout even when the application stylesheet has no large font size.

## Recommended approach

This is an unresolved metrics problem. Measure the QPA font and DPI contract before overriding fonts; treat point size, pixel size, logical DPI and any high-DPI scaling factor as separate inputs.

## Implementation steps

1. Log `QApplication::font()`, `QFontInfo` point size, pixel size and family, screen logical DPI, device pixel ratio, and any `QT_SCALE_FACTOR`/font-related environment override.
2. Build a minimal `QMenuBar`/`QToolBar` window with the default application font and compare its item heights and text size with the real app. If the minimal window is also oversized, the problem is in the QPA/default metrics rather than the app stylesheet.
3. Check whether the selected font family resolves on the device and whether fallback changes the metrics; compare a CJK family and a monospace family.
4. Check the high-DPI policy and whether the QPA already applies a scaling factor. Do not compensate by multiplying or dividing font sizes again without measuring the active factor.
5. Test setting an explicit font before versus after `QApplication` construction, and inspect `QFontInfo` after setting it to see the actual pixel size the toolkit will use.
6. Separate font metrics from style pixel metrics and stylesheet rules: menu bar height, item spacing and stylesheet font-size can each change layout independently.
7. If the minimal window reproduces the size, record the QPA/runtime, DPI and font data and keep the issue unresolved rather than hiding it with margins.

## Conditions and alternatives

There is no confirmed general remedy in the reviewed source. The app enables high-DPI attributes and sets a platform-specific font on some builds, but the OH default path remains unresolved. A per-platform font override may reduce the symptom while changing text metrics elsewhere.

## Common mistakes

- Hardcoding a font family/size and calling the problem fixed without measuring the resulting pixel size.
- Mixing point and pixel units in stylesheets and `QFont`.
- Compensating with layout margins or fixed heights instead of correcting the metrics.
- Assuming the application stylesheet is the only source of the size.
- Testing only the full app and not a minimal menu/toolbar control.

## Check it works

Compare minimal and real-app menu metrics against the desktop build at the same logical DPI. If the text size matches without contrived margins and the menu items are fully visible, the issue is resolved. Otherwise keep the observation unresolved with the measured font/DPI/QPA data.

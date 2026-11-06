---
{
  "schema_version": 1,
  "id": "qt-pyqt-ohos-ghost-tooltip-windows",
  "title": "Remove ghost tooltip and hidden-dialog subwindows on OH PyQt",
  "topic_key": "window.qt-pyqt-ohos-ghost-tooltip-windows",
  "area": "window",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "If the first click is lost or focus leaves the main window at startup, stop eager hidden dialogs and tooltips creating focusable subwindows.",
  "when_to_use": "The first click after start has no effect, or focus leaves the main window, and window tools show extra Qt subwindows with odd rectangles.",
  "applicability": "PyQt5 on the reviewed Qt 5 OH QPA, where Qt::ToolTip and Qt::Dialog windows are reported to become focusable ArkUI subwindows. The source reports a window-service check on one device. Disabling tooltips removes a usability feature.",
  "keywords": [
    "tooltip",
    "ghost window",
    "first click lost",
    "focus stolen",
    "QToolTip",
    "Qt::ToolTip",
    "QDialog",
    "subwindow",
    "garbage windowRect"
  ],
  "related": [
    "qt-pyqt-ohos-subwindow-adoption",
    "qt6-ohos-subwindow-input",
    "qt-dialog-window-parenting"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Remove ghost tooltip and hidden-dialog subwindows on OH PyQt

## When to use

The first click after start has no effect, or focus leaves the main window, and window tools show extra Qt subwindows with odd rectangles.

## Recommended approach

If the first click is lost or focus leaves the main window at startup, stop eager hidden dialogs and tooltips creating focusable subwindows.

## Implementation steps

1. Dump the window list at startup and find extra windows with focusable state or strange rectangles.
2. Create dialogs lazily rather than at startup, especially debug consoles that call setGeometry or winId early.
3. If tooltips cause the problem, disable both entry points on OH: QWidget.setToolTip and QToolTip.showText.
4. Offer the tooltip text another way, such as a status bar, for features that depend on it.
5. Guard this with a platform check, and retest it when the QPA changes.

## Conditions and alternatives

Look for a QPA fix first where you can build Qt. Monkeypatching in Python covers Python call sites, not tooltips set from C++.

## Common mistakes

Applying it unconditionally on all platforms. Removing tooltips while eager dialogs remain the actual cause.

## Check it works

After a cold start, click once on the main window and confirm it registers. Check the window list after hover and after opening and closing dialogs.

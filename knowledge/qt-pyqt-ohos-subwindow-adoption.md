---
{
  "schema_version": 1,
  "id": "qt-pyqt-ohos-subwindow-adoption",
  "title": "Give persistent PyQt subwindows an owner before native creation",
  "topic_key": "window.qt-pyqt-ohos-subwindow-adoption",
  "area": "window",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Assign the existing main window as owner and the intended dialog type before the utility window is first shown, preserving its geometry and excluding transient system-managed windows.",
  "when_to_use": "A parentless PyQt utility window opens as an independent OH main window or follows the wrong QPA window route.",
  "applicability": "PyQt5 with an OH QPA that routes owned Qt dialogs to SubWindow. This route is version-specific and must be checked under PC move, resize, maximize and tray activation.",
  "keywords": [
    "PyQt5",
    "setParent",
    "Qt.Dialog",
    "SubWindow",
    "QMessageBox",
    "QDialog.exec_",
    "QWidget.show",
    "window owner"
  ],
  "related": [
    "qt6-ohos-subwindow-surfaces",
    "qt-ohos-embedded-file-dialogs"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Give persistent PyQt subwindows an owner before native creation

## When to use

A parentless PyQt utility window opens as an independent OH main window or follows the wrong QPA window route.

## Recommended approach

Assign the existing main window as owner and the intended dialog type before the utility window is first shown, preserving its geometry and excluding transient system-managed windows.

## Implementation steps

1. Create the main window first, then assign ownership to persistent utility windows before show() or native handle creation.
2. Filter eligible parentless Qt.Window/Qt.Dialog instances; skip the main window, already-owned widgets, menus, popups and tooltips.
3. Save geometry before setParent(mainWindow), set the intended dialog window flag, and restore geometry in the coordinate space required by the selected QPA.
4. Prefer explicit constructors or a narrow factory. If legacy code needs interposition, cover both show() and QDialog.exec_() because modal dialogs may bypass the Python show override.
5. Keep QMessageBox outside a blanket adoption hook: reparenting and geometry changes during a tray action can collapse its platform window. Test modal message boxes independently.

## Conditions and alternatives

The reviewed adapter translates saved global coordinates relative to the main window. That is a source-specific workaround, not a guarantee for every QPA. Do not replace a working native picker or modal-box route solely to make all windows share one implementation.

## Common mistakes

Do not reparent after the native window has been created, blanket-adopt every top-level widget, or reuse pre-parenting coordinates without checking their space.

## Check it works

Open persistent utility windows and modal dialogs from menus and tray actions; verify ownership, visible content, geometry and close behavior. Repeat after moving/maximizing the main window and changing display scale.

---
{
  "schema_version": 1,
  "id": "electron-ohos-crash-window-not-shown",
  "title": "Report uncaught OH Electron errors without opening a new crash window",
  "topic_key": "lifecycle.electron-ohos-crash-window-not-shown",
  "area": "lifecycle",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Keep the existing window and show an unparented dialog.showMessageBox, because the reviewed runtime fork does not reliably show a newly created crash window or a parented message box.",
  "when_to_use": "After an uncaught exception the app shows a dead window that ignores input, and no crash dialog appears.",
  "applicability": "Electron OH ports with desktop-style crash windows, on the reviewed runtime fork. The behavior comes from source comments; retest on newer runtimes.",
  "keywords": [
    "uncaught exception",
    "crash window",
    "showMessageBox",
    "dead window",
    "BrowserWindow not shown",
    "[Unknown]",
    "error dialog"
  ],
  "related": [
    "electron-ohos-window-types",
    "electron-ohos-close-reload-forced-exit",
    "electron-ohos-platform-capability-gates"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Report uncaught OH Electron errors without opening a new crash window

## When to use

After an uncaught exception the app shows a dead window that ignores input, and no crash dialog appears.

## Recommended approach

Keep the existing window and show an unparented dialog.showMessageBox, because the reviewed runtime fork does not reliably show a newly created crash window or a parented message box.

## Implementation steps

1. On the OH branch, avoid the path that destroys the main window and opens a dedicated crash window.
2. Show dialog.showMessageBox with no parent window, offering actions such as Reload, Continue and Quit.
3. Keep the renderer error handler registered, so later errors are also reported.
4. Serialize non-Error objects field by field for logs, as they otherwise arrive as [Unknown].
5. Guard against dialogs stacking when errors repeat.

## Conditions and alternatives

Continue after an uncaught exception can leave corrupted state, so make Quit the safe default where data is at risk.

## Common mistakes

Testing only by logging, without checking that the dialog is visible. Using a parented dialog. Removing the handler after the first error.

## Check it works

Throw from main and renderer, reject a promise with a non-Error, trigger repeated errors, and use each dialog action.

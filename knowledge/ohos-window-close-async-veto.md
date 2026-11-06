---
{
  "schema_version": 1,
  "id": "ohos-window-close-async-veto",
  "title": "Confirm before closing an OH PC window with windowWillClose",
  "topic_key": "window.ohos-window-close-async-veto",
  "area": "window",
  "scope": "common",
  "frameworks": [],
  "summary": "To ask the user before closing, register windowWillClose, return Promise.resolve(true) to block the close, and later call terminateSelf yourself when the user confirms.",
  "when_to_use": "A confirm-before-close prompt never gets a chance to run on a 2in1 window, or the window closes before the asynchronous answer arrives.",
  "applicability": "Title-bar close on 2in1/PC windows. A plugin write-up reports windowWillClose from API 15 and states windowStageClose is synchronous. No plugin source was reviewed, so check API behavior on your target. Phone back handling is separate.",
  "keywords": [
    "windowWillClose",
    "windowStageClose",
    "close confirmation",
    "unsaved changes",
    "terminateSelf",
    "getLastWindow",
    "title bar close",
    "prevent close"
  ],
  "related": [
    "qt-close-event-origin",
    "window-mode-and-fullscreen-state",
    "foreground-recovery-and-save-lifetime"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run"
}
---

# Confirm before closing an OH PC window with windowWillClose

## When to use

A confirm-before-close prompt never gets a chance to run on a 2in1 window, or the window closes before the asynchronous answer arrives.

## Recommended approach

To ask the user before closing, register windowWillClose, return Promise.resolve(true) to block the close, and later call terminateSelf yourself when the user confirms.

## Implementation steps

1. Get the window with window.getLastWindow(context), or use your window stage's main window.
2. Register win.on('windowWillClose', cb) and keep the same callback reference, so off can remove it.
3. In the callback, notify your UI and return Promise.resolve(true), which the source says prevents the system close.
4. When the user confirms, call UIAbilityContext.terminateSelf; on cancel, do nothing further.
5. Unregister when the handler is removed, and let the default close work when no veto is wanted.

## Conditions and alternatives

Multi-window apps should decide per window whether closing one ends the ability. Guard against a repeat close click while the dialog is open. Other exit routes, such as system shutdown, are not covered by this report.

## Common mistakes

Blocking the close with no visible prompt, leaving a window that cannot be closed. Leaving a stale listener.

## Check it works

Click close with and without unsaved changes, cancel, confirm, click close twice, and test with the UI busy.

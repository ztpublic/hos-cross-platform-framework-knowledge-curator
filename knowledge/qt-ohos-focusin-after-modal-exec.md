---
{
  "schema_version": 1,
  "id": "qt-ohos-focusin-after-modal-exec",
  "title": "Restore Qt widget focus state after a modal dialog on OH",
  "topic_key": "input.qt-ohos-focusin-after-modal-exec",
  "area": "input",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "If a text cursor stays hidden after QDialog::exec closes and setFocus has no effect, a narrow workaround is to send QFocusEvent(FocusIn, ActiveWindowFocusReason) to the target widget.",
  "when_to_use": "After a modal dialog closes, the editor shows no cursor, setFocus and activateWindow have no effect, and locking and unlocking the device makes it work again.",
  "applicability": "Qt Widgets on OH with the reviewed Qt 6 development generation on PC. This addresses a state symptom rather than the activation cause, and fires only where you connect it.",
  "keywords": [
    "QDialog::exec",
    "focusInEvent",
    "cursor invisible",
    "setFocus",
    "activateWindow",
    "FocusIn",
    "ActiveWindowFocusReason",
    "modal dialog"
  ],
  "related": [
    "qt-dialog-window-parenting",
    "qt6-ohos-subwindow-input",
    "qt-close-event-origin"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Restore Qt widget focus state after a modal dialog on OH

## When to use

After a modal dialog closes, the editor shows no cursor, setFocus and activateWindow have no effect, and locking and unlocking the device makes it work again.

## Recommended approach

If a text cursor stays hidden after QDialog::exec closes and setFocus has no effect, a narrow workaround is to send QFocusEvent(FocusIn, ActiveWindowFocusReason) to the target widget.

## Implementation steps

1. First rule out dialog parenting and subwindow input routing, and test whether your Qt build still reproduces this.
2. Choose a trigger you control, such as the current-editor change signal or after exec returns, and connect once only.
3. Check that the target widget is non-null, then call QApplication::sendEvent(widget, &focusEvent) with QEvent::FocusIn and Qt::ActiveWindowFocusReason.
4. Guard it with the OH platform macro and record the Qt version so it can be removed.

## Conditions and alternatives

The source triggers this on editor change, so it does not repair focus when the same editor stays current after the dialog closes; a post-exec call covers that case. The synthetic event does not restore real window activation, so other activation-dependent behavior may stay wrong.

## Common mistakes

Assuming the window is truly active because the cursor returned. Connecting repeatedly. Sending to a widget that has been destroyed.

## Check it works

Open and close modal dialogs by keyboard and pointer, then type at once. Confirm the cursor, IME and shortcuts all work, with and without changing the current editor.

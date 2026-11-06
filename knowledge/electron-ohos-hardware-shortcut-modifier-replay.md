---
{
  "schema_version": 1,
  "id": "electron-ohos-hardware-shortcut-modifier-replay",
  "title": "Replay hardware shortcuts when the OH Electron runtime drops modifiers",
  "topic_key": "input.electron-ohos-hardware-shortcut-modifier-replay",
  "area": "input",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Track Control, Alt and Shift from their own key events in the main process, cancel the plain key that arrives while one is held, and replay it through webContents.sendInputEvent with explicit modifiers.",
  "when_to_use": "On a PC keyboard Ctrl+A inserts a letter or triggers a plain-key action, Ctrl+C/V/Z/S do nothing, and the page sees ctrlKey=false although the Ctrl key itself produced keydown and keyup.",
  "applicability": "Electron on OH with hardware keyboards, where the selected runtime forwards the modifier keys but not their flags on the following key. This describes one reviewed runtime generation, so confirm the symptom before installing the bridge; keep it behind process.platform === 'openharmony'.",
  "keywords": [
    "Ctrl+A",
    "keyboard shortcut",
    "modifier",
    "ctrlKey false",
    "before-input-event",
    "sendInputEvent",
    "hardware keyboard",
    "AppGallery review"
  ],
  "related": [
    "electron-ohos-platform-capability-gates",
    "qt-ohos-terminal-direct-key-input",
    "electron-ohos-window-types"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Replay hardware shortcuts when the OH Electron runtime drops modifiers

## When to use

On a PC keyboard Ctrl+A inserts a letter or triggers a plain-key action, Ctrl+C/V/Z/S do nothing, and the page sees ctrlKey=false although the Ctrl key itself produced keydown and keyup.

## Recommended approach

Track Control, Alt and Shift from their own key events in the main process, cancel the plain key that arrives while one is held, and replay it through webContents.sendInputEvent with explicit modifiers.

## Implementation steps

1. Log before-input-event for Ctrl+A. Install the bridge only when input.control is false on the letter while a separate Control keyDown was seen.
2. On app web-contents-created, attach before-input-event per webContents and keep per-contents state for Control, Alt, Shift and the replayed keys.
3. For a Control/Alt/Shift event, record state from input.type and let it through. Ignore keyDown events that already carry input.control, input.alt or input.meta so replayed events and a fixed runtime pass untouched.
4. For a plain keyDown while Control or Alt is held, map input.code to an Electron keyCode (KeyA to A, Digit1 to 1, ArrowUp to Up, and so on), call event.preventDefault(), remember the mapping by code and call contents.sendInputEvent({type: 'keyDown', keyCode, modifiers}).
5. On the matching keyUp, cancel it and replay keyUp with the same keyCode and modifiers, so a held key never sticks.
6. Reset modifier state and pending replays on browser-window-blur, because a modifier released in another window never reaches this one.

## Conditions and alternatives

This is a compatibility bridge for a runtime defect, not an input API contract. Unmapped codes pass through unchanged. Meta is not tracked in the reviewed bridge, and IME composition and key-repeat behavior were not reviewed. Prefer a runtime that delivers flags if one is available.

## Common mistakes

Replaying without the already-has-modifier guard loops or double-fires. Forgetting keyUp replay leaves stuck keys. Skipping the blur reset leaves Ctrl logically held after Alt+Tab, turning later typing into shortcuts. Registering a global hotkey instead captures keys from other apps.

## Check it works

With a physical keyboard, or injected device-level events if your tool can send Ctrl, test Ctrl+A/C/V/Z/Y/S and Ctrl+Shift+Z on the canvas and in a text field, then press Ctrl, switch app, release and return. Confirm no shortcut fires while another app is in front and that plain typing still works.

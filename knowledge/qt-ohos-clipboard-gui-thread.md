---
{
  "schema_version": 1,
  "id": "qt-ohos-clipboard-gui-thread",
  "title": "Read the OH Qt clipboard from the GUI thread",
  "topic_key": "input.qt-ohos-clipboard-gui-thread",
  "area": "input",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Perform the clipboard access and result dispatch on the Qt GUI thread; keep background parsing or polling separate from the QPA clipboard bridge.",
  "when_to_use": "A Qt or PyQt clipboard-import action hangs or never opens its result window when QClipboard is read inside a worker thread.",
  "applicability": "Qt 5/PyQt5 OH QPA integrations that marshal pasteboard access through the ArkTS bridge. PC tray and menu actions also need foreground and pasteboard authorization checks.",
  "keywords": [
    "QClipboard",
    "QThread",
    "GUI thread",
    "READ_PASTEBOARD",
    "tray",
    "clipboard import",
    "deadlock",
    "PyQt5"
  ],
  "related": [
    "qt-ohos-external-clipboard-paste",
    "media-save-and-clipboard-permissions"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Read the OH Qt clipboard from the GUI thread

## When to use

A Qt or PyQt clipboard-import action hangs or never opens its result window when QClipboard is read inside a worker thread.

## Recommended approach

Perform the clipboard access and result dispatch on the Qt GUI thread; keep background parsing or polling separate from the QPA clipboard bridge.

## Implementation steps

1. Identify the thread that executes the actual QClipboard call, including timer and tray-triggered helper paths.
2. Route the read to an object living on the Qt GUI thread, using a queued signal when the initiator is a worker. Read the clipboard and dispatch the UI result from that slot.
3. Connect completion handlers before starting any remaining worker. Pass a copied text or data payload to background parsing instead of a QClipboard or widget pointer.
4. For the selected QPA, verify whether READ_PASTEBOARD must be requested by the UIAbility. Manifest declaration and a bridge permission check do not themselves produce a user grant.
5. Remove worker-based automatic clipboard polling if it cannot meet the thread contract; retain an explicit user action and handle denial/empty content without blocking.

## Conditions and alternatives

This is a scoped remedy for an identified worker-thread access path. An unrelated external-paste failure can still be a permission, MIME, focus or QPA-version issue. The reviewed permission request is asynchronous; wait for the result before relying on access.

## Common mistakes

Do not use a blocking cross-thread callback while holding a GUI lock, start a thread before connecting its result, or infer external clipboard support from in-app copying alone.

## Check it works

Copy text from another app, trigger import through both the menu and tray, and verify one responsive GUI result. Test denial, an empty clipboard, repeated actions and background/foreground transitions.

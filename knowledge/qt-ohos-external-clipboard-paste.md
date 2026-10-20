---
{
  "schema_version": 1,
  "id": "qt-ohos-external-clipboard-paste",
  "title": "Diagnose external clipboard paste failures in Qt OH apps",
  "topic_key": "input.qt-ohos-external-clipboard-paste",
  "area": "input",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "When in-app copy works but external paste does nothing, isolate the QPA clipboard bridge, pasteboard permission, MIME formats and focus/selection before adding an application workaround.",
  "when_to_use": "Copying text inside a Qt OH app works, but text copied from another OH application cannot be pasted, even though the paste action is enabled and the widget is focused.",
  "keywords": [
    "QClipboard",
    "pasteboard",
    "mimeData",
    "external paste",
    "QPA",
    "Scintilla",
    "QPlainTextEdit",
    "clipboard permission",
    "Qt 5"
  ],
  "status": "unresolved",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Qt 5 OH apps whose paste path goes through the QPA clipboard. The bridge, permissions and MIME behavior are runtime/QPA-version specific; no remedy is established here.",
  "related": [
    "qt-drag-data-readiness",
    "qt6-ohos-subwindow-input"
  ]
}
---

# Diagnose external clipboard paste failures in Qt OH apps

## When to use

Copying text inside a Qt OH app works, but text copied from another OH application cannot be pasted, even though the paste action is enabled and the widget is focused.

## Recommended approach

This is an unresolved integration gap. Do not add an application-level clipboard workaround until the QPA bridge, pasteboard permission, MIME formats and focus/selection state have been measured separately.

## Implementation steps

1. Reproduce in a minimal widget (for example a `QPlainTextEdit` or `QLineEdit`) with no app event filters: copy inside the app, then paste text copied from another OH app.
2. Connect to `QClipboard::dataChanged` and log `mimeData()->formats()`, `text()`, `hasText()` and the clipboard mode. If an external copy never raises the signal, the bridge is not delivering data.
3. Check the selected QPA's clipboard implementation and whether reading the OH pasteboard requires a permission or a foreground/ability state. Confirm the app's declared permissions instead of assuming a prompt.
4. Verify the paste target: focus, editability, read-only state, selection mode and whether the toolkit's paste command is enabled.
5. Test plain text versus HTML/rich formats, and `QClipboard::Clipboard` versus `Selection` if the QPA implements both.
6. If the minimal widget fails, record the QPA/runtime versions and keep the observation unresolved; file it upstream rather than bypassing `QClipboard` with private OH APIs.
7. If the minimal widget works, compare the app's shortcuts, event filters, Scintilla bindings and clipboard cache with the minimal case.

## Conditions and alternatives

There is no confirmed general remedy in the reviewed source. The failure may be in the QPA bridge, the system pasteboard policy or the app's paste binding. An app-level custom pasteboard bridge would change semantics and should be a last resort.

## Common mistakes

- Assuming the problem is a missing permission without reading the clipboard formats or signals.
- Clearing or overwriting the clipboard during startup.
- Bypassing `QClipboard` with private system APIs before isolating the QPA layer.
- Testing only app-internal copy/paste, which can succeed through an in-process clipboard cache.
- Treating a single MIME type (plain text) as proof that rich paste works.

## Check it works

The issue is resolved only when a minimal Qt widget and the real app can both paste plain text and the supported rich formats copied from another application. Until then, record the QPA/runtime versions, observed formats/signals and the exact failure step so the gap can be reproduced.

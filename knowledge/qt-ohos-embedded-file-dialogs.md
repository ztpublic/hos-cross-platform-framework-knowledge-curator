---
{
  "schema_version": 1,
  "id": "qt-ohos-embedded-file-dialogs",
  "title": "Embed Qt file dialogs inside the authorized work folder",
  "topic_key": "window.qt-ohos-embedded-file-dialogs",
  "area": "window",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Interpose the file-dialog statics, use a non-native QFileDialog embedded in the main window, start in the authorized work folder, and route combo popups to ArkTS instead of creating new windows.",
  "when_to_use": "Qt file dialogs on OH open outside the granted folder, show an empty or denied directory, or create extra windows when their look-in or file-type popups open.",
  "keywords": [
    "QFileDialog",
    "DontUseNativeDialog",
    "work folder",
    "embedded dialog",
    "look-in combo",
    "file-type combo",
    "ArkTS picker",
    "Qt Widgets"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Qt Widgets apps on OH PC with a single main window and a single-XComponent QPA that cannot safely create extra popup windows. A valid work-folder grant is required.",
  "related": [
    "qt-ohos-work-folder-grant",
    "qt-native-file-dialog-contract",
    "qt-picker-suffix-filter-mapping"
  ]
}
---

# Embed Qt file dialogs inside the authorized work folder

## When to use

Qt file dialogs on OH open outside the granted folder, show an empty or denied directory, or create extra windows when their look-in or file-type popups open.

## Recommended approach

Interpose the file-dialog statics, construct a real `QFileDialog` with native dialogs disabled, embed it in the main window, and always start it in the authorized work folder. Swallow the internal combo popups and route them to the ArkTS picker instead of creating popup windows.

## Implementation steps

1. Interpose `QFileDialog::getOpenFileName`, `getOpenFileNames`, `getSaveFileName` and `getExistingDirectory`.
2. Construct a real `QFileDialog` with `DontUseNativeDialog`, set the required file mode and accept mode, and run it through the app's in-window modal dialog template.
3. Choose the initial directory: the authorized work folder wins; fall back to the caller-supplied directory only when no grant exists.
4. Intercept presses on the internal look-in and file-type combo boxes before they open a popup; show the ArkTS picker and apply the selection.
5. When no main window exists (startup), fall back to the event/picker path instead of creating a top-level dialog.
6. Preserve filters, multi-select, overwrite confirmation and directory-only modes; validate that the selection remains inside the grant.
7. Test open, save, save-as, directory selection and cancel.

## Conditions and alternatives

This assumes a Qt Widgets app with one main window and a single-XComponent QPA that cannot safely create extra popup windows. A native picker remains an alternative for one-off files outside the work folder. The embedded dialog needs a valid work-folder grant to be useful.

## Common mistakes

- Using the native dialog and then wondering why the initial folder is outside the grant.
- Letting the internal combo popups open their own windows.
- Starting in the app's internal working directory rather than the authorized root.
- Falling back to a top-level dialog during startup.
- Ignoring cancel or an empty selection and returning a stale path.

## Check it works

Open and save a file inside the work folder; confirm the dialog renders inside the main window and starts in the granted root. Open the look-in and file-type dropdowns and confirm they use the ArkTS picker and create no extra window. Cancel and verify the caller gets an empty result, not a stale path.

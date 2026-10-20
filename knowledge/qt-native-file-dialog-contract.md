---
{
  "schema_version": 1,
  "id": "qt-native-file-dialog-contract",
  "title": "Verify the selected QFileDialog backend before replacing it",
  "topic_key": "storage.qt-native-file-dialog-contract",
  "area": "storage",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Try the selected QPA's implemented dialog route and validate result access and visibility separately.",
  "when_to_use": "A migration disables QFileDialog or assumes a returned path is a completed user-visible export.",
  "keywords": [
    "QFileDialog",
    "save",
    "文件选择"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Historical Qt backend examples differ in public-document behavior. A functioning dialog is not proof of persistent URI grants or file-manager visibility.",
  "related": [
    "document-picker-uri-access",
    "file-export-contract"
  ]
}
---

# Verify the selected QFileDialog backend before replacing it

## When to use

A migration disables QFileDialog or assumes a returned path is a completed user-visible export.

## Recommended approach

Try the selected QPA's implemented dialog route and validate result access and visibility separately.

## Implementation steps

1. Confirm the Qt/QPA/template combination exposes the required open/save dialog behavior.
2. Preserve cancellation and validate whether the result is a usable authorized file path or requires adapter processing.
3. For save, complete the content write and confirm whether the destination is user-selected/public or app-private.
4. Introduce a narrow native adapter only for behavior the actual Qt backend does not provide.

## Conditions and alternatives

Historical Qt backend examples differ in public-document behavior. A functioning dialog is not proof of persistent URI grants or file-manager visibility.

## Common mistakes

Do not replace a working picker with hard-coded sandbox paths or present a private save as public export.

## Check it works

Test open/save/cancel, inaccessible destinations and reopening the saved file from the intended user workflow.

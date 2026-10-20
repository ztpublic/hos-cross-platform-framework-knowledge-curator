---
{
  "schema_version": 1,
  "id": "qt-linux-branch-selection",
  "title": "Keep OH out of unintended desktop Linux branches",
  "topic_key": "architecture.qt-linux-branch-selection",
  "area": "architecture",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Handle Q_OS_OHOS deliberately before broad Linux guards and exclude OH from incompatible qmake unix branches.",
  "when_to_use": "Qt compilation or runtime enters a Linux desktop branch that assumes desktop services.",
  "keywords": [
    "Q_OS_OHOS",
    "Q_OS_LINUX",
    "qmake",
    "unix"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to the selected Qt OH fork; check actual macros and QML platform reporting for that version.",
  "related": []
}
---

# Keep OH out of unintended desktop Linux branches

## When to use

Qt compilation or runtime enters a Linux desktop branch that assumes desktop services.

## Recommended approach

Handle Q_OS_OHOS deliberately before broad Linux guards and exclude OH from incompatible qmake unix branches.

## Implementation steps

1. Review Q_OS_LINUX/Q_OS_UNIX guards and the operations they select, because the OH Qt build may also define Linux.
2. Add an OH branch where behavior differs; keep genuinely portable POSIX logic shared.
3. For desktop-only qmake scopes, exclude ohos alongside other excluded targets.
4. Do not infer native backend availability from a PC layout or QML OS label.

## Conditions and alternatives

Applies to the selected Qt OH fork; check actual macros and QML platform reporting for that version.

## Common mistakes

Do not turn every Linux feature off or assume Qt.platform.os returns ohos.

## Check it works

Compile original and OH targets and exercise the affected platform branch.

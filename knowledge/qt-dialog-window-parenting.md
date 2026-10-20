---
{
  "schema_version": 1,
  "id": "qt-dialog-window-parenting",
  "title": "Establish native dialog ownership before creating its window",
  "topic_key": "window.qt-dialog-window-parenting",
  "area": "window",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Give dialogs a deliberate Qt/transient parent and, where supported, tag the native subwindow relationship before native creation.",
  "when_to_use": "A Qt dialog appears as another task or loses close/wheel input while a sibling modal dialog is active.",
  "keywords": [
    "QDialog",
    "transientParent",
    "blockedByModalWindow",
    "tagWindowOrWidgetAsSubWindowOf"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Qt parentage, native subwindow ownership and system-task identity are separate concepts. QPA behavior differs by version.",
  "related": []
}
---

# Establish native dialog ownership before creating its window

## When to use

A Qt dialog appears as another task or loses close/wheel input while a sibling modal dialog is active.

## Recommended approach

Give dialogs a deliberate Qt/transient parent and, where supported, tag the native subwindow relationship before native creation.

## Implementation steps

1. Select the intended owning top-level window and assign the dialog parent where feasible.
2. Use the selected OH tagging API before show or winId creates the native window.
3. If a QPA synthetic parent exists only natively, inspect whether the matching Qt transient-parent chain is missing.
4. Prefer an app-level ownership repair; scope any QPA setTransientParent fix to the diagnosed fallback and avoid self-parenting.

## Conditions and alternatives

Qt parentage, native subwindow ownership and system-task identity are separate concepts. QPA behavior differs by version.

## Common mistakes

Tagging after winId or show can be too late. Disabling modal blocking globally can introduce incorrect input routing.

## Check it works

Test nested dialogs, modal siblings, close, scroll, task-switch entries and owner destruction.

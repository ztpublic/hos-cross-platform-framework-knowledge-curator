---
{
  "schema_version": 1,
  "id": "qt-qml-dark-palette",
  "title": "Fix indistinguishable QML controls by inspecting palette roles",
  "topic_key": "ui.qt-qml-dark-palette",
  "area": "ui",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Inspect effective palette roles and apply a deliberate app palette only when the native mapping is unsuitable.",
  "when_to_use": "Qt QML controls render text but dark backgrounds merge into one black area.",
  "keywords": [
    "QPalette",
    "SystemPalette",
    "Base",
    "dark theme",
    "纯黑"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "The source observation concerns a particular OH QPA palette. Check the installed backend and accessibility/theme policy before overriding.",
  "related": []
}
---

# Fix indistinguishable QML controls by inspecting palette roles

## When to use

Qt QML controls render text but dark backgrounds merge into one black area.

## Recommended approach

Inspect effective palette roles and apply a deliberate app palette only when the native mapping is unsuitable.

## Implementation steps

1. Differentiate missing QML/resources from a palette problem by checking visible text and component geometry.
2. Inspect QPalette Base, Button, Window and text roles under light/dark modes.
3. Choose an explicit app palette with adequate contrast, or implement a supported follow-system update path.
4. Keep the fixed-theme tradeoff visible; do not claim a one-time override follows later OS theme changes.

## Conditions and alternatives

The source observation concerns a particular OH QPA palette. Check the installed backend and accessibility/theme policy before overriding.

## Common mistakes

Forcing dark colors unconditionally changes light-mode behavior and can hide system preferences.

## Check it works

Test light/dark changes, disabled controls, focus/hover and larger text on the target display.

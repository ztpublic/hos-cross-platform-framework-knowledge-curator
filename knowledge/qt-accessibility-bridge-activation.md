---
{
  "schema_version": 1,
  "id": "qt-accessibility-bridge-activation",
  "title": "Enable and verify the Qt native accessibility bridge",
  "topic_key": "accessibility.qt-accessibility-bridge-activation",
  "area": "accessibility",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Confirm the QPA contains its accessibility bridge, activate the supported launch option and query through the native accessibility consumer.",
  "when_to_use": "An ArkUI inspector sees only an XComponent and automation cannot find Qt controls.",
  "keywords": [
    "QAccessible",
    "enableA11ySupport",
    "uitest",
    "XComponent",
    "无障碍"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to Qt OH builds that implement this experimental launch option. A launch flag cannot enable bridge code omitted from the binary.",
  "related": []
}
---

# Enable and verify the Qt native accessibility bridge

## When to use

An ArkUI inspector sees only an XComponent and automation cannot find Qt controls.

## Recommended approach

Confirm the QPA contains its accessibility bridge, activate the supported launch option and query through the native accessibility consumer.

## Implementation steps

1. Inspect the selected build configuration for QT_NO_ACCESSIBILITY and the actual QPA bridge; binary strings alone are only a diagnostic clue.
2. For versions exposing it, pass the Boolean Want parameter io.qt.experimental.enableA11ySupport before the QPA initializes.
3. Restart the intended process and query its native accessibility tree with a supported consumer such as uitest dumpLayout.
4. Check roles, names, bounds, focus and supported actions for actual Qt controls; resolve current node identities after recreation.

## Conditions and alternatives

Applies to Qt OH builds that implement this experimental launch option. A launch flag cannot enable bridge code omitted from the binary.

## Common mistakes

An ArkUI inspector tree, a compiled symbol or a running system screen reader does not prove the application bridge publishes controls.

## Check it works

Compare native control trees before/after activation and exercise focus/actions after resize and window recreation. Record accessibility separately from build/install.

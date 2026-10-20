---
{
  "schema_version": 1,
  "id": "text-weight-versus-scale-policy",
  "topic_key": "window.text-weight-versus-scale-policy",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "title": "Keep text size, font weight and system bold text from applying twice",
  "area": "window",
  "summary": "Inspect the embedding mapping, preserve inherited accessibility by default and remove duplicate native/theme transformations of the same system preference.",
  "when_to_use": "System text preferences unexpectedly change weight or a custom theme duplicates settings already propagated through MediaQuery.",
  "keywords": [
    "MediaQuery",
    "boldText",
    "TextScaler",
    "FontWeight",
    "fontWeightScale",
    "accessibility",
    "字重",
    "字体缩放",
    "粗体",
    "无障碍"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Flutter OH with the selected embedding/plugin versions; PC behavior requires the stated acceptance checks.",
  "related": []
}
---

# Keep text size, font weight and system bold text from applying twice

## When to use

System text settings unexpectedly change font weight, or a Flutter theme applies the same preference through both a native channel and inherited MediaQuery data.

## Recommended approach

Trace text size, weight and bold-text accessibility as separate inputs and give each one owner. Prefer the pinned embedding's existing MediaQuery propagation when it already carries the system setting.

## Implementation steps

1. Log the effective `MediaQuery.textScaler`, `boldText` and theme font weight while changing one system setting at a time. Inspect the embedding version's mapping before assuming a numeric native configuration field has a particular meaning.
2. If the embedding already maps the system weight preference into `MediaQuery.boldText`, remove redundant native listeners and manual numeric-to-FontWeight remapping for that same preference.
3. Keep app text scale independent from app weight. For a follow-system mode, retain inherited accessibility data and avoid a second multiplication/remapping.
4. If the product explicitly supports an app-owned weight mode, define its interaction with system bold text. Scope any `MediaQuery.copyWith(boldText: false)` override to that intentional policy and affected subtree; preserve the default follow-system accessibility path.
5. Rebuild theme consumers when relevant inherited settings change, and remove obsolete native event handlers/methods together so startup does not retain an inconsistent second source.

## Conditions and alternatives

The observed OH embedding mapping is version-specific. A numeric `fontWeightScale` is not necessarily Flutter's `textScaler` or a nine-weight font selector. If the embedding does not supply a required setting, use a small native adapter with an explicitly defined semantic contract instead of a guessed range table.

## Common mistakes

A normal theme weight does not neutralize inherited bold text. Conversely, globally clearing boldText can suppress a deliberate system accessibility preference. Do not use a sizing fix to silently change weight policy.

## Check it works

Test system text size and bold/weight changes independently, follow-system and explicit app modes, restart, theme changes and native-view recreation. Confirm accessibility settings remain effective in the default mode and text remains readable with larger sizes.

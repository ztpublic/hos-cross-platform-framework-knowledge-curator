---
{
  "schema_version": 1,
  "id": "native-custom-text-app-font-scale-cap",
  "title": "Apply the app font-scale cap in custom-drawn text",
  "topic_key": "text.native-custom-text-app-font-scale-cap",
  "area": "text",
  "scope": "common",
  "frameworks": [],
  "summary": "Text a native renderer scales itself must use the smaller of Configuration.fontSizeScale and UIContext.getMaxFontScale, since the configuration value is the raw system setting without the app cap.",
  "when_to_use": "With a large system font size, ArkUI text in the app stops growing at the declared limit, but text drawn by a native paragraph or custom renderer keeps growing and breaks layout.",
  "applicability": "Apps whose renderer multiplies a scale in native code instead of letting ArkUI scale text, with a cap from app.json5 configuration fontSizeMaxScale or UIContext.setMaxFontScale. getMaxFontScale is API 13+. Check at PC window sizes, where users commonly raise display and font settings.",
  "keywords": [
    "fontSizeScale",
    "fontSizeMaxScale",
    "setMaxFontScale",
    "getMaxFontScale",
    "font scale",
    "Configuration",
    "custom text renderer"
  ],
  "related": [
    "text-weight-versus-scale-policy",
    "arkui-density-and-inset-layout",
    "native-arkui-node-attribute-api-guard"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Apply the app font-scale cap in custom-drawn text

## When to use

With a large system font size, ArkUI text in the app stops growing at the declared limit, but text drawn by a native paragraph or custom renderer keeps growing and breaks layout.

## Recommended approach

Text a native renderer scales itself must use the smaller of Configuration.fontSizeScale and UIContext.getMaxFontScale, since the configuration value is the raw system setting without the app cap.

## Implementation steps

1. Identify which text ArkUI scales and which your renderer scales. Only the second needs this.
2. At initialization compute min(config.fontSizeScale, uiContext.getMaxFontScale()) and pass that to the native side.
3. Apply the same calculation in the onConfigurationUpdated path so changing the setting at runtime keeps the cap.
4. Guard getMaxFontScale with try/catch, or a null UIContext check, and fall back to no extra cap on older APIs.
5. Keep an existing app-level follow-system switch separate; when it is off the scale stays 1.

## Conditions and alternatives

The reviewed comment notes getMaxFontScale reports a default of 2.0 when no cap is configured, with a range of 1 to 3, so this also changes behavior for apps that declared nothing. Verify on your target API.

## Common mistakes

Fixing only the initial value so runtime changes break the cap. Applying the cap on the native side as well, which is harmless for min but duplicates the source of truth.

## Check it works

Set the system font scale above and below the app cap, both before launch and while running, and compare native-drawn text with an ArkUI Text at the same size.

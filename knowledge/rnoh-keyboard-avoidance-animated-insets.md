---
{
  "schema_version": 1,
  "id": "rnoh-keyboard-avoidance-animated-insets",
  "title": "Follow the OH keyboard animation in RNOH without double avoidance",
  "topic_key": "window.rnoh-keyboard-avoidance-animated-insets",
  "area": "window",
  "scope": "framework-specific",
  "frameworks": [
    "react-native"
  ],
  "summary": "Drive insets from window keyboard events with their animation config, and skip app-side translation when the system keyboard avoid mode already moves the content.",
  "when_to_use": "A focused input is hidden behind the soft keyboard, the page lifts twice, or it jumps rather than following the keyboard.",
  "applicability": "RNOH Fabric components using the RN window. keyboardDidShow/Hide need API 18+ and keyboardWillShow/Hide API 20+, gated by SystemCapability.Window.SessionManager. PC boundary: a physical keyboard normally produces no soft-keyboard height, so test with both; PC behavior was not reviewed.",
  "keywords": [
    "keyboardHeightChange",
    "avoidAreaChange",
    "keyboardWillShow",
    "KeyboardInfo",
    "getKeyboardAvoidMode",
    "KeyboardAvoidMode",
    "createAnimator",
    "setTranslate",
    "keyboard insets"
  ],
  "related": [
    "arkui-density-and-inset-layout",
    "rnoh-native-host-and-codegen",
    "rnoh-window-keep-screen-on"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Follow the OH keyboard animation in RNOH without double avoidance

## When to use

A focused input is hidden behind the soft keyboard, the page lifts twice, or it jumps rather than following the keyboard.

## Recommended approach

Drive insets from window keyboard events with their animation config, and skip app-side translation when the system keyboard avoid mode already moves the content.

## Implementation steps

1. On the window from rnInstance.getRNWindow(), listen to keyboardHeightChange and avoidAreaChange, and add the did/will events only when the API check and canIUse pass. Unsubscribe every registered listener.
2. When KeyboardInfo.config provides a curve, drive uiContext.createAnimator with it, and use a generation counter so a newer event cancels a stale animation.
3. Send values to C++ in vp with postMessageToCpp, and include whether getKeyboardAvoidMode() is something other than KeyboardAvoidMode.NONE.
4. In the component, reset the translate to 0 if the system already avoids the keyboard or the height is 0.
5. Otherwise find the focused descendant with isFocused, subtract scroll offset, and apply CustomNode.setTranslate(0, -overlap) with overlap not below 0.

## Conditions and alternatives

If the system avoid mode gives acceptable behavior, prefer it and remove app-side translation. Older API levels fall back to height events without timing data.

## Common mistakes

Stacking app translation over system avoidance. Mixing px and vp. Reacting to stale animations after quick focus changes.

## Check it works

Focus inputs at various positions with soft and physical keyboards, across avoid modes and API levels, and rotate or resize while the keyboard is up. Confirm a single lift and no residual offset after dismissal.

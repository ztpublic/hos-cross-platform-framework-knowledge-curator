---
{
  "schema_version": 1,
  "id": "rnoh-window-keep-screen-on",
  "title": "Apply RNOH keep-awake to the owning RN window",
  "topic_key": "window.rnoh-window-keep-screen-on",
  "area": "window",
  "scope": "framework-specific",
  "frameworks": [
    "react-native"
  ],
  "summary": "Resolve rnInstance.getRNWindow() from a UITurboModule and call setWindowKeepScreenOn on that window, with explicit activation and deactivation.",
  "when_to_use": "A React Native keep-awake module is missing on OH or affects the wrong window in a PC app.",
  "applicability": "RNOH UITurboModule implementations with getRNWindow support. Keep-screen-on is a window policy; it does not grant background execution or system-wide wake locks.",
  "keywords": [
    "KeepAwake",
    "UITurboModule",
    "getRNWindow",
    "setWindowKeepScreenOn",
    "screen timeout"
  ],
  "related": [
    "rnoh-ohos-package-adaptation",
    "continuous-background-task-contract"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Apply RNOH keep-awake to the owning RN window

## When to use

A React Native keep-awake module is missing on OH or affects the wrong window in a PC app.

## Recommended approach

Resolve rnInstance.getRNWindow() from a UITurboModule and call setWindowKeepScreenOn on that window, with explicit activation and deactivation.

## Implementation steps

1. Keep the JS TurboModule name, ArkTS package, C++ package and autolinking configuration aligned.
2. Resolve `ctx.rnInstance.getRNWindow()` and await the resulting `setWindowKeepScreenOn(true)` operation; catch/log failure if the public JS API is void.
3. Set false when the feature no longer needs the screen. At app integration level reconcile multiple request owners and window replacement rather than letting one consumer clear another's request.

## Conditions and alternatives

The reviewed module logs async errors and provides activate/deactivate; it does not implement reference counting or teardown restoration. Add those policies where the application needs them.

## Common mistakes

Using an arbitrary last window can change the wrong PC window. A void JS call is not evidence that the window accepted the request.

## Check it works

Test two RN windows, rapid activation changes, navigation away and stage recreation. Check idle behavior while visible and confirm ordinary timeout returns afterward.

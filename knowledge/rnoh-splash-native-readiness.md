---
{
  "schema_version": 1,
  "id": "rnoh-splash-native-readiness",
  "title": "Start the RN surface after the native splash is ready",
  "topic_key": "host.rnoh-splash-native-readiness",
  "area": "host",
  "scope": "framework-specific",
  "frameworks": [
    "react-native"
  ],
  "summary": "Configure the native splash from RNAbility.onWindowStageCreate, wait for its show Promise, then create the React Native surface and hide the splash when application initialization completes.",
  "when_to_use": "An RNOH splash never shows, JS show lacks WindowStage configuration, or a startup loop prevents the RN page from loading.",
  "applicability": "RNOH hosts using a native subwindow splash adapter with the matching package generation. PC overlays need separate resize/focus checks.",
  "keywords": [
    "RNAbility",
    "SplashScreen.show",
    "onWindowStageCreate",
    "TurboModule",
    "startup",
    "WindowStage"
  ],
  "related": [
    "native-subwindow-async-transaction",
    "rnoh-native-host-and-codegen"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Start the RN surface after the native splash is ready

## When to use

An RNOH splash never shows, JS show lacks WindowStage configuration, or a startup loop prevents the RN page from loading.

## Recommended approach

Configure the native splash from RNAbility.onWindowStageCreate, wait for its show Promise, then create the React Native surface and hide the splash when application initialization completes.

## Implementation steps

1. Register the splash page route and native package for the selected RNOH version.
2. In `onWindowStageCreate`, supply the Ability context, WindowStage, image Resource, background and route to native show; call the base RNAbility stage creation after that Promise resolves.
3. Define a show-failure policy that either continues RN startup with diagnostics or reports a startup failure; do not silently leave the RN surface uncreated.
4. Hide from JS after app readiness and route module destruction through the native serialized cleanup. Initial JS show alone cannot invent native stage configuration.

## Conditions and alternatives

The reviewed API exposes void JS show/hide and an awaitable native show. A JS call returning does not prove native teardown finished.

## Common mistakes

Busy-waiting in stage creation blocks asynchronous content loading; invoking the base creation before a racy splash can hide against an empty window handle.

## Check it works

Cold-start offline, hide immediately after mount, recreate the RN instance and fail image loading. Confirm the RN page starts or an intentional error is surfaced.

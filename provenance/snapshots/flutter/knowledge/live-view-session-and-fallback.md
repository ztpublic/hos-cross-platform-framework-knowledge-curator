---
{
  "schema_version": 2,
  "id": "live-view-session-and-fallback",
  "title": "Add an optional HarmonyOS Live View without losing the fallback",
  "topic_key": "integration.live-view-session-and-fallback",
  "area": "integration",
  "keywords": ["LiveViewKit", "liveViewManager", "isLiveViewEnabled", "sequence", "startLiveView", "updateLiveView", "stopLiveView", "实况窗", "胶囊", "权益", "实况窗降级", "repeated update", "countdown"],
  "summary": "Keep the business operation and fallback independent of Live View; serialize native lifecycle requests, preserve sequence state and distinguish support, enablement and accepted results.",
  "when_to_use": "A course, delivery or task countdown needs a system Live View, but devices, user settings or service admission can prevent it from being shown.",
  "status": "scoped"
}
---

# Add an optional HarmonyOS Live View without losing the fallback

## When to use

A course, delivery or task countdown needs a system Live View, but devices, user settings or service admission can prevent it from being shown.

## Recommended approach

Keep the business operation and fallback independent of Live View; serialize native lifecycle requests, preserve sequence state and distinguish support, enablement and accepted results.

## Implementation steps

1. Put `probe`, `start`, `update`, `stop` and active-state queries behind an application interface. Select a native implementation at startup; an unavailable implementation returns an explicit result. Keep ordinary notifications or a home-screen card connected to the same domain state so failure of this extra surface does not stop the feature.
2. Check the selected SDK and device capability before native calls. Treat `canIUse('SystemCapability.LiveView.LiveViewService')`, the user's `liveViewManager.isLiveViewEnabled()` setting, and any required application/service admission as separate conditions. A false or failed enablement query alone does not identify which condition failed.
3. Define a typed payload with a stable scene ID, current phase and explicit time units. Build a native `LiveView` using the selected SDK's template requirements, including appropriate title, layout, icon and click action. Validate timer ranges and text sizes against that SDK; optional-looking declarations and examples do not replace runtime validation.
4. Own request ordering in the native adapter. Serialize lifecycle operations and sequence allocation per scene, persist the increasing sequence before use, and return an error if persistence cannot establish the next valid value. Rate-limit across the scope required by the service, coalescing superseded updates rather than dropping the latest state permanently.
5. Reconcile the actual lifecycle: update an active scene, start an absent one, and stop when the domain operation ends. Use documented already-exists/not-exists results for a bounded recovery; do not treat every query error as absence or every failed start as permission to replace an existing scene. Interpret `LiveViewResult.resultCode` as well as thrown errors before returning success.
6. Derive countdown state from an absolute deadline and one captured clock value. Recompute at phase boundaries and on foreground recovery. Cancel owned timers when their UI/service owner is disposed. Native countdown display, Dart boundary timers and background delivery have separate lifetime guarantees.

## Conditions and alternatives

`@kit.LiveViewKit` is a HarmonyOS product service: generic OpenHarmony support does not establish access to it. Align the SDK/API and deployed device range, and verify service admission for the actual application identity. An application compiled with a newer kit still needs an appropriate implementation strategy for older supported devices.

If Live View is unavailable, keep the verified notification/card path. A [Form card](native-form-snapshot-bridge.md) has its own refresh contract; it is not automatically a continuously updated Live View. Do not disable the fallback merely because one native call completed.

## Common mistakes

Resetting a failed sequence read to a constant can reuse an old sequence. An async read/increment/write is not serialized by a throttle alone. Treating a resolved Promise as successful presentation can hide a non-success service result. Catching an exception and returning `degraded` does not itself publish the fallback.

## Check it works

Exercise supported and unavailable devices, a disabled user setting, denied service access, rejected template data and failed sequence persistence. Test concurrent updates, restart, already-active and already-ended scenes, rapid phase changes and final stop. Confirm the native result and visible surface separately, and verify the notification/card remains usable when Live View fails.

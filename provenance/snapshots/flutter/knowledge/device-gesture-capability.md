---
{
  "id": "device-gesture-capability",
  "title": "Use holding-hand or other device-specific gestures as optional input",
  "topic_key": "integration.device-gesture-capability",
  "area": "integration",
  "keywords": [
    "holding hand",
    "DETECT_GESTURE",
    "smart grip",
    "握持",
    "EventChannel",
    "device capability",
    "holding-hand detection",
    "握持检测",
    "传感器不可用"
  ],
  "summary": "Expose the gesture as an optional capability/event source and keep a usable deterministic UI when the capability or permission is unavailable.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "You want native device gestures to adjust the Flutter UI, such as moving controls according to how a large-screen device is held."
}
---

# Use holding-hand or other device-specific gestures as optional input

## When to use

You want native device gestures to adjust the Flutter UI, such as moving controls according to how a large-screen device is held.

## Recommended approach

Expose the gesture as an optional capability/event source and keep a usable deterministic UI when the capability or permission is unavailable.

## Implementation steps

1. Check device/API support for the native gesture before enabling the feature.
2. Request only the permission required for the chosen operation and preserve a denied/unavailable state.
3. Map native events into a small Dart model rather than exposing native objects to widgets.
4. Subscribe while the feature is active; remove listeners during teardown and reattach deliberately after lifecycle changes.
5. Debounce or stabilize layout changes as needed so noisy events do not move controls while the user is interacting.
6. Keep a manual/default layout choice and make the automatic behavior optional.

## Conditions and alternatives

Device-specific input can improve an experience but should not become a prerequisite for basic app operation.

## Common mistakes

A permission declaration is not capability detection or a granted subscription. Do not assume every OH device emits the same sensor/gesture events.

## Check it works

Test supported/unsupported devices, denial, absent/noisy events, repeated side changes and suspend/resume without duplicate listeners.

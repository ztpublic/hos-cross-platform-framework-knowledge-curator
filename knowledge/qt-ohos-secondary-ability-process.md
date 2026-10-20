---
{
  "schema_version": 1,
  "id": "qt-ohos-secondary-ability-process",
  "title": "Launch a second Qt window in its own OH process",
  "topic_key": "window.qt-ohos-secondary-ability-process",
  "area": "window",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Declare the secondary window as an exported UIAbility in an isolation-only feature module, then start it from Qt with an explicit want and physical-pixel window geometry.",
  "when_to_use": "A desktop-style Qt application needs an independent top-level window on OH PC, but embedding it in the first UIAbility shares one process, one window stage and one crash domain.",
  "keywords": [
    "secondary UIAbility",
    "isolationMode",
    "QOpenHarmonyWant",
    "QOpenHarmonyAbility",
    "start options",
    "multi-process",
    "feature module",
    "2in1",
    "window geometry",
    "Qt for OpenHarmony"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH PC/2in1 with Qt for OpenHarmony ability extras. Process isolation and start options depend on the selected product, API and QtOh version.",
  "related": [
    "qt-ohos-shared-entrypoint",
    "qt-application-thread-initialization",
    "qt-close-event-origin"
  ]
}
---

# Launch a second Qt window in its own OH process

## When to use

A desktop-style Qt application needs an independent top-level window on OH PC, but embedding it in the first UIAbility shares one process, one window stage and one crash domain.

## Recommended approach

Declare the secondary window as an exported UIAbility in a feature module configured for process isolation, then start it from Qt with an explicit want and window options. Treat the new ability as a separate application instance, not as another QWidget.

## Implementation steps

1. Create a feature module for the secondary window. Declare a UIAbility with `exported: true`, `launchType: specified`, the supported PC device types, and `isolationMode: isolationOnly` so it runs outside the caller's process.
2. From the Qt process, build a want with the bundle name, module name and ability name. Put launch arguments in `parameters`; serialize multiple values into one string because want parameters are not a shared C++ object graph.
3. Fill the start options with window width/height, animation, start-window background and position. Compute left/top from the available screen geometry multiplied by `devicePixelRatio()`; logical pixels place the window incorrectly on a scaled PC display.
4. Check and request runtime permissions from the caller with the QtOh permission helpers, and declare the same permissions in the target module. An isolated secondary ability may not be able to present the permission prompt itself, so complete the request before starting it.
5. Start the ability and install a result receiver; treat result code 0 as success and surface other codes with the ability/module name for diagnosis.
6. Let the secondary ability initialize its own Qt runtime and window. Do not pass QObject pointers, file descriptors or other process-local handles across the boundary.
7. Verify that the secondary process owns its window and can close independently of the main process.

## Conditions and alternatives

The feature module must be installed with the HAP, and isolation consumes another process. The `isolationMode` value and ability-start API come from the Qt for OpenHarmony integration and must match its version. If process isolation is not required, a second window inside the main UIAbility is cheaper but shares its lifecycle and crash domain.

## Common mistakes

- Omitting the module name from the want and relying on the ability name alone.
- Computing the window position in logical pixels instead of physical pixels.
- Relying on the target ability to show the camera, microphone or capture permission prompt.
- Expecting the secondary ability to see the caller's Qt objects or static state.
- Using a singleton launch type and then assuming a new window was created.

## Check it works

Launch the secondary window from the main app. Confirm it runs in a different process, appears at the expected physical position and size, receives its launch arguments, prompts for its declared permissions, and closes without terminating the main window. A failed start must return a non-zero code and a diagnostic message.

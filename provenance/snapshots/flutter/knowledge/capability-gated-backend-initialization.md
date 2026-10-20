---
{
  "id": "capability-gated-backend-initialization",
  "title": "Avoid initializing a native backend on an unsupported target",
  "topic_key": "architecture.capability-gated-backend-initialization",
  "area": "architecture",
  "keywords": [
    "capability",
    "lazy initialization",
    "功能开关",
    "MediaKit",
    "WebView",
    "mobile",
    "desktop",
    "crash on startup",
    "lazy plugin initialization",
    "unsupported backend",
    "启动崩溃",
    "notification", "reminder", "UnsupportedError", "通知初始化", "提醒不可用"
  ],
  "summary": "Select the supported capability before creating native-backed objects. Use lazy construction and explicit unavailable results for excluded backends.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "A controller crashes during construction or startup even though the UI later hides the unsupported feature."
}
---

# Avoid initializing a native backend on an unsupported target

## When to use

A controller crashes during construction or startup even though the UI later hides the unsupported feature.

## Recommended approach

Select the supported capability before creating native-backed objects. Use lazy construction and explicit unavailable results for excluded backends.

## Implementation steps

1. Define capabilities in terms of operations your app implements: play media, select a directory, manage a window, or query a sensor.
2. Resolve the backend before constructing its controller, session or singleton. Keep optional plugin objects lazy so importing or building unrelated UI does not initialize them.
3. Guard accessors as well as startup code. An accessor must not silently construct a backend that startup deliberately skipped.
4. Return a clear unsupported result or provide an alternate implementation; make the UI use the same capability decision.
5. Dispose resources when changing backends or losing the owning native context.
6. Keep operating system, device category, current window layout and native backend support as separate decisions. An OH PC may use desktop-style Dart UI while still requiring an OH HAR/MethodChannel backend; a Windows/Linux FFI route is usable only when its libraries also exist for the OH target. Query needed native device information before consumers read cached predicates, and define a conservative fallback for older or missing handlers.

For optional reminders, check both the selected plugin and the app's notification factory. A factory with only Android/iOS branches still returns an unsupported result on OH even if an OH notification package and native permission are configured. Implement a compatible OH factory branch when available; otherwise let startup proceed with an explicit unavailable reminder capability, and use that result in reminder settings, restore jobs and foreground callbacks. Catching a startup exception is only containment; it does not implement scheduling.

## Conditions and alternatives

An OH phone, tablet and desktop can offer different native capabilities. Capability support should reflect the implemented plugin/device route, not a blanket mobile/desktop label.

## Common mistakes

A disabled feature in one implementation is not a universal OS prohibition. Conversely, a true capability flag is not evidence that every method works.

## Check it works

Construct the app and relevant controllers without the optional plugin, then test supported, unavailable and teardown paths. Ensure hidden features never allocate their native objects.

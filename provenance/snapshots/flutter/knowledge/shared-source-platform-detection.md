---
{
  "id": "shared-source-platform-detection",
  "title": "Keep shared Dart code compilable with stock Flutter and Flutter OH",
  "topic_key": "compatibility.shared-source-platform-detection",
  "area": "compatibility",
  "keywords": [
    "Platform.isOhos",
    "TargetPlatform.ohos",
    "operatingSystem",
    "stock Flutter",
    "CI",
    "平台检测",
    "TargetPlatform.ohos undefined",
    "stock Flutter analyze",
    "conditional platform code",
    "双SDK"
  ],
  "summary": "Keep the shared compilation surface free of unavailable symbols and isolate SDK-specific dependencies behind a deliberate platform abstraction.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "Shared code fails stock Flutter analysis because it refers to an OH-only enum member, platform property or plugin type, even inside an OS check."
}
---

# Keep shared Dart code compilable with stock Flutter and Flutter OH

## When to use

Shared code fails stock Flutter analysis because it refers to an OH-only enum member, platform property or plugin type, even inside an OS check.

## Recommended approach

Keep the shared compilation surface free of unavailable symbols and isolate SDK-specific dependencies behind a deliberate platform abstraction.

## Implementation steps

1. Find symbols that exist only in the OH fork. A runtime `if` does not prevent the compiler from resolving them.
2. Where appropriate, expose an app-level platform predicate implemented with supported APIs such as the operating-system string. Use a web-compatible abstraction if the app also builds for web.
3. Keep backend initialization and plugin-specific types behind interfaces whose shared signatures compile on each intended SDK.
4. Make OH dependency selection an explicit build/developer step if separate graphs are necessary. Verify the active override configuration before resolution.
5. Analyze the shared entrypoints under each supported SDK rather than assuming one analyzer covers both.
6. Audit platform-enum switches in app code and vendored widgets. If the selected SDK adds an enum value, a statement switch written for older targets can leave a `late` field uninitialized. Assign a safe baseline before the switch, then override supported behavior explicitly; test the OH input/selection path as well as compilation.
7. Use `bool.fromEnvironment` only for a documented compiler/build define. An arbitrary `dart.library.ohos` name does not detect the running OS by itself. Verify who sets the define and its value in each build; otherwise use the runtime predicate consistently for native routing and backend initialization.

## Conditions and alternatives

This is useful only when supporting both SDK families is a real requirement. A project exclusively built with Flutter OH can use its supported platform APIs directly.

## Common mistakes

A custom OH-named override file is not automatically selected by pub. Switching a source dependency affects the whole graph, not only the branch of a runtime OS condition.

The conventional `pubspec_overrides.yaml` is read during dependency resolution for every target while present. A comment describing it as OH-only does not make it conditional. Use isolated checkouts or a reversible build step to select the intended graph before `pub get`, and do not delete a developer's preexisting override file without preserving it.

## Check it works

Run both analysis paths from reproducible dependency states; build web where applicable. Confirm the OH build still selects and initializes its native backend.

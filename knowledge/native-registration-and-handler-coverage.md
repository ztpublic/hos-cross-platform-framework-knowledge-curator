---
{
  "schema_version": 1,
  "id": "native-registration-and-handler-coverage",
  "topic_key": "plugins.native-registration-and-handler-coverage",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "title": "Diagnose a missing native plugin or method handler",
  "area": "plugins",
  "summary": "Trace the call through Dart backend selection and native registration on the same engine before changing the feature implementation.",
  "when_to_use": "A native-backed call throws `MissingPluginException`, never reaches ArkTS, or uses the wrong platform implementation.",
  "keywords": [
    "registration",
    "MethodChannel",
    "MissingPluginException",
    "BLE",
    "SPP",
    "插件注册",
    "handler",
    "no native handler",
    "BLE adapter not working",
    "插件未注册",
    "Pigeon",
    "messageChannelSuffix",
    "channel-error"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Flutter OH with the selected embedding/plugin versions; PC behavior requires the stated acceptance checks.",
  "related": []
}
---

# Diagnose a missing native plugin or method handler

## When to use

A native-backed call throws `MissingPluginException`, never reaches ArkTS, or uses the wrong platform implementation.

## Recommended approach

Trace the call through Dart backend selection and native registration on the same engine before changing the feature implementation.

## Implementation steps

1. Confirm which app-facing API and resolved backend the caller actually uses. Inspect platform guards and early returns before the channel call: a compiled native handler may never be invoked. An override for a similarly named package does not replace a different imported package.
2. Check backend selection: endorsed implementation metadata, explicit backend dependency, or a documented registration mechanism.
3. Confirm the native plugin is attached to the engine that owns the caller's messenger. Each engine needs the plugins it uses.
4. Match channel names, method names and codecs exactly, then confirm the requested method has a real handler.
5. Fix package metadata/host registration inputs and rebuild the native app. Avoid editing generated registrants as a permanent repair or registering the same package twice.
6. Return explicit unsupported results for unimplemented operations; do not substitute success-shaped stubs.

## Conditions and alternatives

App-owned native plugins can be registered directly. Federated packages may require both Dart backend selection and native handler registration.

For Pigeon, generating matching Dart and ArkTS bindings defines the protocol. It still requires a concrete host implementation and setup on the correct messenger. An FFI-only plugin takes a different route: trace its native build, packaging and symbol loading rather than expecting a MethodChannel handler.

When generated Pigeon APIs support `messageChannelSuffix`, use the same logical suffix for Dart callers, ArkTS host setup and native-to-Dart callbacks. Check the generator's separator convention instead of adding punctuation on only one side. A generated abstract API and codec can exist without any concrete `setup` invocation; regenerating those files cannot repair missing host registration. Regenerate both sides together and rebuild the native host after protocol changes.

## Common mistakes

A Dart BLE adapter, empty native host or package declaration is not a complete device integration. Hot reload cannot add newly compiled native handlers to the existing app.

A cached desired value is not the native state. For operations such as screen keep-on, await the native operation, report failures and update the cache only after success; reconcile state after the window or engine changes.

## Check it works

Call one implemented method after a full restart, test an unsupported method, and check engine teardown/recreation. For multiple engines, test each independently.

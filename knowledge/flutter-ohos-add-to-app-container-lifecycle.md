---
{
  "schema_version": 1,
  "id": "flutter-ohos-add-to-app-container-lifecycle",
  "title": "Forward lifecycle for an embedded Flutter OH container",
  "topic_key": "host.flutter-ohos-add-to-app-container-lifecycle",
  "area": "host",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "summary": "Register the native Ability and WindowStage with FlutterManager, initialize the shared engine and plugins, and forward the embedded FlutterBoostEntry container lifecycle and visibility.",
  "when_to_use": "Flutter embedded in an ArkUI tab stays blank, loses plugins, or keeps running as visible after a native tab switch.",
  "applicability": "Flutter OH embeddings exposing FlutterManager, FlutterPage and a matching OH FlutterBoost adapter. The source is a mobile-origin example; PC resizing, focus and multiple windows were not validated.",
  "keywords": [
    "add-to-app",
    "FlutterBoostEntry",
    "FlutterManager",
    "FlutterPage",
    "GeneratedPluginRegistrant",
    "onVisibleAreaChange",
    "tab visibility"
  ],
  "related": [
    "native-host-and-shared-dart",
    "flutter-engine-cache-coherence",
    "viewport-scale-and-layout-lifetime"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Forward lifecycle for an embedded Flutter OH container

## When to use

Flutter embedded in an ArkUI tab stays blank, loses plugins, or keeps running as visible after a native tab switch.

## Recommended approach

Register the native Ability and WindowStage with FlutterManager, initialize the shared engine and plugins, and forward the embedded FlutterBoostEntry container lifecycle and visibility.

## Implementation steps

1. Use a compatible Flutter module HAR, engine HAR and FlutterBoost implementation; register generated plugins on the initialized engine.
2. Pair `pushUIAbility`/`popUIAbility` with Ability creation/destruction, and `pushWindowStage`/`popWindowStage` with stage creation/destruction.
3. Construct one FlutterBoostEntry for each container route and arguments. Call its appear hook before obtaining FlutterView and pass the matching view ID to FlutterPage.
4. Forward page-show/hide and container-disappear hooks. For an embedded tab, use visibility notifications because an ArkUI child is not necessarily a routed page.
5. Guard deferred visibility callbacks against disposed containers and coordinate shared-engine ownership across concurrent containers.

## Conditions and alternatives

These APIs are embedding-version specific. The source shares an engine through FlutterBoost; do not replace a modern host template with its old API names without checking the selected SDK.

## Common mistakes

Only forwarding UIAbility foreground/background misses tab visibility. Reusing a view ID from a destroyed container can reconnect the wrong surface.

## Check it works

Switch tabs repeatedly, open native/Flutter routes, destroy a stage with a pending callback and resize a PC window. Verify plugin calls and engine/container counts.

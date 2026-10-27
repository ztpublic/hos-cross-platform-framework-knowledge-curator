---
{
  "schema_version": 1,
  "id": "qt-flutter-ohos-embedded-node-host",
  "title": "Host a Flutter OH page inside a Qt widget",
  "topic_key": "integration.qt-flutter-ohos-embedded-node-host",
  "area": "integration",
  "scope": "framework-specific",
  "frameworks": [
    "qt",
    "flutter"
  ],
  "summary": "Use a compatible mixed ArkUI-node bridge, one coherent Flutter HAR dependency, and per-page engine identity across Qt, ArkTS and Dart.",
  "when_to_use": "A Qt OH application needs an embedded Flutter page with independent routing, input focus and messages rather than a separate application window.",
  "applicability": "Qt for OpenHarmony QPA accepting the bridge's native Node wrapper and the matched Flutter OH embedding. PC focus, resize, multi-page isolation and destruction are unverified in this curation.",
  "keywords": [
    "FlutterEmbeddedQWidget",
    "QWindow::fromWinId",
    "createWindowContainer",
    "ComponentContent",
    "FlutterManager",
    "embedded.har",
    "engineId",
    "PACKAGE_FIND_FILE"
  ],
  "related": [
    "qt-qpa-runtime-template-coherence",
    "rnoh-native-host-and-codegen",
    "native-bridge-stale-handle-identity"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Host a Flutter OH page inside a Qt widget

## When to use

A Qt OH application needs an embedded Flutter page with independent routing, input focus and messages rather than a separate application window.

## Recommended approach

Use a compatible mixed ArkUI-node bridge, one coherent Flutter HAR dependency, and per-page engine identity across Qt, ArkTS and Dart.

## Implementation steps

1. Resolve one compatible Flutter HAR for the Qt host and bridge, including the root override when both depend on it. Match Qt QPA and bridge ABI; do not package arbitrary duplicate Qt libraries.
2. Include PACKAGE_FIND_FILE when provided by the OH cross-HAR build and link the bridge target into the Qt business library.
3. Push the owning UIAbility and WindowStage into FlutterManager. Register the native bridge, message transfer and mixed-component factory on the owning UI thread before requesting an embedded page.
4. Create the bridge's ArkUI ComponentContent containing FlutterPage. The reviewed Qt wrapper passes a bridge-specific Node through QWindow::fromWinId and QWidget::createWindowContainer; it is not an arbitrary desktop native handle.
5. Use one unique engine/page ID in the FlutterWindow, plugin registry, Dart entrypoint arguments and message handler. Route messages through queued Qt invocation and an owning-thread NAPI/TSFN path.
6. Forward focus/blur as window-stage activation events and page visibility to the FlutterWindow. Package the assets/AOT product required by the exact Flutter build mode.
7. Remove the widget and request bridge destruction before releasing its wrapper. Verify actual engine, window-map, message-handler, native-node and TSFN cleanup; visual disappearance alone does not establish release.

## Conditions and alternatives

The reviewed bridge provides source for mounting and communication, but its aboutToDisappear only logs. Runtime cleanup and late-message safety need independent validation. Its native Node wrapper is specific to the selected QPA; a matching SDK is a precondition, not a universal Qt embedding API.

## Common mistakes

Do not mix Qt runtimes from the host and HAR, route every page through one unscoped channel, assume onPageHide destroys an engine, or copy setup commands as evidence that the integration runs.

## Check it works

On the intended PC create two pages, exchange messages in both directions, resize and switch focus, destroy/recreate one page, and inspect engine/node/handler counts and late-message rejection. Verify correct debug/release resources separately.

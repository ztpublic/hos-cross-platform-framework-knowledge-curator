---
{
  "schema_version": 1,
  "id": "flutter-ohos-background-callback-engine",
  "title": "Run a Flutter OH plugin callback in a second engine by handle",
  "topic_key": "plugin.flutter-ohos-background-callback-engine",
  "area": "plugin",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "summary": "Persist the Dart callback handle, then create a separate FlutterEngine and execute it with DartCallback, so plugin events reach Dart independently of the UI isolate.",
  "when_to_use": "A plugin's Dart background callback, such as a download progress dispatcher, never runs on OH.",
  "applicability": "Flutter OH embedding generation reviewed in one plugin. Lifetime while the app is backgrounded is governed by the OS and was not reviewed. Engine APIs vary by embedding version.",
  "keywords": [
    "background isolate",
    "FlutterEngine",
    "executeDartCallback",
    "DartCallback",
    "lookupCallbackInformation",
    "callback handle",
    "findAppBundlePath",
    "MethodChannel"
  ],
  "related": [
    "flutter-ohos-add-to-app-container-lifecycle",
    "native-host-and-shared-dart",
    "continuous-background-task-contract"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Run a Flutter OH plugin callback in a second engine by handle

## When to use

A plugin's Dart background callback, such as a download progress dispatcher, never runs on OH.

## Recommended approach

Persist the Dart callback handle, then create a separate FlutterEngine and execute it with DartCallback, so plugin events reach Dart independently of the UI isolate.

## Implementation steps

1. Save the handle received from Dart with preferences.putSync.
2. Create new FlutterEngine(context, null, null, null) and initialize it.
3. Resolve FlutterCallbackInformation.lookupCallbackInformation(handle), and check for failure if the app bundle changed.
4. Run it through dartExecutor.executeDartCallback with a DartCallback using the resource manager and FlutterInjector.getInstance().getFlutterLoader().findAppBundlePath().
5. Open the background MethodChannel on that engine's messenger, wait for a ready signal before sending queued events, and destroy the engine when finished.

## Conditions and alternatives

The callback must be a top-level or static entry point marked for preservation in Dart. A second engine costs memory. If events are needed only in the foreground, the normal channel is enough.

## Common mistakes

Sending events before the Dart side is ready. Leaving the engine alive indefinitely. Assuming a handle survives an app update without checking.

## Check it works

Trigger events with the UI idle and with the app in the background, after a restart and after an update. Check delivery order and memory after repeated use.

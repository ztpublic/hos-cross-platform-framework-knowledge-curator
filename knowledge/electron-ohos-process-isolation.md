---
{
  "schema_version": 1,
  "id": "electron-ohos-process-isolation",
  "title": "Isolate Electron windows in OH processes",
  "topic_key": "lifecycle.electron-ohos-process-isolation",
  "area": "lifecycle",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Declare non-main abilities with a dedicated process name, use the engine child-process container for render processes, and enable multi-instance only when the product needs it.",
  "when_to_use": "An Electron app on OH needs independent browser or utility windows, a child window blocks or crashes the main UI, or multiple app instances must coexist.",
  "keywords": [
    "Electron",
    "multi-process",
    "process isolation",
    "ChildProcess",
    "UIAbility",
    "multiInstance",
    "BrowserAbility",
    "render process",
    "2in1"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH 2in1/tablet Electron HAPs with a thin entry module and an engine HAR. The source configures isolation but the sample application itself remains single-window/single-instance.",
  "related": [
    "electron-ohos-resfile-injection",
    "electron-ohos-non-pty-exec",
    "electron-ohos-platform-capability-gates"
  ]
}
---

# Isolate Electron windows in OH processes

## When to use

An Electron app on OH needs independent browser or utility windows, a child window blocks or crashes the main UI, or multiple app instances must coexist.

## Recommended approach

Declare separate abilities with a dedicated process name for non-main windows, use the engine's child-process container to start the render process, and enable multi-instance at the application level. Treat each process as a separate memory domain.

## Implementation steps

1. Split the HAP into a thin entry module and an engine HAR; keep window/ability implementations in the HAR so the entry only forwards lifecycle calls.
2. Declare the main entry ability and separate abilities for browser/stateless windows; give the non-main abilities a shared dedicated process name (`process: ':browser'`).
3. Extend the engine's child-process class for the module and start the render process through the engine's native context; this is the OH child-process model, not Node's `child_process.fork`.
4. Enable `multiAppMode` with an explicit instance count only when multiple app instances are a product requirement; do not infer multi-instance from multiple windows.
5. Do not share QObjects, JS heaps or process-local singletons between isolated windows; communicate through the engine's IPC or bridge.
6. Give each isolated process its own permissions and lifecycle expectations, and test window open/close, crash isolation and app exit.
7. Verify process names and PIDs on device and that closing a child window does not terminate the main process.

## Conditions and alternatives

Multiple abilities do not automatically mean multiple processes; the `process` field controls that. Multi-instance is a separate application-level setting and may be restricted by product policy. If the app does not need isolation, a single process is simpler.

## Common mistakes

- Declaring multiple abilities without a `process` field and assuming isolation.
- Sharing in-memory singletons across isolated windows.
- Confusing Node `child_process.fork` with the OH child-process container.
- Enabling multi-instance and expecting shared session state.
- Forgetting to forward lifecycle calls in the thin entry.

## Check it works

Open a secondary window and confirm it runs in the named process (different PID), close it without affecting the main window, and reopen it. Start a second app instance and confirm the configured instance limit. Trigger a failure in the child window and confirm the main window survives.

---
{
  "schema_version": 1,
  "id": "native-subwindow-async-transaction",
  "title": "Serialize asynchronous subwindow show and hide",
  "topic_key": "window.native-subwindow-async-transaction",
  "area": "window",
  "scope": "common",
  "frameworks": [],
  "summary": "Put window creation, content loading, showing and hiding on one failure-tolerant Promise chain; report show completion only after the native window is visible.",
  "when_to_use": "A hide request runs before an asynchronously created splash window exists, leaving an overlay visible or leaking its image resources.",
  "applicability": "OH WindowStage-owned subwindows, including PC startup overlays. Window ownership, resize and fullscreen behavior are product/API specific.",
  "keywords": [
    "createSubWindow",
    "setUIContent",
    "showWindow",
    "destroyWindow",
    "Promise",
    "splash",
    "PixelMap",
    "ImageSource"
  ],
  "related": [
    "webview-controller-and-lifetime",
    "async-bridge-shutdown-wait"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Serialize asynchronous subwindow show and hide

## When to use

A hide request runs before an asynchronously created splash window exists, leaving an overlay visible or leaking its image resources.

## Recommended approach

Put window creation, content loading, showing and hiding on one failure-tolerant Promise chain; report show completion only after the native window is visible.

## Implementation steps

1. Associate the operation queue with the owning WindowStage. Await `createSubWindow`, `setUIContent` and `showWindow` in order before resolving show.
2. Queue hide behind any pending show. Absorb a previous queue rejection into the queue tail while returning the current operation's error to its caller.
3. On any intermediate failure, destroy the partially created window and release owned ImageSource/PixelMap resources through their supported release APIs.
4. Make repeated show/hide idempotent, detach stale handles before release, and drain queued cleanup during owner teardown. Track asynchronous release completion if the shutdown contract requires it.

## Conditions and alternatives

A process-global static window is suitable only for an explicitly single-owner design. A replacement WindowStage needs a new generation so old work cannot publish into it.

## Common mistakes

Mixing callbacks with an outer Promise that returns after image decode makes completion mean the wrong thing. Clearing a field alone does not release native image memory.

## Check it works

Queue show-hide-show immediately, inject failure at every creation stage, and destroy the owner during decode. Verify no overlay remains and release callbacks complete.

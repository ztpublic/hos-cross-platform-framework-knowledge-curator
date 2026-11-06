---
{
  "schema_version": 1,
  "id": "kmp-kuikly-ohos-sync-gesture-context-scheduling",
  "title": "Run KuiklyUI context tasks on the main thread during sync gestures",
  "topic_key": "input.kmp-kuikly-ohos-sync-gesture-context-scheduling",
  "area": "input",
  "scope": "framework-specific",
  "frameworks": [
    "kotlin-multiplatform"
  ],
  "summary": "During a synchronous touchMove or scroll session on OH, pause the context thread and drain its queue on the main thread in 2 ms slices, so the sync event does not wait out a full frame task.",
  "when_to_use": "Dragging or scrolling a KuiklyUI page with sync events on OH janks because each main-thread sync callback blocks on the context thread lock while frame tasks run.",
  "applicability": "KuiklyUI on OH from the renderer generation with sync-event sessions, with sync touchMove or syncScroll registered. This is a scheduling change inside the renderer, so prefer upgrading to reimplementing it. Pointer dragging on PC exercises the same continuous sync path.",
  "keywords": [
    "sync touchMove",
    "syncScroll",
    "context thread",
    "main-thread freeze",
    "jank",
    "vsync",
    "input-first",
    "KRThread",
    "2ms"
  ],
  "related": [
    "kmp-kuikly-ohos-resize-sync",
    "kmp-kuikly-statusbar-scroll-ownership",
    "rendering-budget-and-progressive-ui"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Run KuiklyUI context tasks on the main thread during sync gestures

## When to use

Dragging or scrolling a KuiklyUI page with sync events on OH janks because each main-thread sync callback blocks on the context thread lock while frame tasks run.

## Recommended approach

During a synchronous touchMove or scroll session on OH, pause the context thread and drain its queue on the main thread in 2 ms slices, so the sync event does not wait out a full frame task.

## Implementation steps

1. Bound a session from ArkUI state: first pointer down to last pointer up or cancel for touch, and scroll start to scroll stop including inertia and animated offsets. Single-shot events such as keyboard height changes do not open one.
2. Reference-count sessions, make repeated starts for the same view and event idempotent, and release them when a view is removed, destroyed or an ancestor stops the event.
3. Have the context thread release its task lock after the current task when the main thread is waiting, returning the rest of the batch to the front of its queue.
4. While a session is active, run context tasks on the main thread for at most 2 ms per round, and postpone UI work posted while the context task lock is held.
5. Let frame tasks wait for the current input and merge at most one vsync. Keep the deferred frame per page and drop it when the page is destroyed.

## Conditions and alternatives

A long-running single task is not interrupted by the 2 ms budget. The reviewed spec also sets a nativeBuild gate choosing between vsync and a 12 ms timer for Compose, so a mismatched host and shared library version can change the path.

## Common mistakes

Ending the session on the wrong event, such as finger up during inertia. Keeping one global deferred frame across pages. Delivering a stored frame to a destroyed page.

## Check it works

Drag and fling long lists with heavy frame tasks while recording main-thread wait time and frame intervals. Interrupt the gesture, nest scrollers, and destroy the page in mid-gesture to confirm session counts return to zero.

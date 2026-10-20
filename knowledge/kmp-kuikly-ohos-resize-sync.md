---
{
  "schema_version": 1,
  "id": "kmp-kuikly-ohos-resize-sync",
  "title": "Synchronize KuiklyUI root-view resize events on OH",
  "topic_key": "window.kmp-kuikly-ohos-resize-sync",
  "area": "window",
  "scope": "framework-specific",
  "frameworks": [
    "kotlin-multiplatform"
  ],
  "summary": "When rotating, splitting or resizing the OH window leaves a white frame, make the rootViewSizeDidChanged event synchronous in the Kuikly view delegate instead of dispatching it asynchronously.",
  "when_to_use": "A KuiklyUI page flashes white or lays out one frame late after OH window rotation, split-screen resize or a host container size change, while normal navigation renders correctly.",
  "keywords": [
    "KuiklyUI",
    "rootViewSizeDidChanged",
    "syncSendEvent",
    "resize",
    "rotation",
    "split screen",
    "white frame",
    "IKuiklyViewDelegate"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "KuiklyUI OH hosts whose view delegate exposes syncSendEvent. The event name and delegate API are renderer-version specific.",
  "related": [
    "kmp-kuikly-ohos-host",
    "window-mode-and-fullscreen-state",
    "viewport-scale-and-layout-lifetime"
  ]
}
---

# Synchronize KuiklyUI root-view resize events on OH

## When to use

A KuiklyUI page flashes white or lays out one frame late after OH window rotation, split-screen resize or a host container size change, while normal navigation renders correctly.

## Recommended approach

When rotating, splitting or resizing the OH window leaves a white frame, make the `rootViewSizeDidChanged` event synchronous in the Kuikly view delegate instead of dispatching it asynchronously.

## Implementation steps

1. Reproduce with a window rotation, a split-screen resize and a host container size change; confirm the page recovers on the next frame and that only resize is affected.
2. In the view delegate, override `syncSendEvent(event)` and return true for `rootViewSizeDidChanged`.
3. Keep every other event asynchronous; making all native-to-Kotlin events synchronous can block the UI thread or change event ordering.
4. Rebuild the host and repeat rotation, split, maximize/restore and fullscreen transitions.
5. If the white frame persists, add lifecycle/first-frame logging to determine whether the resize event, the layout pass or the surface is late, rather than broadening the synchronous set.
6. Recheck after a renderer upgrade: the event name and the delegate hook may change.

## Conditions and alternatives

The source comment presents this as a targeted workaround for white-screen-on-resize, not a universal default. A host that controls the container can also debounce or pre-size the view before the resize. Very large synchronous work in the event handler can still stall the frame.

## Common mistakes

- Returning true for every event and serializing unrelated native-to-Kotlin calls.
- Using the wrong event string, so the override never matches.
- Testing only rotation and not split-screen/maximize, where the container size change differs.
- Treating the white frame as a rendering bug when the resize event is simply one frame late.
- Forgetting to recheck the workaround after upgrading the renderer.

## Check it works

Rotate, split, maximize/restore and resize the window repeatedly. The page should update without a white frame and remain interactive. Confirm other events still arrive in the expected order and the UI thread is not blocked during resize.

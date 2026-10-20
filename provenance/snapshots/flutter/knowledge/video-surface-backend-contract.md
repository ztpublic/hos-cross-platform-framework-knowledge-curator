---
{
  "id": "video-surface-backend-contract",
  "title": "Fix video black screens without bypassing the backend contract",
  "topic_key": "media.video-surface-backend-contract",
  "area": "media",
  "keywords": [
    "Texture",
    "video_player",
    "media_kit",
    "FVP",
    "MDK",
    "Erika",
    "OHNativeWindow",
    "黑屏",
    "video black screen",
    "audio without video",
    "Texture issue",
    "有声音没画面"
  ],
  "summary": "Keep the player widget, native surface and renderer lifecycle aligned; reduce layout/transforms only as a targeted diagnostic or workaround.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "Audio plays but no picture appears, or a video surface breaks when wrapped, resized or replaced with a raw Texture widget."
}
---

# Fix video black screens without bypassing the backend contract

## When to use

Audio plays but no picture appears, or a video surface breaks when wrapped, resized or replaced with a raw Texture widget.

## Recommended approach

Keep the player widget, native surface and renderer lifecycle aligned; reduce layout/transforms only as a targeted diagnostic or workaround.

## Implementation steps

1. Confirm the selected OH backend really implements the renderer and native-window/texture bridge your app needs.
2. Start with the plugin's supported video widget and a simple bounded layout. Separate source/decoder failures from surface-composition failures.
3. If frames disappear under transforms or fitting wrappers, remove those layers one at a time and identify the smallest failing composition.
4. Avoid replacing the plugin widget with a raw Texture unless you also preserve all required surface registration, sizing and lifecycle behavior.
5. On backend changes or view recreation, dispose old textures/controllers and attach events to the new instance.

## Conditions and alternatives

Different media backends have different OH support and integration requirements. A backend excluded by one dependency graph may be viable through another supported implementation.

### Await native teardown before replacing a backend

A fork may expose `onMediaStatus` as a stream while another version registers a callback, and its `dispose` may return a Future. Adapt the actual resolved API, retain/cancel the stream subscription, mark the owner disposed before late callbacks can reach notifiers, and await native teardown before creating a conflicting replacement. Coalesce synchronous disposal and explicit asynchronous disposal into one teardown Future. Preserve teardown errors for the caller; a fire-and-forget wrapper should report them rather than claim release completed.

Test repeated dispose calls, a pending status event during teardown, delayed/rejected native disposal and rapid backend swaps. Cancellation of a Dart subscription is distinct from completion of native resource release.

### Serialize surface reconstruction with source loading

If a renderer or HDR option is fixed at player creation, put source loading and surface reconstruction on the same queue. Recheck the desired versus applied configuration at execution time so rapid PiP or source changes do not act on stale requests. After a failed rebuild, keep an explicit retry state: the old player may already be gone, so a later request must be able to recreate it even when no current player exists.

Publish controller replacement to the UI. If a video or subtitle widget subscribes only in `initState`, changing its controller property is insufficient; use a generation key or a supported rebind path to replace those subscriptions. Announce controller allocation separately from completion of source opening. Restore position, pause state and speed without invoking an unrelated autoplay policy; reattach tracks and other player-bound work only after the new source is ready.

For dynamic-range output, also preserve the [HDR surface, signaling and display contract](hdr-output-surface-and-capability.md).

### Branch custom renderers by the active output type

A lightweight video widget copied from a texture-only backend must not require a texture ID when the selected controller outputs through a native platform view. Observe platform-controller readiness as well as geometry and texture changes, inspect the actual platform controller's output mode, and mount its supported native video widget when required. Wait for valid dimensions rather than treating an absent texture ID as a decoder failure. Use the backend's documented units when converting video geometry to logical layout size.

Include in-app overlays in the render-state policy. If their transforms, clipping or ownership cannot host the native view, request a supported alternate path through the same reconstruction queue, then restore the normal path on return. A renderer capable of constructing a platform view does not by itself prove that view can be moved into every overlay. Test controller replacement after the lightweight widget has mounted, plus rapid enter/exit and disposal during a queued rebuild.

## Common mistakes

A selected hardware-decoder label is not proof of actual hardware decoding. Initialization and audio output are not proof of rendered video.

For custom native conversion/player handles, also preserve the [event-loop teardown contract](native-event-loop-handle-teardown.md): removing callbacks and destroying a handle are separate operations.

## Check it works

Test first frame, multiple codecs, resizing/orientation, route disposal/recreation and backend switching. Use native decoder evidence for hardware-acceleration claims.

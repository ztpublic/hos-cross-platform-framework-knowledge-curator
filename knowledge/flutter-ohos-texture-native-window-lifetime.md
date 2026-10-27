---
{
  "schema_version": 1,
  "id": "flutter-ohos-texture-native-window-lifetime",
  "title": "Bind a Flutter OH texture to an owned native window",
  "topic_key": "media.flutter-ohos-texture-native-window-lifetime",
  "area": "media",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "summary": "Register the texture surface, attach its surface ID through an owned OHNativeWindow, and serialize resize, detach and release across Dart, ArkTS and native code.",
  "when_to_use": "A custom Flutter OH video texture remains black, breaks after resizing, or crashes when its route or engine is destroyed.",
  "applicability": "Flutter OH embeddings exposing TextureRegistry and SurfaceTextureEntry, plus a renderer supporting OHNativeWindow output. PC DPI, window resizing and texture composition require target validation.",
  "keywords": [
    "TextureRegistry",
    "SurfaceTextureEntry",
    "getSurfaceId",
    "OHNativeWindow",
    "CreateNativeWindowFromSurfaceId",
    "unregisterTexture",
    "devicePixelRatio",
    "generation"
  ],
  "related": [
    "video-surface-backend-contract",
    "flutter-player-surface-reconstruction",
    "flutter-player-async-teardown",
    "native-bridge-stale-handle-identity"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Bind a Flutter OH texture to an owned native window

## When to use

A custom Flutter OH video texture remains black, breaks after resizing, or crashes when its route or engine is destroyed.

## Recommended approach

Register the texture surface, attach its surface ID through an owned OHNativeWindow, and serialize resize, detach and release across Dart, ArkTS and native code.

## Implementation steps

1. Use LayoutBuilder and devicePixelRatio to calculate a valid physical buffer size from the logical view. Keep width/height units and the separate renderer scale explicit.
2. Obtain and register a texture ID, setTextureBufferSize, then retain its SurfaceTextureEntry and getSurfaceId result in the ArkTS adapter.
3. Create OHNativeWindow from the surface ID in native code. Attach it to the renderer and store ownership only after successful attachment; destroy the new window immediately if attachment fails.
4. On resize update both the embedding buffer size and the renderer surface metrics. Coalesce async work and invalidate late results with a view generation; release a texture whose creation completes after disposal.
5. On replacement detach the previous renderer before binding the new player. On release detach the renderer, destroy the owned native window, then unregister the embedding texture.
6. On engine teardown stop frame polling, release all texture bindings and native players, then clear channel/event handlers. Repeated release must not touch a replacement instance.

## Conditions and alternatives

Texture registration does not prove displayed frames or hardware zero-copy. The upstream runtime report exercised the production native bridge in a simulator using XComponents, without starting the full Flutter engine; it does not verify Dart-to-texture composition on a PC.

## Common mistakes

Do not unregister while the renderer still owns the window, multiply physical size by DPI twice, allow stale async creation to attach after disposal, or infer physical hardware decoding from a decoder label.

## Check it works

In a complete Flutter app on the intended PC, verify first frame, clipping/overlays, repeated resize/DPI changes, rapid player switches, route disposal during creation and engine shutdown. Track texture/window counts and inspect actual rendered frames.

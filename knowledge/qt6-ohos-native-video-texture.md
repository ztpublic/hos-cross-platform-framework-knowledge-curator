---
{
  "schema_version": 1,
  "id": "qt6-ohos-native-video-texture",
  "title": "Hand a Qt video window to OH's native image pipeline",
  "topic_key": "media.qt6-ohos-native-video-texture",
  "area": "media",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Create a dedicated QOpenGLWindow, build an OH_NativeImage around an external OES texture, and register its native window with the media engine before playback.",
  "when_to_use": "A Qt media application plays audio on OH but the video area stays black or washed out, or the decoder surface is lost when the video widget is recreated or resized.",
  "keywords": [
    "QOpenGLWindow",
    "createWindowContainer",
    "OH_NativeImage",
    "GL_TEXTURE_EXTERNAL_OES",
    "samplerExternalOES",
    "onativewindow",
    "video surface",
    "hardware texture",
    "Qt 6",
    "OHOS"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Qt 6 on OH PC with an external-texture video backend and a QPA that exposes native windows. Software or native-surface backends need their own presentation path.",
  "related": [
    "qt6-ohos-subtitle-overlay",
    "native-surface-owner",
    "video-surface-backend-contract"
  ]
}
---

# Hand a Qt video window to OH's native image pipeline

## When to use

A Qt media application plays audio on OH but the video area stays black or washed out, or the decoder surface is lost when the video widget is recreated or resized.

## Recommended approach

Create a dedicated `QOpenGLWindow`, wrap it in a native widget container, build an `OH_NativeImage` around an external OES texture, and register that native window with the media engine before playback. Keep one surface for the playback session.

## Implementation steps

1. Use a dedicated `QOpenGLWindow` rather than a top-level `QOpenGLWidget`; on OH a top-level GL widget can create a second Ability/window. Embed it with `QWidget::createWindowContainer` and mark it as filling its native parent if the QPA requires that hint.
2. In `initializeGL`, generate a `GL_TEXTURE_EXTERNAL_OES` texture, create an `OH_NativeImage` for it, and acquire the `OHNativeWindow` with `OH_NativeImage_AcquireNativeWindow`.
3. Set the buffer geometry to a non-zero video size before the decoder produces its first buffer. `OH_NativeImage` starts at 0×0, and a codec can block on a zero-sized producer queue; use the encoded size or a bootstrap size.
4. Register the frame-available listener and draw the external texture with a `samplerExternalOES` shader. Keep the GL context current for uploads.
5. Expose the native window to the media engine and register it before starting playback (`onativewindow` or the engine's drawable variable). Do not replace or recreate the surface when playback starts.
6. On resize, update the buffer geometry and viewport, but keep the same native window and decoder session.
7. If the first decoder session on a fresh surface renders washed out, a bounded warm-up (stop/play once after the first consumed frame) can recover while the color-metadata path is investigated.

## Conditions and alternatives

The path assumes an external-texture video backend and an OH QPA that exposes native windows. A software or native-surface backend needs its own presentation path. The warm-up is a workaround, not a color-management fix.

## Common mistakes

- Creating a top-level `QOpenGLWidget` and triggering an extra Ability.
- Leaving the NativeImage buffer at 0×0 and blaming the decoder.
- Recreating the surface after playback begins.
- Doing GL uploads without making the context current.
- Mixing logical and native sizes when setting buffer geometry.

## Check it works

Play a video and confirm the first frame appears, the image is not washed out, and resizing keeps the video aligned. Check logs for a non-zero buffer geometry, one surface creation, and frame callbacks. Rotate/close/open a second video without leaking the old native window.

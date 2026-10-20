---
{
  "schema_version": 1,
  "id": "qt6-ohos-gl4es-context-state",
  "title": "Keep Qt and a GL translation layer on one state model",
  "topic_key": "rendering.qt6-ohos-gl4es-context-state",
  "area": "rendering",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Register native EGL contexts with the translation layer, restore the GLES state it missed, and invalidate program, buffer and vertex-attrib caches that Qt can change behind its back.",
  "when_to_use": "A Qt 6 app rendering through a GL translation layer on OH shows white or transparent 3D content, crashes on the first solid, or renders opaque or misplaced faces after Qt composites a frame.",
  "keywords": [
    "OpenGL",
    "GLES",
    "EGL",
    "gl4es",
    "QOpenGLWidget",
    "QSurfaceFormat",
    "state cache",
    "vertex attrib",
    "texture namespace",
    "RasterGLSurface",
    "GL backing store",
    "Qt 6"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Qt 6 OH PC apps that share one native GLES context with a GL translation layer. A native-GL-only or translation-layer-only build needs less synchronization.",
  "related": [
    "qt6-ohos-gl4es-draw-batching",
    "qt6-ohos-native-video-texture",
    "native-surface-owner"
  ]
}
---

# Keep Qt and a GL translation layer on one state model

## When to use

A Qt 6 app rendering through a GL translation layer on OH shows white or transparent 3D content, crashes on the first solid, or renders opaque or misplaced faces after Qt composites a frame.

## Recommended approach

Register the native EGL context with the translation layer whenever it becomes current, restore the native GLES state it may have missed, and invalidate the caches that Qt can change behind its back. Enable the OH QPA's GL backing store for `QOpenGLWidget` content.

## Implementation steps

1. Expose an external-context activation entrypoint in the translation layer. Map each native EGL context to one translation-layer state; for a new context, create state from its share context and activate it.
2. On activation, resync native GLES capabilities the layer caches (blend, cull, depth, stencil, polygon offset, blend function, depth function and mask).
3. Invalidate the program, array/index buffer bindings and all vertex-attrib enable bits so the next draw re-emits them. Qt's compositor can change these while the layer still believes its cached values are current.
4. Resolve GL entry points through the context-aware EGL proc-address path after `eglMakeCurrent`, not through cached `dlsym` wrappers; otherwise entries can report invalid.
5. Keep texture names in one namespace: when a texture is created through native GL, translate to the layer's backing name before blitting, and restore the active texture unit after operations that bypass the layer.
6. For `QOpenGLWidget`/`RasterGLSurface`, enable the QPA GL backing store (`io.qt.experimental.enableGlBackingStore`) before the first window; the generic QRhi compositor may not be usable through the layer.
7. Exercise opaque, transparent, 2D and 3D content after each change.

## Conditions and alternatives

This applies when Qt and the translation layer share one native GLES context. A build where Qt uses only native GLES or only the layer needs less synchronization. The exact state fields and cache structures are layer-specific.

## Common mistakes

- Assuming EGL context creation automatically initializes the layer's per-context state.
- Resyncing capabilities but leaving program, buffer and attrib caches stale.
- Creating a texture through native GL and binding it later by the layer's public name.
- Using `dlsym` for GLES entry points that must be context-aware.
- Enabling the generic QRhi compositor with a `QOpenGLWidget`.

## Check it works

Render a 3D scene with opaque and transparent faces; create a solid and rotate it. Confirm no white viewport, no crash on the first draw, correct blending/depth, and no stale buffer/program state after a Qt composite. Compare a 2D-only window as a control.

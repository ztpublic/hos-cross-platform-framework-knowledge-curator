---
{
  "schema_version": 1,
  "id": "qt6-ohos-gl4es-draw-batching",
  "title": "Bound translated draw batches to avoid GPU device loss",
  "topic_key": "rendering.qt6-ohos-gl4es-draw-batching",
  "area": "rendering",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Keep asynchronous client arrays alive, upload translated fixed-function arrays to a scratch VBO, and flush native draw batches at a small fixed limit instead of serializing every draw.",
  "when_to_use": "A complex Qt/OpenGL scene on OH freezes or the GPU resets after sustained rotation, while smaller scenes render fine.",
  "keywords": [
    "gl4es",
    "vertex array",
    "VBO",
    "glFlush",
    "glFinish",
    "GPU device lost",
    "batch",
    "draw call",
    "rotation",
    "Qt 6"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Qt 6 OH PC apps rendering complex scenes through a GL translation layer whose client-array consumption is asynchronous. The safe batch size is GPU/driver specific.",
  "related": [
    "qt6-ohos-gl4es-context-state",
    "native-dependency-closure"
  ]
}
---

# Bound translated draw batches to avoid GPU device loss

## When to use

A complex Qt/OpenGL scene on OH freezes or the GPU resets after sustained rotation, while smaller scenes render fine.

## Recommended approach

Keep client-side vertex arrays alive until the asynchronous GLES consumer has used them, upload translated fixed-function color arrays into a scratch VBO, and flush translated batches at a small fixed limit instead of calling `glFinish()` after every draw.

## Implementation steps

1. Track pending native draws per translation-layer context.
2. For client-side arrays consumed asynchronously, either copy the data into a retained buffer or upload it to a temporary VBO before returning; do not let the caller free the array while the GPU may still read it.
3. Convert the fixed-function client color array into the temporary VBO as part of the batch, respecting the original type and stride.
4. Count native draws and call a non-blocking `glFlush()` every few draws (for example four) to bound the driver submission; reset the count after the flush.
5. Keep the throttle in the platform-specific path and make the threshold measurable; do not add a global `glFinish()`, which serializes the pipeline and collapses frame rate.
6. Re-test with a large model and sustained camera motion, watching for device-lost, reset or page-fault logs.

## Conditions and alternatives

The correct threshold depends on the GPU, driver and scene. A different device may tolerate larger batches; re-measure before changing it. `glFlush()` is not a synchronization barrier; code that needs completion still needs an explicit wait.

## Common mistakes

- Freeing a client array immediately after the draw call while GLES consumption is asynchronous.
- Calling `glFinish()` after every draw and mistaking the low frame rate for a rendering fix.
- Flushing only at frame end, allowing one huge submission.
- Applying the throttle to all platforms without measurement.

## Check it works

Rotate a large model continuously for several minutes. Confirm the process survives, no device-lost/reset/page-fault logs appear, and frame time remains usable. Compare with a small scene to confirm the throttle did not regress ordinary rendering.

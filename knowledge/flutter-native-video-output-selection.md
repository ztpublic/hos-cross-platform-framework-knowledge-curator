---
{
  "schema_version": 1,
  "id": "flutter-native-video-output-selection",
  "title": "Branch custom renderers by the active output type",
  "topic_key": "media.flutter-native-video-output-selection",
  "area": "media",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "summary": "Mount the widget matching the active native output and reconstruct through the same queue when the host changes.",
  "when_to_use": "A texture-only custom widget shows no picture for a native platform-view backend.",
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
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Different media backends have different OH support and integration requirements. A backend excluded by one dependency graph may be viable through another supported implementation.",
  "related": [
    "video-surface-backend-contract"
  ]
}
---

# Branch custom renderers by the active output type

## When to use

A texture-only custom widget shows no picture for a native platform-view backend.

## Recommended approach

Mount the widget matching the active native output and reconstruct through the same queue when the host changes.

## Implementation steps

A lightweight video widget copied from a texture-only backend must not require a texture ID when the selected controller outputs through a native platform view. Observe platform-controller readiness as well as geometry and texture changes, inspect the actual platform controller's output mode, and mount its supported native video widget when required. Wait for valid dimensions rather than treating an absent texture ID as a decoder failure. Use the backend's documented units when converting video geometry to logical layout size.

Include in-app overlays in the render-state policy. If their transforms, clipping or ownership cannot host the native view, request a supported alternate path through the same reconstruction queue, then restore the normal path on return. A renderer capable of constructing a platform view does not by itself prove that view can be moved into every overlay. Test controller replacement after the lightweight widget has mounted, plus rapid enter/exit and disposal during a queued rebuild.

## Conditions and alternatives

Different media backends have different OH support and integration requirements. A backend excluded by one dependency graph may be viable through another supported implementation.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Test texture and native-view outputs, delayed dimensions, controller replacement, in-app overlays, rapid entry/exit and disposal during reconstruction.

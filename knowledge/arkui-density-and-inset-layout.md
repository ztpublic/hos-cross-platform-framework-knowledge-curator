---
{
  "schema_version": 1,
  "id": "arkui-density-and-inset-layout",
  "title": "Recompute native layout when density changes",
  "topic_key": "window.arkui-density-and-inset-layout",
  "area": "window",
  "scope": "common",
  "frameworks": [],
  "summary": "Use the owning UIContext to convert current physical bounds, listen for density changes and merge avoid areas once per edge.",
  "when_to_use": "A PC window keeps its physical size but logical layout becomes wrong after a density or display change.",
  "keywords": [
    "DPI",
    "platform view",
    "HCPP",
    "keyboard",
    "resize",
    "scroll offset",
    "缩放",
    "触摸",
    "platform view touch offset",
    "DPI mismatch",
    "resize loses scroll",
    "触摸偏移",
    "键盘遮挡",
    "WebView page zoom",
    "CSS zoom",
    "double zoom",
    "densityUpdate",
    "UIContext.px2vp",
    "safe area",
    "网页缩放"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to ArkUI shells using vp and a live owning UIContext; verify density and avoid-area events in the selected SDK and PC window modes.",
  "related": [
    "viewport-scale-and-layout-lifetime"
  ]
}
---

# Recompute native layout when density changes

## When to use

A PC window keeps its physical size but logical layout becomes wrong after a density or display change.

## Recommended approach

Use the owning UIContext to convert current physical bounds, listen for density changes and merge avoid areas once per edge.

## Implementation steps

If an ArkUI shell stores its container size in vp, use the owning, initialized `UIContext.px2vp` for physical window dimensions. Listen for `densityUpdate` as well as window-size changes: physical bounds can stay constant while their logical size changes. Reread the current window rectangle, recompute logical dimensions and update the shell state. Remove the exact listener when that UI instance disappears.

Treat native avoid-area rectangles as window-relative geometry. Derive each edge's inset using its position and extent, then combine overlapping system-bar and cutout exclusions by per-edge maximum rather than addition. Convert physical pixels once before merging with the shared layout's existing system insets; do not apply the same inset again inside the content. Query failure and unsupported window modes need an explicit fallback, not a fabricated valid zero area. Test phone and free-window behavior separately.

## Conditions and alternatives

Applies to ArkUI shells using vp and a live owning UIContext; verify density and avoid-area events in the selected SDK and PC window modes.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Change density with unchanged physical window bounds, move/resize free windows, vary system bars/cutouts and recreate the UI owner. Confirm correct hit coordinates, no double insets and exact listener removal.

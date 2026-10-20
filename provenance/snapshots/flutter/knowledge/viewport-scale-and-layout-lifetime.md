---
{
  "id": "viewport-scale-and-layout-lifetime",
  "title": "Fix platform-view hit testing and state loss during resize",
  "topic_key": "window.viewport-scale-and-layout-lifetime",
  "area": "window",
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
  "summary": "Locate the mismatch between native metrics, app scaling and widget lifetime before applying a scoped layout or embedding workaround.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "An embedded native view draws or receives taps at the wrong scale, or resizing/keyboard changes reset scroll position and controller state."
}
---

# Fix platform-view hit testing and state loss during resize

## When to use

An embedded native view draws or receives taps at the wrong scale, or resizing/keyboard changes reset scroll position and controller state.

## Recommended approach

Locate the mismatch between native metrics, app scaling and widget lifetime before applying a scoped layout or embedding workaround.

## Implementation steps

1. Compare native viewport dimensions/density with Flutter's logical size, app-level scaling and the embedded view's bounds.
2. Test the native view without custom scaling/transforms. If that isolates the problem, temporarily neutralize only the relevant scale and restore it on disposal.
3. Preserve controllers and stable widget identity when responsive layouts reparent content. Avoid rebuilding the owning state merely because insets change.
4. For keyboard issues, compare native container sizing with Flutter inset handling; adjust the affected screen rather than disabling resizing globally.
5. Consider an embedding patch only after the mismatch is localized to a specific embedding implementation, and keep the patch version-scoped.

## Conditions and alternatives

Platform-view composition modes and density handling differ by embedding/backend. A coordinate workaround for one mode may be wrong for another.

### Separate WebView page zoom from application paint zoom

Track application scale and page zoom independently. When the OH embedding composes the native view through Flutter's paint transform, keep application scale there and send only page zoom to ArkWeb. Multiplying both factors again in native content applies application scale twice. A native overlay backend that cannot transform input coordinates may require a different adapter; do not copy its scale cancellation into a composited OH view.

If the selected ArkWeb SDK lacks the required absolute page-zoom API, a scoped alternative is main-frame CSS `zoom`. Preserve the root's original inline value and priority and its computed base zoom, apply the page factor relative to that base, and restore the original style at 1x. Apply the change to the current document and retain it through [document-start registration](arkweb-document-start-registration.md) for future navigation. Keep that policy independent from removable user scripts.

CSS zoom requires JavaScript execution and may differ from native zoom in layout, frames and page interaction. Use it only when those differences meet the application's requirements; keep the native option when it supplies the necessary behavior.

### Recompute native layout when density changes

If an ArkUI shell stores its container size in vp, use the owning, initialized `UIContext.px2vp` for physical window dimensions. Listen for `densityUpdate` as well as window-size changes: physical bounds can stay constant while their logical size changes. Reread the current window rectangle, recompute logical dimensions and update the shell state. Remove the exact listener when that UI instance disappears.

Treat native avoid-area rectangles as window-relative geometry. Derive each edge's inset using its position and extent, then combine overlapping system-bar and cutout exclusions by per-edge maximum rather than addition. Convert physical pixels once before merging with Flutter `padding`/`viewPadding`; do not apply the same inset again inside the content. Query failure and unsupported window modes need an explicit fallback, not a fabricated valid zero area. Test phone and free-window behavior separately.

## Common mistakes

Do not compensate for native scaling twice. A screen-specific keyboard workaround does not mean the OS cannot resize Flutter content correctly.

## Check it works

Test tap coordinates, multiple densities, keyboard open/close, window moves/resizes, scroll retention and restoration of temporary scale state.

For WebViews, vary app scale and page zoom separately and together. Verify links and text selection, navigation/reload, a page with its own root zoom, reset to 1x, user-script removal and the JavaScript-disabled case.

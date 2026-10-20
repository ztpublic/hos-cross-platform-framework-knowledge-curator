---
{
  "schema_version": 1,
  "id": "viewport-scale-and-layout-lifetime",
  "topic_key": "window.viewport-scale-and-layout-lifetime",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "title": "Fix platform-view hit testing and state loss during resize",
  "area": "window",
  "summary": "Locate the mismatch between native metrics, app scaling and widget lifetime before applying a scoped layout or embedding workaround.",
  "when_to_use": "An embedded native view draws or receives taps at the wrong scale, or resizing/keyboard changes reset scroll position and controller state.",
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
  "applicability": "Flutter OH with the selected embedding/plugin versions; PC behavior requires the stated acceptance checks.",
  "related": [
    "arkui-density-and-inset-layout",
    "flutter-arkweb-page-zoom"
  ]
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

See [Separate WebView page zoom from application paint zoom](flutter-arkweb-page-zoom.md) for this independent contract.

See [Recompute native layout when density changes](arkui-density-and-inset-layout.md) for this independent contract.

## Common mistakes

Do not compensate for native scaling twice. A screen-specific keyboard workaround does not mean the OS cannot resize Flutter content correctly.

## Check it works

Test tap coordinates, multiple densities, keyboard open/close, window moves/resizes, scroll retention and restoration of temporary scale state.

For WebViews, vary app scale and page zoom separately and together. Verify links and text selection, navigation/reload, a page with its own root zoom, reset to 1x, user-script removal and the JavaScript-disabled case.

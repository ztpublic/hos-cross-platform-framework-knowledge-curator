---
{
  "schema_version": 1,
  "id": "flutter-arkweb-page-zoom",
  "title": "Separate WebView page zoom from application paint zoom",
  "topic_key": "window.flutter-arkweb-page-zoom",
  "area": "window",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "summary": "Keep page zoom independent from Flutter paint transforms and preserve the original page style when using a CSS fallback.",
  "when_to_use": "Application scaling and ArkWeb zoom multiply twice or reset incorrectly.",
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
  "applicability": "Platform-view composition modes and density handling differ by embedding/backend. A coordinate workaround for one mode may be wrong for another.",
  "related": [
    "arkweb-document-start-registration",
    "viewport-scale-and-layout-lifetime"
  ]
}
---

# Separate WebView page zoom from application paint zoom

## When to use

Application scaling and ArkWeb zoom multiply twice or reset incorrectly.

## Recommended approach

Keep page zoom independent from Flutter paint transforms and preserve the original page style when using a CSS fallback.

## Implementation steps

Track application scale and page zoom independently. When the OH embedding composes the native view through Flutter's paint transform, keep application scale there and send only page zoom to ArkWeb. Multiplying both factors again in native content applies application scale twice. A native overlay backend that cannot transform input coordinates may require a different adapter; do not copy its scale cancellation into a composited OH view.

If the selected ArkWeb SDK lacks the required absolute page-zoom API, a scoped alternative is main-frame CSS `zoom`. Preserve the root's original inline value and priority and its computed base zoom, apply the page factor relative to that base, and restore the original style at 1x. Apply the change to the current document and retain it through [document-start registration](arkweb-document-start-registration.md) for future navigation. Keep that policy independent from removable user scripts.

CSS zoom requires JavaScript execution and may differ from native zoom in layout, frames and page interaction. Use it only when those differences meet the application's requirements; keep the native option when it supplies the necessary behavior.

## Conditions and alternatives

Platform-view composition modes and density handling differ by embedding/backend. A coordinate workaround for one mode may be wrong for another.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Vary application scale and page zoom independently and together; check taps/selection, existing root CSS zoom, navigation, reset to 1x, script removal and JavaScript-disabled fallback.

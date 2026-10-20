---
{
  "schema_version": 1,
  "id": "rendering-budget-and-progressive-ui",
  "topic_key": "rendering.rendering-budget-and-progressive-ui",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "title": "Reduce expensive effects and transition work on constrained devices",
  "area": "rendering",
  "summary": "Provide an explicit lower-cost visual path and schedule expensive work around interaction, preserving usable content while it completes.",
  "when_to_use": "Blurred surfaces or reader transitions drop frames or consume excessive memory on the intended OH devices.",
  "keywords": [
    "BackdropFilter",
    "blur",
    "performance",
    "缓存",
    "首帧",
    "动画",
    "progressive rendering",
    "BackdropFilter jank",
    "blur performance",
    "reader transition",
    "动画卡顿"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Flutter OH with the selected embedding/plugin versions; PC behavior requires the stated acceptance checks.",
  "related": [
    "startup-work-prerequisites"
  ]
}
---

# Reduce expensive effects and transition work on constrained devices

## When to use

Blurred surfaces or reader transitions drop frames or consume excessive memory on the intended OH devices.

## Recommended approach

Provide an explicit lower-cost visual path and schedule expensive work around interaction, preserving usable content while it completes.

## Implementation steps

1. Measure frame time and memory to identify whether the cost is effects, rebuilding, decoding or native work.
2. For an optional lightweight mode, replace blur-heavy effects with simpler gradients/opaque surfaces, reduce shadows and keep text/layout semantics unchanged.
3. Defer nonessential heavy rebuilding until route transitions finish, and initiate channel-backed work after required frame/context setup.
4. Keep existing/original content visible while preparing replacements.
5. Bound caches and use a conservative policy if a native memory-info capability is unavailable.
6. Cancel or invalidate deferred work when the screen/session is disposed.

For decorative press animations started by `Listener.onPointerDown`, a touch that becomes a drag can already have started repeated painting. Gate new decoration while the relevant scroll scope is active; when scrolling begins, stop any running animation and reset its visual state without a reverse animation. Propagate `ScrollStartNotification` and `ScrollEndNotification` state through a scoped notifier, remove listeners on disposal, and preserve the scroll gesture by allowing notifications to continue bubbling. Use active-scroll ownership or a count for overlapping/nested scrolls; one global boolean can be cleared too early by another scroll ending.

## Conditions and alternatives

The right tradeoff depends on hardware and content. This is an application rendering policy, not a claim that OH lacks Flutter visual effects.

See [Reduce startup work without delaying correctness prerequisites](startup-work-prerequisites.md) for this independent contract.

## Common mistakes

Deferred work can still jank later or run after disposal. A fallback cache size is not actual memory telemetry, and removing effects without measurement may not fix the bottleneck.

## Check it works

Compare frame time, memory, visual quality and responsiveness on representative devices; test rapid navigation and persistence of the user's visual preference.

Also test tap, long press, touch-then-drag, scrolling while an effect is active, and simultaneous/nested scrolling. Confirm the idle decoration does not keep a ticker or trigger whole-card repainting.

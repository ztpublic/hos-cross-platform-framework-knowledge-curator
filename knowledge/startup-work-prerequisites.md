---
{
  "schema_version": 1,
  "id": "startup-work-prerequisites",
  "title": "Reduce startup work without delaying correctness prerequisites",
  "topic_key": "rendering.startup-work-prerequisites",
  "area": "rendering",
  "scope": "common",
  "frameworks": [],
  "summary": "Complete correctness prerequisites first, then schedule owned optional work after a usable first frame.",
  "when_to_use": "First display is delayed by housekeeping or scans, or deferral starts consumers before storage and grants are ready.",
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
  "applicability": "The right tradeoff depends on hardware and content. This is an application rendering policy, not a claim that the OS lacks those visual effects.",
  "related": [
    "rendering-budget-and-progressive-ui"
  ]
}
---

# Reduce startup work without delaying correctness prerequisites

## When to use

First display is delayed by housekeeping or scans, or deferral starts consumers before storage and grants are ready.

## Recommended approach

Complete correctness prerequisites first, then schedule owned optional work after a usable first frame.

## Implementation steps

Separate prerequisites for safe first use from optional refreshes. Restore required URI grants and initialize the storage needed by the first screen before consumers run; defer cache housekeeping, nonessential network renewal and library scans until a usable first frame. Coordinate a launch-file playback operation with scanning so the scan does not immediately compete with the requested playback.

Wrap deferred tasks with an owner/generation, explicit error handling and timing. Post-frame callbacks schedule work, but do not make synchronous CPU or native calls nonblocking. For frame diagnostics, derive budget from the actual display refresh rate and flush bounded measurements on background transitions, since the next frame may never arrive. Measure cold startup and follow-on interaction cost separately.

## Conditions and alternatives

The right tradeoff depends on hardware and content. This is an application rendering policy, not a claim that the OS lacks those visual effects.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Measure cold first-frame and subsequent interaction costs separately. Verify no consumer runs before required grants/storage, and that deferred work is cancelled or ignored after owner disposal.

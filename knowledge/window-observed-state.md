---
{
  "schema_version": 1,
  "id": "window-observed-state",
  "title": "Separate requested visibility from focus and drawable surface state",
  "topic_key": "window.window-observed-state",
  "area": "window",
  "scope": "common",
  "frameworks": [],
  "summary": "Reconcile platform window status, focus, framework visibility and surface readiness independently.",
  "when_to_use": "A PC window is brought forward but the framework still thinks it is hidden or has no drawable surface.",
  "keywords": [
    "focus",
    "visibility",
    "white screen",
    "窗口",
    "WMS"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to asynchronous PC window adapters. Callback names and resynchronization hooks belong to the selected framework/backend.",
  "related": []
}
---

# Separate requested visibility from focus and drawable surface state

## When to use

A PC window is brought forward but the framework still thinks it is hidden or has no drawable surface.

## Recommended approach

Reconcile platform window status, focus, framework visibility and surface readiness independently.

## Implementation steps

1. Capture the requested transition and the current native window generation.
2. Observe actual window status/exposure/focus events rather than a single show/hide callback.
3. Submit rendering only when the current surface is valid and ready; update framework state from appropriate native observations.
4. Cancel pending transition work on destruction and report timeout as unknown/incomplete rather than completed visibility.

## Conditions and alternatives

Applies to asynchronous PC window adapters. Callback names and resynchronization hooks belong to the selected framework/backend.

## Common mistakes

A window object, successful command or focus event does not prove every visibility/render state is synchronized.

## Check it works

Test task switching, preload/restore, minimize/recover, close/reopen and surface recreation while comparing native and framework state.

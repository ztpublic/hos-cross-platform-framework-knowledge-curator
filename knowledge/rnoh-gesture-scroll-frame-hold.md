---
{
  "schema_version": 1,
  "id": "rnoh-gesture-scroll-frame-hold",
  "title": "Hold RNOH scroll frames while gesture arbitration waits",
  "topic_key": "input.rnoh-gesture-scroll-frame-hold",
  "area": "input",
  "scope": "framework-specific",
  "frameworks": [
    "react-native"
  ],
  "summary": "Implement a temporary frame/delegate hold for a pending native gesture, retain nested-scroll ownership, and restore the current ScrollView policy when arbitration finishes.",
  "when_to_use": "An RNOH pan/waitFor interaction scrolls the parent before the gesture is decided, or leaves a list stuck after cancellation.",
  "applicability": "RNOH C++ gesture-handler ports using ScrollViewComponentInstance and ArkUI native scroll nodes. Relevant to nested PC touch/trackpad interactions; mouse-wheel and pointer-type parity are not established.",
  "keywords": [
    "ScrollFramesHold",
    "waitFor",
    "onScrollFrameBegin",
    "NODE_SCROLL_NESTED_SCROLL",
    "SELF_ONLY",
    "gesture arbitration"
  ],
  "related": [
    "rnoh-native-component-registration-parity",
    "device-gesture-capability"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Hold RNOH scroll frames while gesture arbitration waits

## When to use

An RNOH pan/waitFor interaction scrolls the parent before the gesture is decided, or leaves a list stuck after cancellation.

## Recommended approach

Implement a temporary frame/delegate hold for a pending native gesture, retain nested-scroll ownership, and restore the current ScrollView policy when arbitration finishes.

## Implementation steps

1. Install the hold only while the ScrollView owner exists, replacing its ScrollNode delegate with a forwarding delegate.
2. While waiting, return zero frame offset and avoid reporting a drag to RN. Keep nested scroll SELF_ONLY so unused delta does not escape to a parent.
3. Save forward/backward nested-scroll modes when forcing ownership; forward ordinary events to the owner.
4. On release/destruction, restore the original delegate and reconcile modes with edge position and any newer prop-driven policy rather than blindly restoring a stale snapshot.
5. Ensure cancellation/drop/owner destruction releases every hold on the owning UI thread.

## Conditions and alternatives

This depends on RNOH internal ScrollView/delegate APIs and the selected gesture port. It is not a portable JS-only workaround or proof of every gesture recognizer's correctness.

## Common mistakes

Rejecting a recognizer too early changes waitFor semantics; returning zero without holding nested-scroll policy can still move the parent.

## Check it works

Test success/failure/cancel, nested edges, props changing during a hold, destroyed ScrollViews and repeated gestures. Confirm no stuck scroll or duplicate drag start/stop.

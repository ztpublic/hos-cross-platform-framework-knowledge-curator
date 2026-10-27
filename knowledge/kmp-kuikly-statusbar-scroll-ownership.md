---
{
  "schema_version": 1,
  "id": "kmp-kuikly-statusbar-scroll-ownership",
  "title": "Give the OH renderer one owner for status-bar scroll-to-top",
  "topic_key": "input.kmp-kuikly-statusbar-scroll-ownership",
  "area": "input",
  "scope": "framework-specific",
  "frameworks": [
    "kotlin-multiplatform"
  ],
  "summary": "Disable the built-in node back-to-top action when the renderer owns the status-bar event, and let each active scroll container choose callback handling or default scrolling.",
  "when_to_use": "A status-bar click scrolls twice, ignores a business callback, or moves a hidden KMP page.",
  "applicability": "Kuikly native OH renderer revisions with common-event integration. Relevant only on PC/window configurations that expose a clickable status bar and emit its event; desktop chrome is not assumed equivalent.",
  "keywords": [
    "Kuikly",
    "COMMON_EVENT_CLICK_STATUSBAR",
    "NODE_SCROLL_BACK_TO_TOP",
    "scrollToTop",
    "active page",
    "API 15"
  ],
  "related": [
    "kmp-kuikly-ohos-host",
    "kmp-kuikly-ohos-resize-sync",
    "native-arkui-node-attribute-api-guard"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Give the OH renderer one owner for status-bar scroll-to-top

## When to use

A status-bar click scrolls twice, ignores a business callback, or moves a hidden KMP page.

## Recommended approach

Disable the built-in node back-to-top action when the renderer owns the status-bar event, and let each active scroll container choose callback handling or default scrolling.

## Implementation steps

1. Subscribe by renderer instance identity before its view tree is constructed; release the subscription during destruction.
2. Marshal delivery onto the owning UI thread and skip paused, hidden or destroyed render instances.
3. From API 15, set `NODE_SCROLL_BACK_TO_TOP` to false on managed scroll nodes. Repeat this idempotently when a node attaches.
4. If `scrollToTop` is registered, call it without auto-scrolling; otherwise scroll the container to the origin. Reset the callback when the property is removed.

## Conditions and alternatives

Keep existing behavior below the attribute API gate. Multiple visible instances and nested scrollers need an explicit routing policy; the source iterates containers in each active instance.

## Common mistakes

Leaving both the OS action and custom event active causes duplicate effects; a global subscription without instance cleanup reaches stale views.

## Check it works

Test callback present/absent/reset, hidden tabs, multiple instances, nested lists and repeated mount/destroy. Confirm one event has the intended effect.

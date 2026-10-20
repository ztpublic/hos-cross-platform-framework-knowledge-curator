---
{
  "schema_version": 1,
  "id": "qt-close-event-origin",
  "title": "Handle Qt close events according to their native origin",
  "topic_key": "lifecycle.qt-close-event-origin",
  "area": "lifecycle",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Distinguish interactive window closing from ability termination and persist incrementally instead of relying on shutdown UI.",
  "when_to_use": "A save prompt stalls termination or an ordinary window close is handled like system shutdown.",
  "keywords": [
    "closeEvent",
    "AbilityClose",
    "WindowStageClose",
    "OnPrepareToTerminate"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "The OS may terminate a process without a useful final callback. Forced termination cannot be promised as interceptable.",
  "related": [
    "foreground-recovery-and-save-lifetime"
  ]
}
---

# Handle Qt close events according to their native origin

## When to use

A save prompt stalls termination or an ordinary window close is handled like system shutdown.

## Recommended approach

Distinguish interactive window closing from ability termination and persist incrementally instead of relying on shutdown UI.

## Implementation steps

1. Inspect the selected Qt OH close-cause API and map its native origins.
2. In Qt 5 variants with Extras, use the actual scoped CloseEventRootCause values and the getCloseEventRootCause free function.
3. Allow interactive save/cancel only on the supported user-close path; use bounded noninteractive persistence for termination callbacks.
4. For Qt 6, inspect changed/private close APIs rather than copying Qt 5 enum names.

## Conditions and alternatives

The OS may terminate a process without a useful final callback. Forced termination cannot be promised as interceptable.

## Common mistakes

Do not show a blocking dialog in ability shutdown or assume all close signals mean the user clicked a window button.

## Check it works

Test window close, task removal, termination and restart after unsaved edits.

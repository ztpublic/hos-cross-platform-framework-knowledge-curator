---
{
  "schema_version": 1,
  "id": "input-driven-io-coalescing",
  "title": "Coalesce expensive I/O triggered by text editing",
  "topic_key": "ui.input-driven-io-coalescing",
  "area": "ui",
  "scope": "common",
  "frameworks": [],
  "summary": "Use an owned restartable debounce or an explicit submit action, then validate input and correlate results with the current edit generation.",
  "when_to_use": "Every keystroke starts parsing or file I/O, causing stalls, races or stale results.",
  "keywords": [
    "debounce",
    "textChanged",
    "I/O",
    "防抖"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Common UI mechanism extracted from a Qt textChanged failure; a debounce does not prove the underlying parser or native library is crash-safe.",
  "related": []
}
---

# Coalesce expensive I/O triggered by text editing

## When to use

Every keystroke starts parsing or file I/O, causing stalls, races or stale results.

## Recommended approach

Use an owned restartable debounce or an explicit submit action, then validate input and correlate results with the current edit generation.

## Implementation steps

1. Cancel/restart one pending debounce when the input changes; choose its interval from interaction needs.
2. Validate the path/input and bound parsing before launching expensive work.
3. Move blocking work off the UI path where supported and ignore completion from superseded edits.
4. Check empty/short parse results before indexing and release operation resources on owner destruction.

## Conditions and alternatives

Common UI mechanism extracted from a Qt textChanged failure; a debounce does not prove the underlying parser or native library is crash-safe.

## Common mistakes

Multiple delayed callbacks with only string equality can still repeat work when input changes away and back. A speculative symbol-resolution theory is not a confirmed cause.

## Check it works

Test rapid typing, same-value revisits, invalid/large files, empty parsing and navigation during work.

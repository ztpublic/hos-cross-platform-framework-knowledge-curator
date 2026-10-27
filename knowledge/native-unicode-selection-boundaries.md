---
{
  "schema_version": 1,
  "id": "native-unicode-selection-boundaries",
  "title": "Expand native text selections on Unicode boundaries",
  "topic_key": "input.native-unicode-selection-boundaries",
  "area": "input",
  "scope": "common",
  "frameworks": [],
  "summary": "Compute half-open selection ranges with ICU grapheme, word and sentence breaks, then map that range to rendered glyph boxes without changing its logical endpoints.",
  "when_to_use": "Native text selection splits an emoji or combining sequence, selects nothing on punctuation, or starts on the wrong line after a soft wrap.",
  "applicability": "Custom native text renderers using UTF-16 offsets and ICU on OH PC. Applies to pointer and touch selection; ICU availability and glyph-offset conventions must match the renderer.",
  "keywords": [
    "ICU",
    "UBRK_CHARACTER",
    "UBRK_WORD",
    "UBRK_SENTENCE",
    "UTF-16",
    "grapheme",
    "emoji",
    "soft wrap",
    "selection"
  ],
  "related": [
    "text-weight-versus-scale-policy"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Expand native text selections on Unicode boundaries

## When to use

Native text selection splits an emoji or combining sequence, selects nothing on punctuation, or starts on the wrong line after a soft wrap.

## Recommended approach

Compute half-open selection ranges with ICU grapheme, word and sentence breaks, then map that range to rendered glyph boxes without changing its logical endpoints.

## Implementation steps

1. Use `ubrk_open`, `ubrk_following`, `ubrk_preceding` and `ubrk_close` over the exact UTF-16 text used by layout.
2. For word selection, inspect the break rule status and fall back from whitespace or punctuation to the touched grapheme cluster. Define paragraph terminators separately.
3. Preserve `[start,end)` as the logical range. Derive visible rectangles from the first and last glyph intersecting it; lock the first matching line at soft-wrap boundaries.
4. Keep drag-handle character updates separate from initial word/sentence expansion, and validate offsets before applying fallback arithmetic.

## Conditions and alternatives

The source links the platform ICU library. Empty text, invalid offsets and failed break-iterator creation require explicit safe handling; a one-code-unit fallback is insufficient for arbitrary Unicode.

## Common mistakes

Treating UTF-16 code units as characters or deriving logical end offsets from zero-width line terminator boxes corrupts the selected range.

## Check it works

Test CJK, surrogate pairs, combining marks, ZWJ emoji, punctuation, CR/LF and wrapped spans; copied text must equal the requested logical interval.

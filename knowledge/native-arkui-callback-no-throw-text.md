---
{
  "schema_version": 1,
  "id": "native-arkui-callback-no-throw-text",
  "title": "Keep text conversion from throwing inside ArkUI node event callbacks",
  "topic_key": "input.native-arkui-callback-no-throw-text",
  "area": "input",
  "scope": "common",
  "frameworks": [],
  "summary": "Cap pasted text on a UTF-8 character boundary and use replacing decoders in ArkUI node event paths, because a C++ exception escaping the callback terminates the process.",
  "when_to_use": "Pasting long text into a length-limited native input crashes with SIGABRT, often only when the limit is counted in characters and the pasted text is multi-byte.",
  "applicability": "Native C++ text handling on OH_ArkUI_NodeEvent callbacks. PC boundary: large clipboard pastes from a keyboard are routine, so the limit path is reached more often than with touch input.",
  "keywords": [
    "paste crash",
    "SIGABRT",
    "OH_ArkUI_NodeEvent",
    "UTF-8 truncation",
    "std::range_error",
    "wstring_convert",
    "maxLength",
    "U+FFFD"
  ],
  "related": [
    "native-unicode-selection-boundaries",
    "utf8-command-payload-transport",
    "native-arkui-node-attribute-api-guard"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Keep text conversion from throwing inside ArkUI node event callbacks

## When to use

Pasting long text into a length-limited native input crashes with SIGABRT, often only when the limit is counted in characters and the pasted text is multi-byte.

## Recommended approach

Cap pasted text on a UTF-8 character boundary and use replacing decoders in ArkUI node event paths, because a C++ exception escaping the callback terminates the process.

## Implementation steps

1. Find byte-based caps such as min(strlen(text), maxLength * 4). They are acceptable as copy limits only if they end on a character boundary.
2. Step back from the cut over UTF-8 continuation bytes, at most three, and drop a final sequence that the cut left incomplete.
3. Replace throwing conversions such as std::wstring_convert with decoders that substitute U+FFFD for ill-formed input, and use one replacement rule in each direction.
4. Do semantic length limiting afterwards, on the decoded text, in the filter that knows whether the limit counts bytes, characters or visual width.
5. Treat the C callback boundary as no-throw. Catch at the outermost handler when called code might still throw.

## Conditions and alternatives

Replacement changes text only when the input is ill-formed; legitimate text is untouched. Where data integrity matters more than continuity, return an explicit error instead, still without throwing across the callback.

## Common mistakes

Fixing only the truncation leaves any other ill-formed source able to crash the app. Fixing only the decoder hides truncation as replacement characters in the input.

## Check it works

Paste strings of CJK, emoji and invalid byte sequences at limits that fall inside a multi-byte character, using byte, character and visual-width limit modes. Expect no crash, no partial character and correct length handling.

---
{
  "schema_version": 1,
  "id": "typed-channel-payload-extraction",
  "topic_key": "channels.typed-channel-payload-extraction",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "title": "Preserve typed values across a Dart–ArkTS channel",
  "area": "channels",
  "summary": "Extract and validate an explicit typed payload at the channel boundary before passing it to application logic or JSON serialization.",
  "when_to_use": "A native session loses fields, serializes to unexpected JSON, or fails because channel arguments were treated like ordinary JavaScript objects.",
  "keywords": [
    "ArkTS",
    "MethodCall",
    "codec",
    "call.argument",
    "typed model",
    "类型",
    "JSON",
    "channel args JSON empty",
    "ArkTS typed arguments",
    "参数丢失",
    "codec mismatch"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Flutter OH with the selected embedding/plugin versions; PC behavior requires the stated acceptance checks.",
  "related": [
    "chunked-native-file-transfer"
  ]
}
---

# Preserve typed values across a Dart–ArkTS channel

## When to use

A native session loses fields, serializes to unexpected JSON, or fails because channel arguments were treated like ordinary JavaScript objects.

## Recommended approach

Extract and validate an explicit typed payload at the channel boundary before passing it to application logic or JSON serialization.

## Implementation steps

1. Define the payload schema, version, required fields, optional fields and supported codec types.
2. Use the selected embedding's argument accessor to read each field. Check missing/null values and validate types, ranges and size limits.
3. Construct an ArkTS typed model from the validated values. Handle optional fields intentionally instead of relying on accidental undefined serialization.
4. Serialize only that model if a downstream component needs JSON; do not stringify the raw codec container blindly.
5. Return clear invalid-argument errors and catch asynchronous failures in the operation that owns the result.
6. Keep I/O handle cleanup in `finally` when processing the payload opens files or resources.
7. If a typed byte buffer passes through JSON, explicitly encode the bytes, such as base64, and decode them at the receiving boundary. JSON object serialization is not a binary transport. Prefer a supported binary codec when JSON is unnecessary. Preserve empty bytes versus missing content and reject malformed encodings instead of dropping fields.
8. Specify whether each response is a JSON string or a decoded map. Parse a native SDK's JSON text before returning a map expected by Dart; stringifying the text again adds another encoding layer and breaks typed consumers.

## Conditions and alternatives

Use a generated typed bridge for larger interfaces when a compatible OH generator is available; both sides must be generated from the same contract/version.

### Give native result objects an explicit structural schema

If ArkTS rejects or obscures a generic `Object` result, define an interface for the bridge's actual fields and return that structure. Preserve honest unknown metadata rather than inventing filesystem identity from placeholder zeros. Validate nullable values and serialization on both sides; declaring a structure does not establish that every field reflects a measured native value.

For large binary transfers, use the [owned chunk protocol](chunked-native-file-transfer.md) rather than an unbounded JSON array. Enforce integral byte/index/length fields before converting to Uint8Array; coercion can silently wrap invalid values.

## Common mistakes

A cast does not validate an external value. Similar-looking accessors or APIs from another embedding version may not have the same contract.

## Check it works

Round-trip numbers, arrays, optional/null fields, Unicode and non-text binary bytes. Include empty buffers and bytes above 127; reject missing, malformed and oversized payloads and verify each call completes once.

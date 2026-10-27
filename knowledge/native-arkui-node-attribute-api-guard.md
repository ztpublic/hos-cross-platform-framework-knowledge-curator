---
{
  "schema_version": 1,
  "id": "native-arkui-node-attribute-api-guard",
  "title": "Guard native ArkUI attributes by runtime API level",
  "topic_key": "native.native-arkui-node-attribute-api-guard",
  "area": "native",
  "scope": "common",
  "frameworks": [],
  "summary": "Check the running OS API before setting a newer ArkUI node attribute; a newer build SDK or exported dispatcher does not establish that its implementation exists.",
  "when_to_use": "A native image node crashes at address zero or rejects NODE_IMAGE_SOURCE_SIZE on an older OH image.",
  "applicability": "Native ArkUI node consumers on OH PC and other products. The reviewed source skips NODE_IMAGE_SOURCE_SIZE below API 21; independent product validation remains required.",
  "keywords": [
    "NODE_IMAGE_SOURCE_SIZE",
    "OH_GetSdkApiVersion",
    "setAttribute",
    "API 21",
    "null function pointer",
    "image decode size"
  ],
  "related": [
    "hap-minimum-api-compatibility",
    "sdk-version-versus-os-limitation"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Guard native ArkUI attributes by runtime API level

## When to use

A native image node crashes at address zero or rejects NODE_IMAGE_SOURCE_SIZE on an older OH image.

## Recommended approach

Check the running OS API before setting a newer ArkUI node attribute; a newer build SDK or exported dispatcher does not establish that its implementation exists.

## Implementation steps

1. Read `OH_GetSdkApiVersion()` before dispatching the attribute; skip `NODE_IMAGE_SOURCE_SIZE` below 21 in the affected implementation.
2. Keep the node-handle null check and preserve the renderer's ordinary image decode path when the size hint is unavailable.
3. Check setAttribute results on supported versions and retain width/height units as physical pixels.

## Conditions and alternatives

This is an optional decode-size hint. If an attribute is essential to correctness, declare an adequate minimum API or select another backend. Verify thresholds against the selected SDK headers.

## Common mistakes

Calling a newer attribute and relying on an error return can still jump through an unimplemented compatible-library function.

## Check it works

Exercise the image node below and at API 21, including resize and repeated image replacement. Confirm old images render without the hint and no null dispatch occurs.

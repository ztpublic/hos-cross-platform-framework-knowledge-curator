---
{
  "schema_version": 1,
  "id": "native-avsession-metadata-asset-id",
  "title": "Include stable asset identity in native AVSession metadata",
  "topic_key": "media.native-avsession-metadata-asset-id",
  "area": "media",
  "scope": "common",
  "frameworks": [],
  "summary": "Set a nonempty assetId before generating native metadata and check publication separately from each builder operation.",
  "when_to_use": "OH_AVSession_SetAVMetadata returns AV_SESSION_ERR_SERVICE_EXCEPTION (6600101) although individual title, artist or cover setters appear successful.",
  "applicability": "OH native AVSession/AVMetadata builder APIs as used by the reviewed OH Kotlin distribution. Relevant to PC media controls on products exposing those APIs; required fields and versions must be checked for the selected SDK.",
  "keywords": [
    "OH_AVMetadataBuilder_SetAssetId",
    "OH_AVSession_SetAVMetadata",
    "6600101",
    "AV_SESSION_ERR_SERVICE_EXCEPTION",
    "assetId",
    "cover art"
  ],
  "related": [
    "qt6-ohos-avsession-bridge",
    "kmp-ohos-native-avsession-host"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Include stable asset identity in native AVSession metadata

## When to use

OH_AVSession_SetAVMetadata returns AV_SESSION_ERR_SERVICE_EXCEPTION (6600101) although individual title, artist or cover setters appear successful.

## Recommended approach

Set a nonempty assetId before generating native metadata and check publication separately from each builder operation.

## Implementation steps

1. Create the metadata builder and supply a stable track identity with OH_AVMetadataBuilder_SetAssetId before generating the metadata object.
2. Add title, artist, duration and cover URI as supported, checking their return codes. A successful setter does not prove the session service accepts the complete record.
3. Generate metadata and check both the generation result and OH_AVSession_SetAVMetadata result. Log codes without assuming every service exception is a cover-decoding problem.
4. Retain builder-owned storage for the lifetime required by the selected API contract; replace and destroy metadata/builders in a defined order.
5. Update identity when the track changes even if two tracks have identical titles. Keep unavailable session/cover capabilities visible to the application.

## Conditions and alternatives

The missing-assetId failure is source-reported and the reviewed code sets it explicitly. This is a scoped diagnostic, not a claim that every 6600101 error has the same cause.

## Common mistakes

Do not rely on title as the only identity, diagnose the cover first without inspecting required fields, or report media-control integration from builder success alone.

## Check it works

Publish metadata with and without a valid identity under controlled conditions, switch between same-title tracks, and inspect the actual PC control card and native return codes.

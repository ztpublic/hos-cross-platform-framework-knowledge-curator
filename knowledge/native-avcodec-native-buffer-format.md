---
{
  "schema_version": 1,
  "id": "native-avcodec-native-buffer-format",
  "title": "Read the OH native buffer format instead of the AVCodec NV12 alias",
  "topic_key": "media.native-avcodec-native-buffer-format",
  "area": "media",
  "scope": "common",
  "frameworks": [],
  "summary": "Determine decoded pixel layout from OH_NativeBuffer_GetConfig, because AVCodec metadata can report NV12 for a 10-bit P010 buffer on some devices.",
  "when_to_use": "10-bit (Main10) hardware-decoded video shows corrupted, half-width or wrongly colored frames in a buffer-mode pipeline, while 8-bit plays correctly.",
  "applicability": "OH AVCodec buffer-mode decoding where the app copies or interprets frames itself, on devices that report the alias. Surface-mode playback that the system composes does not read these bytes. Display HDR capability is a separate question.",
  "keywords": [
    "P010",
    "NV12",
    "NV12_10bit",
    "Main10",
    "OH_NativeBuffer_GetConfig",
    "OH_AVBuffer_GetNativeBuffer",
    "stride",
    "10-bit video",
    "AVCodec"
  ],
  "related": [
    "hdr-output-surface-and-capability",
    "video-surface-backend-contract",
    "flutter-native-video-output-selection"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Read the OH native buffer format instead of the AVCodec NV12 alias

## When to use

10-bit (Main10) hardware-decoded video shows corrupted, half-width or wrongly colored frames in a buffer-mode pipeline, while 8-bit plays correctly.

## Recommended approach

Determine decoded pixel layout from OH_NativeBuffer_GetConfig, because AVCodec metadata can report NV12 for a 10-bit P010 buffer on some devices.

## Implementation steps

1. Get the native buffer for each output with OH_AVBuffer_GetNativeBuffer, and read format and stride through OH_NativeBuffer_GetConfig.
2. Map the native format explicitly: the reviewed code treats 24 as NV12 and 35 as P010, and uses metadata only when no native format is available. Return an error for other combinations.
3. Use the byte stride and two bytes per sample when copying P010 rows, and calculate luma and chroma row bytes with overflow checks.
4. Pass the real format downstream, such as AV_PIX_FMT_P010LE, so renderers and converters do not process 16-bit samples as 8-bit.
5. Validate each plane size against stride and height before copying, then log the format that was resolved.

## Conditions and alternatives

Constant values come from the reviewed bindings; confirm them against your SDK headers rather than copying numbers. If only 8-bit output is required, request a format conversion from the decoder where supported.

## Common mistakes

Branching only on the metadata pixel format. Assuming stride equals width. Silently treating an unknown format as NV12.

## Check it works

Play 8-bit and Main10 samples on each target device in buffer mode, compare colors and geometry with a reference frame, and confirm the log shows the resolved format. Include odd dimensions.

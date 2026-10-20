---
{
  "schema_version": 1,
  "id": "multimedia-timebase-api-migration",
  "title": "Preserve time-base semantics when replacing a removed media API",
  "topic_key": "media.multimedia-timebase-api-migration",
  "area": "media",
  "scope": "common",
  "frameworks": [],
  "summary": "Identify the time domain consumed by the caller and choose a replacement with equivalent units and meaning.",
  "when_to_use": "A native media dependency no longer declares the function used to read a codec time base.",
  "keywords": [
    "FFmpeg",
    "av_stream_get_codec_timebase",
    "AVStream",
    "time_base"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to native FFmpeg consumers across frameworks. A source report of a removed function does not establish a universal replacement or current version boundary.",
  "related": []
}
---

# Preserve time-base semantics when replacing a removed media API

## When to use

A native media dependency no longer declares the function used to read a codec time base.

## Recommended approach

Identify the time domain consumed by the caller and choose a replacement with equivalent units and meaning.

## Implementation steps

1. Inspect the installed media headers and the intended meaning of the removed value at each call site.
2. Distinguish stream timestamps from codec timing; use AVStream.time_base only when the operation requires the stream time base.
3. Convert timestamp units explicitly where a change of domain is required.
4. Version-scope compatibility code and preserve original behavior on still-supported library generations.

## Conditions and alternatives

Applies to native FFmpeg consumers across frameworks. A source report of a removed function does not establish a universal replacement or current version boundary.

## Common mistakes

Do not assume stream and codec time bases are interchangeable merely because a replacement compiles.

## Check it works

Test representative audio/video timestamp conversion, variable frame rate, seeking and reported durations against known media.

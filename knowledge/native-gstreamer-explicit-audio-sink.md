---
{
  "schema_version": 1,
  "id": "native-gstreamer-explicit-audio-sink",
  "title": "Select a real OH GStreamer audio output backend",
  "topic_key": "media.native-gstreamer-explicit-audio-sink",
  "area": "media",
  "scope": "common",
  "frameworks": [],
  "summary": "Choose an available OH audio sink explicitly and package its full decoding/output chain; do not rely on a desktop autodetect sink absent from the port.",
  "when_to_use": "GStreamer reports that autoaudiosink could not be created, or an OH player starts but cannot decode and output an MP3.",
  "applicability": "OH GStreamer ports lacking the autodetect module and providing an OH-compatible OpenSL ES sink. The reviewed source reports PC playback; no curation playback test ran.",
  "keywords": [
    "autoaudiosink",
    "openslessink",
    "GStreamer",
    "Q_OS_OHOS",
    "gst-libav",
    "MP3",
    "OpenSL ES"
  ],
  "related": [
    "native-gstreamer-plugin-registry",
    "native-dependency-closure",
    "audio-session-concurrency-policy"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Select a real OH GStreamer audio output backend

## When to use

GStreamer reports that autoaudiosink could not be created, or an OH player starts but cannot decode and output an MP3.

## Recommended approach

Choose an available OH audio sink explicitly and package its full decoding/output chain; do not rely on a desktop autodetect sink absent from the port.

## Implementation steps

1. Confirm the OH platform branch is compiled, then select `openslessink` when no user-selected output is configured and that sink is actually supplied.
2. Inspect element creation and negotiation independently: typefind/demux, decoder, conversion and audio sink each need a real implementation.
3. For the affected GStreamer port that disables built-in MP3 typefind, include its matching gst-libav/FFmpeg path. A sink-only fix cannot repair format recognition.
4. Verify the sink was ported to the product's supported OpenSL ES interfaces; unsupported control calls must be handled according to whether they are essential.

## Conditions and alternatives

The reviewed application contains the sink-selection branch. The README describes external gst-libav/OpenSL ES prerequisites whose implementation is not in that application snapshot; their full porting recipe is not independently validated here.

## Common mistakes

A no-op sink or compiling output stub is not audible playback. Defining a platform macro does not supply absent plugin binaries.

## Check it works

Test element availability, an ID3-tagged MP3, audible output, pause/resume, device routing and window background/foreground transitions.

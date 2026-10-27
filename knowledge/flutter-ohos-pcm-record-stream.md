---
{
  "schema_version": 1,
  "id": "flutter-ohos-pcm-record-stream",
  "title": "Deliver OH microphone PCM through a Flutter record stream",
  "topic_key": "media.flutter-ohos-pcm-record-stream",
  "area": "media",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "summary": "Implement streaming with AudioCapturer readData callbacks and a per-recorder EventChannel, copying each callback buffer before delivery and awaiting capture startup.",
  "when_to_use": "Flutter file recording works on OH but startStream returns no bytes, reuses corrupted buffers, or reports success before AudioCapturer starts.",
  "applicability": "Flutter OH record adapters using raw PCM16 capture. On PC, microphone permission, actual input routes, background policy and device-supported formats remain separate checks.",
  "keywords": [
    "startStream",
    "AudioCapturer",
    "readData",
    "pcm16bits",
    "SAMPLE_FORMAT_S16LE",
    "ENCODING_TYPE_RAW",
    "EventChannel",
    "microphone"
  ],
  "related": [
    "audio-session-concurrency-policy",
    "native-event-loop-handle-teardown",
    "plugin-graph-source-and-api"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Deliver OH microphone PCM through a Flutter record stream

## When to use

Flutter file recording works on OH but startStream returns no bytes, reuses corrupted buffers, or reports success before AudioCapturer starts.

## Recommended approach

Implement streaming with AudioCapturer readData callbacks and a per-recorder EventChannel, copying each callback buffer before delivery and awaiting capture startup.

## Implementation steps

1. Reject unsupported stream encoders explicitly. Build AudioCapturerOptions with supported rate/channel values, `SAMPLE_FORMAT_S16LE`, `ENCODING_TYPE_RAW` and microphone source.
2. Create the capturer, register readData before starting, then acknowledge the MethodChannel request only after start succeeds.
3. Copy each nonempty buffer with `new Uint8Array(buffer).slice()` and publish it to the recorder-specific bytes EventChannel. Keep recording state on its separate event channel.
4. For logical pause, discard callbacks while paused; the hardware may remain active. On stop/dispose, disable delivery, remove the same readData callback, stop and release before reusing the capturer.
5. Serialize mode switches and settle failed startup with cleanup. Do not infer streaming encoder support from an API that unconditionally reports encoder availability.

## Conditions and alternatives

Raw PCM is not AAC/WAV. Consumer framing and backpressure must be explicit; the source forwards chunks without a bounded queue. Source cleanup serialization is useful but does not make all plugin lifecycle races fixed.

## Common mistakes

Retaining the callback's native buffer or returning success immediately after creating the recorder can produce corrupted chunks or silent startup failure.

## Check it works

Check first nonempty PCM bytes, sample rate/channel framing, denied permission, unsupported encoder, rapid stop/start, pause/resume and stream-to-file switching. Monitor memory with a slow consumer.

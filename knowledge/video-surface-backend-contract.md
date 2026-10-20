---
{
  "schema_version": 1,
  "id": "video-surface-backend-contract",
  "topic_key": "media.video-surface-backend-contract",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "title": "Fix video black screens without bypassing the backend contract",
  "area": "media",
  "summary": "Keep the player widget, native surface and renderer lifecycle aligned; reduce layout/transforms only as a targeted diagnostic or workaround.",
  "when_to_use": "Audio plays but no picture appears, or a video surface breaks when wrapped, resized or replaced with a raw Texture widget.",
  "keywords": [
    "Texture",
    "video_player",
    "media_kit",
    "FVP",
    "MDK",
    "Erika",
    "OHNativeWindow",
    "黑屏",
    "video black screen",
    "audio without video",
    "Texture issue",
    "有声音没画面"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Flutter OH with the selected embedding/plugin versions; PC behavior requires the stated acceptance checks.",
  "related": [
    "flutter-native-video-output-selection",
    "flutter-player-async-teardown",
    "flutter-player-surface-reconstruction",
    "native-event-loop-handle-teardown"
  ]
}
---

# Fix video black screens without bypassing the backend contract

## When to use

Audio plays but no picture appears, or a video surface breaks when wrapped, resized or replaced with a raw Texture widget.

## Recommended approach

Keep the player widget, native surface and renderer lifecycle aligned; reduce layout/transforms only as a targeted diagnostic or workaround.

## Implementation steps

1. Confirm the selected OH backend really implements the renderer and native-window/texture bridge your app needs.
2. Start with the plugin's supported video widget and a simple bounded layout. Separate source/decoder failures from surface-composition failures.
3. If frames disappear under transforms or fitting wrappers, remove those layers one at a time and identify the smallest failing composition.
4. Avoid replacing the plugin widget with a raw Texture unless you also preserve all required surface registration, sizing and lifecycle behavior.
5. On backend changes or view recreation, dispose old textures/controllers and attach events to the new instance.

## Conditions and alternatives

Different media backends have different OH support and integration requirements. A backend excluded by one dependency graph may be viable through another supported implementation.

See [Await native teardown before replacing a backend](flutter-player-async-teardown.md) for this independent contract.

See [Serialize surface reconstruction with source loading](flutter-player-surface-reconstruction.md) for this independent contract.

See [Branch custom renderers by the active output type](flutter-native-video-output-selection.md) for this independent contract.

## Common mistakes

A selected hardware-decoder label is not proof of actual hardware decoding. Initialization and audio output are not proof of rendered video.

For custom native conversion/player handles, also preserve the [event-loop teardown contract](native-event-loop-handle-teardown.md): removing callbacks and destroying a handle are separate operations.

## Check it works

Test first frame, multiple codecs, resizing/orientation, route disposal/recreation and backend switching. Use native decoder evidence for hardware-acceleration claims.

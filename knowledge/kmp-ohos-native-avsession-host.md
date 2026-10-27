---
{
  "schema_version": 1,
  "id": "kmp-ohos-native-avsession-host",
  "title": "Connect KMP playback to an OH native media session",
  "topic_key": "media.kmp-ohos-native-avsession-host",
  "area": "media",
  "scope": "framework-specific",
  "frameworks": [
    "kotlin-multiplatform"
  ],
  "summary": "Use the OH native AVSession bindings with an explicit callback owner, publish player state, and coordinate audio background tasks through the owning ArkTS ability.",
  "when_to_use": "A Kotlin/Native OH player produces audio but the system media card has no reliable metadata, controls or background-playback ownership.",
  "applicability": "CPF Kotlin distribution exposing platform.AVSessionKit.OHAVSession and a compatible OH product/API. Source host is phone-oriented; PC minimize/background distinctions and media controls require separate validation.",
  "keywords": [
    "StableRef",
    "staticCFunction",
    "OHAVSession",
    "RegisterCommandCallback",
    "RegisterSeekCallback",
    "AUDIO_PLAYBACK",
    "9800005",
    "KEEP_BACKGROUND_RUNNING"
  ],
  "related": [
    "native-avsession-metadata-asset-id",
    "continuous-background-task-contract",
    "qt6-ohos-avsession-bridge"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Connect KMP playback to an OH native media session

## When to use

A Kotlin/Native OH player produces audio but the system media card has no reliable metadata, controls or background-playback ownership.

## Recommended approach

Use the OH native AVSession bindings with an explicit callback owner, publish player state, and coordinate audio background tasks through the owning ArkTS ability.

## Implementation steps

1. Verify the actual Kotlin distribution includes the OH AVSession cinterop and link the target native libraries. Create/activate an audio session with the running bundle and declared ability identity.
2. Register top-level staticCFunction callbacks with a retained StableRef as userData. Forward play/pause/next/previous/seek into the application playback service rather than creating a second player.
3. Publish metadata with assetId, playback state and position using the native API's time units. Check every registration and publication result.
4. When legitimate playback becomes active, notify the ArkTS host. Request an audio-playback continuous task only with the required permission, ability backgroundModes and eligible active session; stop it on pause/end as appropriate.
5. Serialize playback and background-task transitions so overlapping promises cannot publish stale state. Apply a generation when the owner changes.
6. On teardown cancel/join the publisher, stop background work, unregister callbacks, ensure callbacks are quiescent, destroy session and metadata, then dispose the StableRef. Callback thread assumptions require explicit verification.

## Conditions and alternatives

The source uses a Main-dispatcher polling publisher and retained C callback state. It does not establish callback-thread safety or fully cancel that publisher in release(); the cleanup ordering above is defensive guidance, not a tested patch. PC minimization may not produce the same background transition as a phone.

## Common mistakes

Do not dispose StableRef while callbacks can still execute, infer background eligibility from playing audio alone, or assume all multimedia APIs in an OH fork are available in every product SDK.

## Check it works

On the intended PC, verify external play/pause/seek, same-title track changes, foreground/background transitions and teardown with commands in flight. Confirm session/task cleanup and no callbacks to released Kotlin state.

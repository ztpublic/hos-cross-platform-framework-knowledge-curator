---
{
  "schema_version": 2,
  "id": "audio-session-concurrency-policy",
  "title": "Apply the OH audio-session policy for simultaneous playback",
  "topic_key": "media.audio-session-concurrency-policy",
  "area": "media",
  "keywords": ["audio_session", "audio focus", "mixWithOthers", "ohosAudioConcurrencyMode", "AudioConcurrencyMode", "concurrencyMixWithOthers", "concurrencyPauseOthers", "activateAudioSession", "deactivateAudioSession", "音频焦点", "同时播放", "音频混音"],
  "summary": "Pass the intended OH concurrency policy when activating the native audio session, and verify activation, interruption and deactivation separately.",
  "when_to_use": "An allow-simultaneous-playback setting works on Android/iOS but still pauses another application's audio on OH, or audio focus appears retained after playback stops.",
  "status": "scoped"
}
---

# Apply the OH audio-session policy for simultaneous playback

## When to use

An allow-simultaneous-playback setting works on Android/iOS but still pauses another application's audio on OH, or audio focus appears retained after playback stops.

## Recommended approach

Map the preference to the selected OH plugin's native session concurrency policy at activation. Keep that policy separate from interruption handling, renderer volume and background execution. Verify activation and release through the actual backend.

## Implementation steps

1. Resolve an OH implementation of `audio_session` whose Dart API, platform bridge and native code agree. Some forks expose `ohosAudioConcurrencyMode` on `AudioSession.setActive`; this is a fork-specific API, not an option to assume in every package version.
2. Put all playback activation paths through one session owner. Initialize the session before use, apply the current preference on activation, and await/report activation errors before claiming the session is active. Decide explicitly whether playback may proceed if activation fails.
3. With a compatible fork, request `AudioConcurrencyMode.concurrencyMixWithOthers` for simultaneous playback. Select the non-mixing policy deliberately when the setting is off. Inspect the exact backend's default: an omitted option can select `concurrencyPauseOthers` rather than leave other applications unchanged.
4. Trace the channel to the native `AudioSessionStrategy.concurrencyMode` passed to `activateAudioSession`. An iOS category option, Android focus configuration, or ignoring a Dart pause event does not set this OH policy. Avoid passing arbitrary enum ordinals outside the matched bridge contract.
5. Retain interruption, route-change and manual-pause handling. Resume only playback that the interruption paused and that the user has not subsequently stopped; requesting mixing does not establish immunity to all system interruptions. Release owned event subscriptions on teardown.
6. When changing the preference during playback, follow the backend's supported reactivation procedure and serialize it with start/stop. On stop or teardown, verify the native release path. A `setActive(false)` call that only removes listeners or updates a flag does not establish a native `deactivateAudioSession` call; a successful channel result alone is insufficient.

## Conditions and alternatives

Use the policy supported by the selected SDK, device and audio backend. If the needed option or release behavior is missing, choose or repair a compatible adapter and expose the limitation until verified. Keep the [plugin dependency graph](plugin-graph-source-and-api.md) consistent across callers and native packages.

Audio-session ownership does not itself grant [continuous background execution](continuous-background-task-contract.md), and changing volume is not equivalent to changing focus policy.

## Common mistakes

Suppressing a local pause callback cannot undo another application's earlier interruption. An async initializer called from a constructor does not make later session access wait automatically. A plugin can acknowledge listener registration while exposing incomplete activation or release semantics.

## Check it works

Use two applications on the target device. Test starting either one first, mixing enabled/disabled, changing the preference while playing, activation failure, interruption followed by manual pause, headphones disconnecting, and stop/dispose/restart. Observe the native session state and both audio outputs; record release separately from Dart callback completion.

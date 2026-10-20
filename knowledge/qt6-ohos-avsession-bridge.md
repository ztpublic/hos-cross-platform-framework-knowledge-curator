---
{
  "schema_version": 1,
  "id": "qt6-ohos-avsession-bridge",
  "title": "Connect a Qt media player to OH AVSession",
  "topic_key": "media.qt6-ohos-avsession-bridge",
  "area": "media",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Create an audio AVSession, publish metadata, playback state and position, handle commands on the player's Qt thread, and unregister callbacks before destroying the session.",
  "when_to_use": "Media keys, the system media notification or lock-screen controls do nothing for a Qt player on OH PC, or playback stops when the app loses foreground.",
  "keywords": [
    "AVSession",
    "OH_AVSession",
    "media session",
    "metadata",
    "playback state",
    "media keys",
    "background audio",
    "KEEP_BACKGROUND_RUNNING",
    "Qt 6"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Qt 6 OH PC media players with an audio or video session. AVSession availability, callback behavior and background policy depend on the API level and product.",
  "related": [
    "qt6-ohos-native-video-texture",
    "continuous-background-task-contract",
    "audio-session-concurrency-policy"
  ]
}
---

# Connect a Qt media player to OH AVSession

## When to use

Media keys, the system media notification or lock-screen controls do nothing for a Qt player on OH PC, or playback stops when the app loses foreground.

## Recommended approach

Create an audio AVSession, publish metadata, playback state and position, and handle session commands on the player's Qt thread. Declare background audio and tear the session down with all callbacks unregistered.

## Implementation steps

1. Link the AVSession native library and create a session for audio playback with the app name, bundle name and ability name.
2. Register callbacks for play, pause, stop, previous, next, absolute seek, forward and rewind. Session callbacks do not run on the Qt GUI thread; marshal each one with a queued invocation before touching the player.
3. Publish metadata (asset id/URI, title, duration) when the input changes, and update preparing/playing/paused/completed states from the player's status.
4. Publish position at a bounded rate (for example once per second); do not spam the session on every position tick.
5. Activate the session when media becomes current and deactivate/destroy it at teardown. Unregister every callback before destroy to avoid the system calling a dead object.
6. Declare `backgroundModes: ["audioPlayback"]` and the `KEEP_BACKGROUND_RUNNING` permission, plus any required ArkTS registrations, so audio continues when the app is not foreground.
7. Verify media keys, notification actions, seek units (milliseconds vs seconds) and teardown.

## Conditions and alternatives

AVSession availability and callback behavior depend on the OH API level. A video-only app may use a different session type. Background playback is a product decision and may require store review.

## Common mistakes

- Calling player APIs directly from an AVSession callback thread.
- Forgetting to unregister callbacks before destroying the session.
- Publishing position on every tick and flooding the session.
- Treating seconds and milliseconds as interchangeable in seek callbacks.
- Declaring background audio without the matching permission or ArkTS registration.

## Check it works

Start playback and confirm the system media card shows the title/duration and the correct state. Use media keys and notification actions for play, pause, stop, next/previous and seek. Background the app and confirm audio continues, then stop and confirm the session disappears without a callback-after-free crash.

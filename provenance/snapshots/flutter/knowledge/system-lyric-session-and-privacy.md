---
{
  "id": "system-lyric-session-and-privacy",
  "title": "Keep system lyrics current and clear them in privacy mode",
  "topic_key": "media.system-lyric-session-and-privacy",
  "area": "media",
  "keywords": [
    "system lyrics",
    "audio_service",
    "session epoch",
    "privacy",
    "系统歌词",
    "隐私",
    "stale system lyrics",
    "privacy lyrics",
    "系统歌词残留",
    "隐私模式"
  ],
  "summary": "Own synchronization at application/session scope, tag writes by native session, and actively clear the native surface when content is no longer allowed.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "OS-facing lyrics or media text survive track changes, reappear from queued writes, or remain visible after privacy mode is enabled."
}
---

# Keep system lyrics current and clear them in privacy mode

## When to use

OS-facing lyrics or media text survive track changes, reappear from queued writes, or remain visible after privacy mode is enabled.

## Recommended approach

Own synchronization at application/session scope, tag writes by native session, and actively clear the native surface when content is no longer allowed.

## Implementation steps

1. Query supported lyric/visibility capabilities and establish a native-session token.
2. Serialize or coalesce updates so an older write cannot overtake the newest track state.
3. Tag cached and queued writes with a session epoch; invalidate them when the native session or track ownership changes.
4. On privacy activation, hide the native lyric surface before clearing its metadata, then prevent queued content from repopulating it.
5. Clear state for missing tracks and reconcile native visibility callbacks with the desired app state.
6. Dispose listeners and invalidate outstanding work when the owning session ends.

## Conditions and alternatives

This requires a plugin/native service that actually implements the lyric surface. Keep the Flutter UI usable when the capability is absent.

## Common mistakes

Hiding a Flutter widget does not clear system metadata. A channel reply alone does not prove every OS-facing cache has cleared.

## Check it works

Test rapid track switches, native session replacement, queued updates, privacy toggles, errors and restart while observing the real system surface.

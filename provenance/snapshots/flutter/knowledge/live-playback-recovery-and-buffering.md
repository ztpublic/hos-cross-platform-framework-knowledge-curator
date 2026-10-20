---
{
  "schema_version": 2,
  "id": "live-playback-recovery-and-buffering",
  "title": "Recover OH live playback without reacting to weak signals",
  "topic_key": "media.live-playback-recovery-and-buffering",
  "area": "media",
  "keywords": [
    "AVPlayer",
    "HTTP-FLV",
    "buffering",
    "low latency",
    "setPlaybackStrategy",
    "line failover",
    "heartbeat",
    "直播卡顿",
    "缓冲",
    "自动切线路",
    "低延迟"
  ],
  "summary": "Classify playback evidence, bound recovery and use generation-safe optional line/buffer policies with a stable system-default fallback.",
  "when_to_use": "Live playback switches lines unnecessarily, stalls after buffer tuning, or reports recovery before a meaningful native confirmation.",
  "status": "scoped"
}
---

# Recover OH live playback without reacting to weak signals

## When to use

An OH live player repeatedly switches lines after brief buffering pulses, starts too slowly after buffer tuning, or reports successful recovery before playback returns.

## Recommended approach

Build recovery from classified, generation-bound evidence and a bounded retry policy. Keep system buffering defaults as the stable route; treat low latency and automatic line selection as optional policies with explicit fallback.

## Implementation steps

1. Separate first-frame readiness, timeline advancement, native clock events, cache depth, buffering edges and native errors. Cache updates are not clock liveness; missing telemetry is an unknown observation rather than an automatic stall.
2. Track buffering intervals within a sliding window. Ignore an explicit startup warmup, measure continuous and accumulated duration, and evaluate both periodic samples and the end edge. Clip intervals to the window and clear evidence after stable playback or source changes.
3. Serialize recovery with a session retry budget and cooldown. Reopening, changing line and lowering quality compete for the same owner; reserve a decision before awaiting and reject old completions.
4. For optional line probing, capture room, source-request, player and manual-selection revisions. Compare candidates in the same protocol family and apply a result only while every captured identity remains current. A successful TCP probe establishes endpoint reachability, not stream authorization, decoding or visible playback.
5. Create a pending recovery record before an asynchronous rebuild can report back. Preserve requested, initialized, clock-alive and first-frame-confirmed outcomes separately. Expiry records an unconfirmed attempt with unknown recovery duration; it must not manufacture success or zero latency.
6. Apply the chosen AVPlayer policy at the appropriate source/prepare stage. Handle direct URLs and media sources with headers explicitly; preserve headers when falling back. Gate an experimental profile by the pinned API, supported protocol and user choice, then disable it for the current session on repeated failures.
7. Expose the effective native policy and its fallback reason. Do not label the persisted requested preference as applied. Leave startup-buffer and catch-up settings at system defaults unless measurements justify overriding them.

## Conditions and alternatives

HTTP-FLV sources can expose different clock/cache behavior from VOD or HLS. Thresholds and an experimental one-second preferred buffer are application policies, not universal recommendations. API availability does not establish effective low latency on every device or endpoint. If no meaningful clock observation is available, retain independent first-frame/buffering/error checks and report limited observability.

## Common mistakes

Counting two short pulses as a failed line can interrupt healthy playback. A heartbeat proves receipt of a clock event, not a visible frame; initialization alone also cannot confirm picture recovery. Fixed startup buffering can exceed the application's initialization deadline. A late probe must never override the user's newer selection.

## Check it works

Exercise brief pulses, a long start/end-only stall, unavailable telemetry, paused playback, delayed old events, manual changes during probing, denied/slow endpoints, strategy rejection and rapid source replacement. Verify a finite recovery budget, correct units for percent versus milliseconds, preserved request headers and separately measured first-frame recovery.

See [native playback observations](native-playback-state-observation.md) for the underlying readiness signals.

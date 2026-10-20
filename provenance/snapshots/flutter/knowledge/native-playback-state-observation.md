---
{
  "id": "native-playback-state-observation",
  "title": "Detect actual playback progress and reject stale player events",
  "topic_key": "media.native-playback-state-observation",
  "area": "media",
  "keywords": [
    "first frame",
    "START_RENDER_FRAME",
    "generation",
    "playback",
    "volume",
    "首帧",
    "completion",
    "first frame timeout",
    "stale playback event",
    "首帧超时"
  ],
  "summary": "Use native observations correlated with the current player generation; keep requested state separate from confirmed state.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "The player reports initialized/playing but has not rendered, or late completion/events affect a newly selected stream."
}
---

# Detect actual playback progress and reject stale player events

## When to use

The player reports initialized/playing but has not rendered, or late completion/events affect a newly selected stream.

## Recommended approach

Use native observations correlated with the current player generation; keep requested state separate from confirmed state.

## Implementation steps

1. Add or consume a native first-rendered-frame event when the backend supports one. Carry texture/player/session identity with the event.
2. Increment a generation when changing source or recreating a player. Ignore callbacks belonging to previous generations.
3. Bound first-frame waiting and classify timeout separately from initialization failure. Define bounded retry/failover behavior rather than looping indefinitely.
4. When completion leaves a backend with stale playing intent, use a guarded reset/load/play sequence appropriate to that backend and keep it idempotent.
5. For mutable system state such as volume, reconcile the observed value; do not skip a native command solely because it equals a cached request.
6. Route in-app controls and system media-session commands through the same handler. Distinguish resuming a paused source from loading a newly selected source. Publish metadata and loading state when changing tracks, then derive playing/position/control state from the current backend observations.

## Conditions and alternatives

Use a pause/reset step only when the selected backend's state machine requires it. Deadlines and failover ordering should reflect the product's latency/reliability needs.

### Preserve the meaning and units of native telemetry

Keep percent, cached duration and timeline position as different typed fields. A buffering percent must not become a milliseconds range. If the backend returns an unavailable position, a validated clock-event fallback may improve observation, but clear it whenever the source generation changes. Drop periodic cache/clock telemetry while no event listener is attached, or keep only a bounded latest value; do not grow an unbounded detached event queue.

Record initialized, clock-alive and rendered-first-frame confirmations separately in recovery metrics. A controller rebuild returning synchronously is only a request; missing telemetry should be diagnosed without manufacturing a stall or a rendered-frame success.

When replacing a player, distinguish the generation that tells widgets to bind a new controller from the later event that confirms its source has opened. Reattaching subtitle tracks before opening can be undone by the load operation. Neither event alone proves a rendered first frame. Preserve a valid position snapshot across reconstruction: a newly opened player's temporary zero position must not overwrite saved progress as if it were an intentional seek.

## Common mistakes

Position advancement or isPlaying may not prove visible video. A nonempty video rectangle can reflect known dimensions or a retained surface; treat it as layout readiness unless the backend explicitly guarantees it follows rendering. A first-frame event from the old stream must not unblock the new one.

## Check it works

Test delayed/missing first frame, rapid source switching, replay after completion, rejected commands and external changes to system state.

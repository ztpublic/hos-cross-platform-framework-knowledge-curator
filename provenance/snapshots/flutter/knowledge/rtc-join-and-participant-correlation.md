---
{
  "schema_version": 2,
  "id": "rtc-join-and-participant-correlation",
  "title": "Complete native RTC joins and bind the intended remote participant",
  "topic_key": "media.rtc-join-and-participant-correlation",
  "area": "media",
  "keywords": ["RTC", "DingRTC", "joinChannel", "remoteUserId", "participant", "surfaceId", "MethodResult", "视频通话", "远端黑屏", "入会超时", "旧入会回调"],
  "summary": "Separate join acceptance, confirmed membership and media readiness; correlate callbacks and surfaces with the active call and authorized participant.",
  "when_to_use": "An OH RTC bridge reports joined too early, displays a black remote view, or lets an old callback complete a later call.",
  "status": "scoped"
}
---

# Complete native RTC joins and bind the intended remote participant

## When to use

An OH RTC bridge reports joined too early, displays a black remote view, or lets an old callback complete a later call.

## Recommended approach

Keep call identity, native join completion, participant identity and surface lifetime explicit. A successful synchronous SDK return usually accepts a request; use the selected SDK's asynchronous membership result to settle the Dart join operation.

## Implementation steps

1. Validate the selected backend's credentials, channel, local user, requested peer and media permissions before allocating a pending result. Keep backend-specific token formats behind the app's RTC adapter; generate authorization on the appropriate server, not by embedding server secrets in the client.
2. Create a call generation and record the pending join's channel, local user and engine identity. Serialize joins or explicitly leave/release the old operation before replacing it. Associate SDK listeners and timeout handlers with that generation.
3. Settle the channel result once on matching asynchronous success, synchronous rejection, SDK error, timeout, leave or dispose. Clear pending state before replying. Validation exceptions must also clear it; otherwise a later callback can reply to an already failed request.
4. Track the expected peer separately from the peer reported by native online/track events. Resolve any backend-specific identity mapping through the authorized call/signaling context. Do not fix a mismatch by automatically adopting whichever participant emits the last event. Represent multiple participants with per-peer bindings when the product supports them.
5. Bind a remote render surface only when the current call, allowed participant, track and surface are available. Handle both orders: peer first, surface later; surface first, peer later. Track membership, subscription and first rendered frame as separate states. See [native playback observations](native-playback-state-observation.md).
6. On peer exit, surface disposal, leave or engine release, unbind using that backend's documented operation and clear the matching identity. Invalidate deferred callbacks and cancel deadlines. A late event from the old engine must not settle a new join or attach to its view.

## Conditions and alternatives

An SDK replacement can change token fields, callback names and video-view ownership together. Preserve the app-facing adapter but adapt these contracts explicitly. Use [native registration checks](native-registration-and-handler-coverage.md) and [dependency completeness](native-dependency-completeness.md) to establish that the selected OH SDK actually reaches the app.

## Common mistakes

Joining successfully does not establish remote media reception. Subscribing to every participant is not equivalent to selecting the intended peer. Reusing one pending-result field without callback correlation lets an old success complete a newer request.

## Check it works

Test invalid credentials, rejected join, timeout, rapid join replacement, leave/dispose while joining, a delayed old success, unexpected peers, peer reconnection and surface recreation. Confirm one terminal join result and correct peer/view binding.

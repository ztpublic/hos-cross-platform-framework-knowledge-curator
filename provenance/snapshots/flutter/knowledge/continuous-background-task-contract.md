---
{
  "id": "continuous-background-task-contract",
  "title": "Keep legitimate playback or transfers running in the background",
  "topic_key": "lifecycle.continuous-background-task-contract",
  "area": "lifecycle",
  "keywords": [
    "backgroundTaskManager",
    "WantAgent",
    "dataTransfer",
    "audioPlayback",
    "后台任务",
    "AVSession",
    "background download stops",
    "audio stops in background",
    "SSH keepalive",
    "后台被暂停"
  ],
  "summary": "Pair the active user-visible operation with an appropriate native background-task lifecycle, including declarations, authorization, session/notification behavior and cleanup.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "Playback, downloads or an SSH-backed transfer stops when the app leaves the foreground, despite Dart timers or keepalive packets."
}
---

# Keep legitimate playback or transfers running in the background

## When to use

Playback, downloads or an SSH-backed transfer stops when the app leaves the foreground, despite Dart timers or keepalive packets.

## Recommended approach

Pair the active user-visible operation with an appropriate native background-task lifecycle, including declarations, authorization, session/notification behavior and cleanup.

## Implementation steps

1. Identify the actual activity and choose its supported background mode, such as audio playback or data transfer. Do not select a mode unrelated to the feature merely to keep the process alive.
2. Configure the ability's background mode and required permissions; supply the native context and WantAgent/session information expected by the API.
3. Start the task when the activity truly begins. Await the native start result before telling Dart it was accepted.
4. For media, maintain playback metadata and system play/pause/interruption handling as well as the execution request.
5. Stop the task when the owning queue/session ends, fails or is disposed. Make repeated start/stop calls safe and remove listeners.
6. Reconcile actual native state after lifecycle transitions rather than trusting a stale Dart flag.

## Conditions and alternatives

Eligibility and behavior depend on the chosen mode, SDK and product. A foreground service on another OS has no automatic OH equivalent.

## Common mistakes

Permissions alone do not implement a background task. Neither a heartbeat nor an early channel success proves OS acceptance. A location subscription is not a general-purpose replacement for a transfer task.

## Check it works

Test native denial, foreground/background transitions, interruptions, queue completion, disconnect and teardown. Observe actual task/session state and resource cleanup.

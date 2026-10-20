---
{
  "schema_version": 1,
  "id": "arkweb-synthetic-input-exclusions",
  "title": "Keep recovery controls outside synthetic game input capture",
  "topic_key": "web.arkweb-synthetic-input-exclusions",
  "area": "web",
  "scope": "common",
  "frameworks": [],
  "summary": "Exclude native HTML controls and recovery overlays from game-input capture and bind bridge work to the active session.",
  "when_to_use": "A synthetic game-touch adapter prevents recovery buttons from receiving pointer events.",
  "keywords": [
    "ArkWeb",
    "GameAbility",
    "synthetic origin",
    "JavaScript proxy",
    "document start",
    "本地资源",
    "local web game",
    "ArkWeb resource handler",
    "document start script",
    "本地网页容器"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "This is useful for a specialized runtime; a standard OH-capable WebView adapter is simpler when it already meets the feature contract.",
  "related": [
    "native-arkweb-game-host"
  ]
}
---

# Keep recovery controls outside synthetic game input capture

## When to use

A synthetic game-touch adapter prevents recovery buttons from receiving pointer events.

## Recommended approach

Exclude native HTML controls and recovery overlays from game-input capture and bind bridge work to the active session.

## Implementation steps

When a JavaScript touch adapter synthesizes game events, explicitly let native HTML controls and recovery overlays receive their ordinary pointer sequence. Classify the event target/ancestors at the adapter boundary; do not swallow a button needed to recover from a failed game session. Keep the bridge operation set and sandbox path policy synchronized with game-version compatibility rather than accepting every desktop command.

Own pending selections and file transfers in the native bridge session and reject outstanding JavaScript requests on native destruction. Check cleanup errors per resource so one failure does not prevent the remaining transfers/listeners from being released. Test game gestures, recovery buttons, directory/file selection and teardown while a write is pending.

## Conditions and alternatives

This is useful for a specialized runtime; a standard OH-capable WebView adapter is simpler when it already meets the feature contract.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Exercise game gestures and recovery controls together; test pointer targets inside nested button elements and teardown while selection/write requests are pending.

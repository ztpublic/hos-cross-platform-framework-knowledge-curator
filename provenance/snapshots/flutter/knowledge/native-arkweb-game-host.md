---
{
  "id": "native-arkweb-game-host",
  "title": "Host local web content in a native ArkWeb ability",
  "topic_key": "web.native-arkweb-game-host",
  "area": "web",
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
  "summary": "Use a deliberate native-host boundary with a scoped local origin, a typed session and explicit launch/return lifecycle.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "A Flutter app launches a web game or other specialized local web runtime that needs native window, resource and bridge control."
}
---

# Host local web content in a native ArkWeb ability

## When to use

A Flutter app launches a web game or other specialized local web runtime that needs native window, resource and bridge control.

## Recommended approach

Use a deliberate native-host boundary with a scoped local origin, a typed session and explicit launch/return lifecycle.

## Implementation steps

1. Define the session passed from Flutter: schema version, entry resource, content root, allowed remote hosts and required feature options.
2. Let the native ability own ArkWeb initialization, window behavior and return-to-launcher handling.
3. Serve local resources through a controlled origin/resource handler. Restrict methods, normalize names, enforce containment, and reject traversal and disallowed symlinks.
4. Scope JS proxy permissions and document-start injection to the intended origin.
5. Ensure declarative initial content does not overwrite controller-driven navigation. Check the injection API and matching-rule grammar of the selected SDK.
6. Close the previous session and its resources when replacing content.

## Conditions and alternatives

This is useful for a specialized runtime; a standard OH-capable Flutter WebView is simpler when it already meets the feature contract.

### Keep recovery controls outside synthetic game input capture

When a JavaScript touch adapter synthesizes game events, explicitly let native HTML controls and recovery overlays receive their ordinary pointer sequence. Classify the event target/ancestors at the adapter boundary; do not swallow a button needed to recover from a failed game session. Keep the bridge operation set and sandbox path policy synchronized with game-version compatibility rather than accepting every desktop command.

Own pending selections and file transfers in the native bridge session and reject outstanding JavaScript requests on native destruction. Check cleanup errors per resource so one failure does not prevent the remaining transfers/listeners from being released. Test game gestures, recovery buttons, directory/file selection and teardown while a write is pending.

## Common mistakes

A URL-looking glob may not match a native injection-rule format. Do not expose powerful native methods to arbitrary origins or confuse local-content containment with trust in remote scripts.

## Check it works

Test earliest script calls, origin restrictions, encoded traversal, symlinks, allowed/disallowed remote requests, session replacement and ability return flow.

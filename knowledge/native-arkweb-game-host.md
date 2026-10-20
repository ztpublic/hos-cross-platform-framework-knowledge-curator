---
{
  "schema_version": 1,
  "id": "native-arkweb-game-host",
  "topic_key": "web.native-arkweb-game-host",
  "scope": "common",
  "frameworks": [],
  "title": "Host local web content in a native ArkWeb ability",
  "area": "web",
  "summary": "Use a deliberate native-host boundary with a scoped local origin, a typed session and explicit launch/return lifecycle.",
  "when_to_use": "An application launches a web game or other specialized local web runtime that needs native window, resource and bridge control.",
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
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": [
    "arkweb-synthetic-input-exclusions"
  ]
}
---

# Host local web content in a native ArkWeb ability

## When to use

An application launches a web game or other specialized local web runtime that needs native window, resource and bridge control.

## Recommended approach

Use a deliberate native-host boundary with a scoped local origin, a typed session and explicit launch/return lifecycle.

## Implementation steps

1. Define the session passed from the application runtime: schema version, entry resource, content root, allowed remote hosts and required feature options.
2. Let the native ability own ArkWeb initialization, window behavior and return-to-launcher handling.
3. Serve local resources through a controlled origin/resource handler. Restrict methods, normalize names, enforce containment, and reject traversal and disallowed symlinks.
4. Scope JS proxy permissions and document-start injection to the intended origin.
5. Ensure declarative initial content does not overwrite controller-driven navigation. Check the injection API and matching-rule grammar of the selected SDK.
6. Close the previous session and its resources when replacing content.

## Conditions and alternatives

This is useful for a specialized runtime; a standard OH-capable WebView adapter is simpler when it already meets the feature contract.

See [Keep recovery controls outside synthetic game input capture](arkweb-synthetic-input-exclusions.md) for this independent contract.

## Common mistakes

A URL-looking glob may not match a native injection-rule format. Do not expose powerful native methods to arbitrary origins or confuse local-content containment with trust in remote scripts.

## Check it works

Test earliest script calls, origin restrictions, encoded traversal, symlinks, allowed/disallowed remote requests, session replacement and ability return flow.

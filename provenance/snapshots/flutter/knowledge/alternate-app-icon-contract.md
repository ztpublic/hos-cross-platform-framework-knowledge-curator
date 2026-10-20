---
{
  "schema_version": 2,
  "id": "alternate-app-icon-contract",
  "title": "Offer alternate app icons with an honest native capability contract",
  "topic_key": "integration.alternate-app-icon-contract",
  "area": "integration",
  "keywords": [
    "alternateIcons",
    "bundleManager",
    "getAlternateIcons",
    "setAlternateIcon",
    "launcher icon",
    "API 26",
    "动态图标",
    "备用图标",
    "应用图标"
  ],
  "summary": "Package named alternate icons, gate the native API, serialize changes and reconcile the OS-enabled choice before reporting effective state.",
  "when_to_use": "Selectable launcher icons must work conditionally across supported OH device/API versions without false success or overlapping changes.",
  "status": "scoped"
}
---

# Offer alternate app icons with an honest native capability contract

## When to use

An app offers selectable launcher icons, but some supported devices lack the API or the UI claims success before the OS accepts the change.

## Recommended approach

Package named alternate icons and expose a native capability/query/change contract. Gate the operation independently of the application's installation floor and reconcile the actual enabled icon after completion.

## Implementation steps

1. Declare bundled alternate icon names/resources through the selected SDK's application configuration, using `alternateIcons` where supported. Keep a default-icon option distinct from named alternatives.
2. Compile against an SDK exposing the required `bundleManager` methods. In an implementation using `getAlternateIcons` and `setAlternateIcon`, retain its API gate; the inspected contract uses API 26 while the app can install on older APIs.
3. Query the available names and enabled icon natively. Validate the requested name against the packaged set; do not trust an arbitrary saved string.
4. Acquire a change guard before awaiting `setAlternateIcon`, release it in finally, and resolve the channel exactly once on accepted completion, busy, unsupported, invalid name or native error.
5. Normalize the public default choice to the native reset convention in one adapter. Re-query enabled state after success and after foreground restoration; persist the effective choice or clearly show a pending/failed request.
6. Remove channel handlers on engine teardown. Keep older devices usable with the default icon and an accurate capability result.

## Conditions and alternatives

The API-26 gate belongs to this native interface and selected SDK family; it does not establish availability on every OpenHarmony product. Check SDK definitions, profile, device and launcher behavior. An application-level image choice cannot replace the OS launcher icon without the corresponding native capability.

## Common mistakes

Returning success on an unsupported device or saving the preference before native acceptance creates false state. A static allowlist must remain synchronized with the packaged icon names. Do not copy Android activity-alias or iOS reset semantics directly into the OH adapter.

## Check it works

Test default-to-alternate and reset, an older supported installation target, unknown names, simultaneous requests, native rejection, restart and launcher refresh. Confirm the UI follows enabled state rather than only the last requested name.

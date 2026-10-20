---
{
  "id": "native-shell-state-synchronization",
  "title": "Keep native navigation and theme in sync with Flutter",
  "topic_key": "integration.native-shell-state-synchronization",
  "area": "integration",
  "keywords": [
    "ArkUI",
    "HDS",
    "native tabs",
    "color mode",
    "theme",
    "原生外壳",
    "windowWillClose",
    "native tabs out of sync",
    "native dark mode mismatch",
    "ArkUI Flutter主题同步"
  ],
  "summary": "Define state ownership and a readiness handshake, then replay the latest state when the native shell becomes ready.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "Native tabs, status surfaces or window controls disagree with Flutter navigation/theme, especially during startup or after native view recreation."
}
---

# Keep native navigation and theme in sync with Flutter

## When to use

Native tabs, status surfaces or window controls disagree with Flutter navigation/theme, especially during startup or after native view recreation.

## Recommended approach

Define state ownership and a readiness handshake, then replay the latest state when the native shell becomes ready.

## Implementation steps

1. Identify which layer owns navigation selection, theme preference and window visibility. Avoid both layers independently deciding the same state.
2. Expose a small bidirectional contract for commands and observed changes.
3. Cache the latest desired state before native initialization; replay it after the shell reports readiness.
4. Translate light/dark/system preference into the appropriate native color-mode behavior rather than matching only a one-time color value.
5. Deduplicate echo updates and remove listeners on teardown.
6. For desktop-style close-to-hide behavior, define how the user reopens or actually exits the app.
7. Avoid sending a selection command during platform-view creation/build when the native backend can immediately echo it into Flutter state. Replay after a safe frame/readiness boundary, check the current view generation, and capture inherited theme values before asynchronous work. Track desired, sent and acknowledged state separately so a failed send remains retryable.

## Conditions and alternatives

Add a native shell only for a product capability or experience that justifies its lifecycle cost. A minimal native host around Flutter remains a valid alternative.

### Synchronize native material settings as effective state

For optional native HDS material levels, define one typed value map across Dart and ArkTS, persist the desired choice, and replay it after native shell readiness. Validate allowed values rather than passing arbitrary integers to AppStorage. Gate the native feature by the selected SDK/product and expose unsupported distinctly; a no-op success on an older API must not make Flutter display the request as applied. Keep the latest desired setting when a native send fails so it can be retried.

Test changes before shell creation, unsupported devices, recreation and an invalid persisted value. Material intensity and theme/navigation ownership are separate settings even if they share a channel.

## Common mistakes

Closing or hiding a window is not a background-execution grant. Native state cached for an old instance must not overwrite newer Flutter preferences.

## Check it works

Test theme/navigation changes before readiness, native recreation, listener cleanup, phone/desktop behavior and close/reopen/exit flows.

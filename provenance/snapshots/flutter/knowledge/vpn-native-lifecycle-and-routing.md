---
{
  "id": "vpn-native-lifecycle-and-routing",
  "title": "Connect a shared VPN core to OH tunnel lifecycle and routing",
  "topic_key": "network.vpn-native-lifecycle-and-routing",
  "area": "network",
  "keywords": [
    "VPN",
    "VpnExtensionAbility",
    "TUN",
    "N-API",
    "IPC",
    "allowBypass",
    "路由",
    "VPN TUN OHOS",
    "route gateway",
    "鸿蒙VPN适配"
  ],
  "summary": "Map the core's network contract into a deliberate OH-native ownership model instead of copying another platform's service assumptions.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "You have a Dart/native VPN core but need an OH implementation of consent, TUN ownership, routing and cleanup."
}
---

# Connect a shared VPN core to OH tunnel lifecycle and routing

## When to use

You have a Dart/native VPN core but need an OH implementation of consent, TUN ownership, routing and cleanup.

## Recommended approach

Map the core's network contract into a deliberate OH-native ownership model instead of copying another platform's service assumptions.

## Implementation steps

1. Choose which native ability/module owns authorization, the tunnel descriptor, core execution and status delivery. If ownership crosses processes, define explicit IPC and error/cancellation behavior.
2. Request consent and required permissions, and stop setup on a required denial rather than treating a logged error as authorization.
3. Derive interface address/prefix/routes from the actual core configuration; map gateway-less routes and bypass policy according to the selected native API.
4. Prevent the core's own traffic from unintentionally reentering the tunnel using the supported mechanism.
5. On disconnect or partial setup failure, release the descriptor, stop the core, cancel polling/listeners and end the associated background activity.

## Conditions and alternatives

A UIAbility-owned core and an extension-service-owned core have different lifetimes and isolation. Native libraries supplied outside the app must expose the required target ABI and ownership contract.

## Common mistakes

A parser accepting a route option does not mean Dart actually supplies it. Do not assume socket protection, bypass semantics or process isolation carry over unchanged.

## Check it works

Test denial, partial setup failure, route/prefix changes, gateway-less routes, self-traffic, reconnect and process teardown with the intended privileges/signing.

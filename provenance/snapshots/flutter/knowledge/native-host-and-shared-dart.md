---
{
  "id": "native-host-and-shared-dart",
  "title": "Add an OH host without rewriting portable Flutter features",
  "topic_key": "host.native-host-and-shared-dart",
  "area": "host",
  "keywords": [
    "FlutterAbility",
    "FlutterPage",
    "Stage",
    "Hvigor",
    "原生宿主",
    "Dart",
    "new HarmonyOS target",
    "how much ArkTS",
    "新增鸿蒙平台"
  ],
  "summary": "Keep portable UI and domain logic in Dart; add the OH host and native implementations only where the app crosses a platform boundary.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "You are starting an OH target and need to decide what belongs in Flutter versus native code."
}
---

# Add an OH host without rewriting portable Flutter features

## When to use

You are starting an OH target and need to decide what belongs in Flutter versus native code.

## Recommended approach

Keep portable UI and domain logic in Dart; add the OH host and native implementations only where the app crosses a platform boundary.

## Implementation steps

1. Generate the native host with the Flutter OH release selected for the app. Use its matching embedding and native build integration.
2. Configure the application's identity, target device types, permissions and signing for that host.
3. Inventory runtime boundaries: plugins, file access, lifecycle callbacks, incoming Wants, platform views and native libraries.
4. Keep pure Dart/widget features unchanged initially. For each boundary, select an OH backend or implement a narrow native adapter behind the existing app interface.
5. Launch the real application entrypoint and exercise each boundary; a generated sample screen is only a host smoke test.

## Conditions and alternatives

A pure Flutter game may need little app-owned ArkTS. A media, Bluetooth or VPN app can require substantial native work even when most screens remain unchanged.

## Common mistakes

Host files and a compiling UI do not establish plugin coverage. Absence of checked-in host files is not decisive when the build generates them; inspect the actual build inputs.

## Check it works

Confirm a signed app launches on the intended device, then test all native-backed features and preserve regression checks for other supported targets.

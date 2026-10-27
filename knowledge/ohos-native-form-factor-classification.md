---
{
  "schema_version": 1,
  "id": "ohos-native-form-factor-classification",
  "title": "Distinguish OH form factor from runtime and window size",
  "topic_key": "window.ohos-native-form-factor-classification",
  "area": "window",
  "scope": "common",
  "frameworks": [],
  "summary": "Read the native device type, map 2in1 explicitly to the application's desktop category, and keep unknown form factors and window capabilities separate.",
  "when_to_use": "An OH application selects phone behavior on a 2in1 device, or a narrow desktop window is incorrectly treated as proof of a phone device.",
  "applicability": "OH products exposing deviceInfo.deviceType. PC classification is source-reviewed through two framework adapters; it does not verify fullscreen, input, display count or every product identifier.",
  "keywords": [
    "deviceInfo.deviceType",
    "2in1",
    "DeviceType.DESKTOP",
    "form factor",
    "window size",
    "unknown",
    "PC detection"
  ],
  "related": [
    "flutter-ohos-pc-form-factor",
    "viewport-scale-and-layout-lifetime",
    "device-gesture-capability"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Distinguish OH form factor from runtime and window size

## When to use

An OH application selects phone behavior on a 2in1 device, or a narrow desktop window is incorrectly treated as proof of a phone device.

## Recommended approach

Read the native device type, map 2in1 explicitly to the application's desktop category, and keep unknown form factors and window capabilities separate.

## Implementation steps

1. Read deviceInfo.deviceType through an actual OH native adapter; an OS/runtime label alone cannot distinguish phone, tablet and 2in1.
2. Map known platform values to a documented application enum. Preserve unknown or unsupported values as unknown; the reviewed Expo mapping uses 2in1 -> DESKTOP.
3. Keep form factor separate from the current window bounds. Use responsive layout for resize while using supported native capabilities for maximize/fullscreen and other window operations.
4. Make initial lookup and cached-value behavior explicit in the adapter. Choose a reversible safe UI while classification is pending rather than enabling unsupported window calls.
5. Declare only supported device types in the host manifest and verify actual input/window/display capabilities before offering the associated action.

## Conditions and alternatives

A platform category is a policy input, not proof that an app supports every desktop feature. Phone-origin tests of a device-info plugin do not verify its 2in1 mapping on a physical PC. Unknown values must remain recoverable as product identifiers evolve.

## Common mistakes

Do not equate every OH runtime with a phone, equate width with device identity, infer touch/keyboard availability from one enum, or treat a DESKTOP return value as completed PC adaptation.

## Check it works

On the intended products compare native type, adapter enum and chosen behavior. Resize a PC window below a mobile breakpoint and verify responsive layout changes without changing hardware identity; test unknown values and lookup errors.

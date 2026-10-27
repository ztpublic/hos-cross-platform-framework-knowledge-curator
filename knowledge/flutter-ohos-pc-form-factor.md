---
{
  "schema_version": 1,
  "id": "flutter-ohos-pc-form-factor",
  "title": "Detect the OH PC form factor and branch the Flutter shell",
  "topic_key": "window.flutter-ohos-pc-form-factor",
  "area": "window",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "summary": "Expose the OH device type from the native side, flag 2in1 as PC, and include it in the Dart desktop/mobile branches before choosing layout, fullscreen and back behavior.",
  "when_to_use": "A Flutter app should feel desktop-like on OH 2in1 but mobile-like on OH phones/tablets, yet Platform.isOhos alone cannot distinguish them, so windows, fullscreen or back behavior are wrong.",
  "keywords": [
    "Flutter",
    "HarmonyOS PC",
    "2in1",
    "deviceType",
    "MethodChannel",
    "Platform.isOhos",
    "desktop layout",
    "fullscreen",
    "moveToBackground"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Flutter OH apps that support both mobile and 2in1/PC device types. The native device-type value and window APIs depend on the OH API/runtime.",
  "related": [
    "flutter-ohos-versioned-so-packaging",
    "native-shell-state-synchronization",
    "window-mode-and-fullscreen-state",
    "ohos-native-form-factor-classification"
  ]
}
---

# Detect the OH PC form factor and branch the Flutter shell

## When to use

A Flutter app should feel desktop-like on OH 2in1 but mobile-like on OH phones/tablets, yet `Platform.isOhos` alone cannot distinguish them, so windows, fullscreen or back behavior are wrong.

## Recommended approach

Expose the OH device type from the native side, flag 2in1 as PC, and include it in the Dart desktop/mobile branches before choosing layout, fullscreen and back behavior.

## Implementation steps

1. Add a native method channel that reads `deviceInfo.deviceType` and returns whether it is `2in1`.
2. Call that channel once at startup before building the shell; default to mobile when the call fails or has not completed.
3. Define `isOhosPC` in the platform layer and make `isDesktop` include `isOhos && isOhosPC`; route layout choices through a single `select(desktop, mobile)` helper rather than ad-hoc width checks.
4. Declare `phone`, `tablet` and `2in1` in the HAP module's device types (or only the supported subset), and set orientation/cutout/background-audio metadata to match the product.
5. Implement fullscreen through the native window APIs: hide the status and navigation indicators, maximize the window, and set orientation. `SystemChrome` immersive mode may not hide the OH navigation indicator completely.
6. Route the back action through the native ability (`moveAbilityToBackground`) when the app should return to the desktop without being destroyed; do not assume Android back semantics.
7. Test phone/tablet versus 2in1, a square-ish display, rotation and window maximize.

## Conditions and alternatives

Use the shared [native form-factor classification](ohos-native-form-factor-classification.md) for the OS category and unknown-value contract. Use the shared [native form-factor classification](ohos-native-form-factor-classification.md) for the OS category and unknown-value contract. The device type is reported by the platform and may not distinguish every PC-like product. A width breakpoint is a fallback for layout, not for form-factor-specific window APIs. Some OH APIs (landscape multi-window, maximize) are version-gated.

## Common mistakes

- Treating every OH device as mobile and shipping a phone layout on 2in1.
- Using only `MediaQuery` width to decide desktop behavior.
- Relying on `SystemChrome` for fullscreen and leaving the navigation indicator visible.
- Assuming a phone back handler works on OH PC.
- Declaring 2in1 support without testing window restore or rotation.

## Check it works

Run on a phone/tablet and on a 2in1. Confirm the correct layout branch is chosen, fullscreen hides both system bars, maximize/restore works, back returns to the desktop as intended, and the app survives rotation and window resize.

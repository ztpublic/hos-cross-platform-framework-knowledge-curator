---
{
  "schema_version": 1,
  "id": "flutter-ohos-target-platform-vendoring",
  "title": "Vendor Flutter packages for the OH TargetPlatform value",
  "topic_key": "build.flutter-ohos-target-platform-vendoring",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "summary": "The OH Flutter SDK adds a TargetPlatform.ohos value; vendor or fork packages with exhaustive TargetPlatform switches, add an explicit OH case, and pin the overrides.",
  "when_to_use": "An OH Flutter build fails on a non-exhaustive TargetPlatform switch in a third-party package, or a package silently falls back to Android behavior for OH.",
  "keywords": [
    "Flutter",
    "TargetPlatform.ohos",
    "exhaustive switch",
    "vendor",
    "dependency_overrides",
    "third-party package",
    "OHOS",
    "platform enum"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Flutter OH projects using packages that enumerate TargetPlatform without a default branch. The needed OH semantics (desktop vs mobile) are product-specific.",
  "related": [
    "flutter-ohos-pc-form-factor",
    "plugin-graph-source-and-api",
    "flutter-ohos-versioned-so-packaging"
  ]
}
---

# Vendor Flutter packages for the OH TargetPlatform value

## When to use

An OH Flutter build fails on a non-exhaustive `TargetPlatform` switch in a third-party package, or a package silently falls back to Android behavior for OH.

## Recommended approach

The OH Flutter SDK adds `TargetPlatform.ohos`; vendor or fork packages with exhaustive `TargetPlatform` switches, add an explicit OH case, and pin the overrides.

## Implementation steps

1. Identify compile failures and packages that switch on `TargetPlatform` or check `Platform.isAndroid/isIOS/...` without an OH branch.
2. Vendor the affected package under a project-owned directory (or a pinned fork) and add an explicit `TargetPlatform.ohos` case. Map it to the intended desktop or mobile behavior rather than blindly to Android.
3. Wire the vendored package through `dependency_overrides`; keep the version/ref pinned and record the patch so it is reapplied when the package upgrades.
4. Check platform-specific widget code (density, scroll physics, gestures, dialogs) because an OH PC may want desktop behavior even though the OS is not Windows/macOS.
5. Keep a default branch only where the behavior is genuinely platform-neutral; do not let a catch-all silently choose the wrong layout.
6. Rebuild all supported targets and confirm the vendored package did not change Android/iOS behavior.

## Conditions and alternatives

The OH enum value is introduced by the oh-flutter SDK; upstream packages may add support later, making the vendor patch unnecessary. A fork with a clearly tracked patch is better than editing the pub cache.

## Common mistakes

- Editing files inside the pub cache, which disappear on the next `pub get`.
- Mapping `TargetPlatform.ohos` to Android unconditionally, so 2in1 gets a phone layout.
- Adding a `default` branch that hides a missing desktop case.
- Forgetting to pin the vendored package, so a dependency upgrade silently drops the patch.
- Testing only the OH build and not the other targets.

## Check it works

Build OH without non-exhaustive-switch errors, then verify the affected widget shows the intended desktop/mobile behavior on a 2in1 and a phone/tablet. Rebuild Android/iOS and confirm their behavior is unchanged and the override is still pinned.

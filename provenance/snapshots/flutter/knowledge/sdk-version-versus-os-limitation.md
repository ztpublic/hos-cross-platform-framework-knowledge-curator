---
{
  "id": "sdk-version-versus-os-limitation",
  "title": "Fix missing Flutter APIs without assuming an OH limitation",
  "topic_key": "compatibility.sdk-version-versus-os-limitation",
  "area": "compatibility",
  "keywords": [
    "Dart",
    "Flutter SDK",
    "API generation",
    "版本差异",
    "charts",
    "ScrollCacheExtent",
    "PopScope",
    "undefined named parameter",
    "missing widget API",
    "Dart version conflict",
    "SDK不兼容"
  ],
  "summary": "Compare the Flutter/Dart/API requirements first. Either align the toolchain or implement a narrowly scoped compatibility alternative.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "A widget, parameter, package or Dart language feature compiles on another Flutter SDK but fails on the chosen Flutter OH toolchain."
}
---

# Fix missing Flutter APIs without assuming an OH limitation

## When to use

A widget, parameter, package or Dart language feature compiles on another Flutter SDK but fails on the chosen Flutter OH toolchain.

## Recommended approach

Compare the Flutter/Dart/API requirements first. Either align the toolchain or implement a narrowly scoped compatibility alternative.

## Implementation steps

1. Identify whether the error comes from Dart language syntax, Flutter framework APIs, a package API or a native OH API.
2. Compare the exact bundled Dart/Flutter versions and package constraints with the feature's requirements.
3. If an upgrade is feasible, align the OH toolchain, plugins and native SDK together. Recheck minimum device API implications.
4. Otherwise, use an equivalent older interface or a small adapter. For a finite options list, a simpler widget composition may replace a newer convenience API without changing user behavior.
5. Mark the compatibility branch by the capability/version condition that requires it, and remove it after verifying the newer baseline.

## Conditions and alternatives

Choose an older package only if it still implements the methods and fixes you need. A relaxed caret constraint permits a range of releases; it is not an exact reproducibility pin.

## Common mistakes

Do not label charts, scrolling or ordinary widgets unsupported by the OS because one SDK lacks an API. Lowering a declared constraint does not make newer syntax compatible.

## Check it works

Analyze with the actual OH SDK, resolve the intended packages and compare behavior of the compatibility path. Test the oldest supported device/API combination.

---
{
  "schema_version": 1,
  "id": "qt-icu-sdk-contract",
  "title": "Match Qt ICU probes to the SDK headers and libraries",
  "topic_key": "build.qt-icu-sdk-contract",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Check the SDK's real ICU headers, exported APIs and library layout before adjusting configure detection.",
  "when_to_use": "Qt configure reports ICU unavailable or compilation fails after forcing it on.",
  "keywords": [
    "ICU",
    "ures.h",
    "QCollator",
    "QTimeZone"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to Qt OH builds with ICU integration; a historical SDK-version recommendation must be rechecked before selecting a current toolchain.",
  "related": []
}
---

# Match Qt ICU probes to the SDK headers and libraries

## When to use

Qt configure reports ICU unavailable or compilation fails after forcing it on.

## Recommended approach

Check the SDK's real ICU headers, exported APIs and library layout before adjusting configure detection.

## Implementation steps

1. Compare the failing probe with actual merged or separate ICU library names in the selected SDK.
2. Confirm required headers and symbols exist, not just a link stub.
3. Prefer a compatible SDK/module combination; scope any probe patch to the verified layout.
4. If disabling unavailable ICU functionality, specify affected collation/time-zone behavior and validate the fallback.

## Conditions and alternatives

Applies to Qt OH builds with ICU integration; a historical SDK-version recommendation must be rechecked before selecting a current toolchain.

## Common mistakes

Force-enabling ICU or suppressing an include does not supply missing converter/resource APIs.

## Check it works

Run configure, compile affected modules and test representative collation, locale and time-zone operations.

---
{
  "schema_version": 1,
  "id": "qt-ohos-kde-shims",
  "title": "Run a KDE Frameworks app with a minimal shim layer",
  "topic_key": "integration.qt-ohos-kde-shims",
  "area": "integration",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Replace small KF6 value APIs with local shims, package QML fallback modules, persist preferences with QSettings, and disable large KDE-dependent flows behind MVP guards.",
  "when_to_use": "A Qt/KDE app fails to build on OH because KF6 modules are missing, or its core flow depends on KLocalizedString, KAboutData, KColorScheme, KConfig or QML modules such as Kirigami.",
  "keywords": [
    "KDE Frameworks",
    "KF6",
    "KLocalizedString",
    "KColorScheme",
    "KAboutData",
    "Kirigami",
    "QSettings",
    "QML fallback",
    "shim",
    "MVP"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Qt/KDE apps ported to OH as an MVP. The shims cover only the APIs the core flow uses; they are not a full KDE Frameworks port.",
  "related": [
    "qt-ohos-qml-module-bundling",
    "qt-optional-module-requirements",
    "qt-cmake-package-discovery"
  ]
}
---

# Run a KDE Frameworks app with a minimal shim layer

## When to use

A Qt/KDE app fails to build on OH because KF6 modules are missing, or its core flow depends on `KLocalizedString`, `KAboutData`, `KColorScheme`, `KConfig` or QML modules such as Kirigami.

## Recommended approach

Replace small KF6 value APIs with local shims, package QML fallback modules, persist preferences with `QSettings`, and disable large KDE-dependent flows behind MVP guards.

## Implementation steps

1. Inventory the KF6 modules the app actually uses and separate small value APIs (i18n, colors, about data, category roles) from large UI/config/KCM modules.
2. Provide local shim headers for the small APIs: translate `KLocalizedString` calls to `QString` formatting, map `KColorScheme` roles to `QPalette`, supply a display name through `KAboutData`, and define the category role constants the models require.
3. Implement the preferences API with `QSettings`, keeping the same keys/defaults the app expects; expose `i18n`/`i18nc` to QML through a root-context object.
4. Package QML fallback modules for `org.kde.kirigami`, `org.kde.coreaddons` and `org.kde.charts` under the QRC import prefix.
5. Guard editor, custom-lesson, preferences, shortcut-config and KCM flows behind MVP compile definitions and return a clean "not available" result; keep the core flow and fullscreen toggle.
6. Bundle only the minimal data set needed for the MVP (courses, layouts, schemas) and skip schema validation when the validator is unavailable.
7. Link only Qt modules and verify the app never loads a KF6 library at runtime.

## Conditions and alternatives

This is an MVP substitution, not a KDE Frameworks port. Shims approximate behavior and must be reviewed for each API. When a feature needs real KConfig/KCM behavior, port that module or keep the feature disabled rather than silently changing semantics.

## Common mistakes

- Trying to port all of KF6 before the first device build.
- Shipping a shim that changes user-visible semantics without documenting it.
- Forgetting the QML fallback modules, so the app builds but fails at import time.
- Leaving KConfig/KCM calls in code paths that are still reachable.
- Bundling an empty or partial data set, then treating the resulting empty screen as a rendering bug.

## Check it works

Build without KF6 linked, launch on device, and complete the core flow: start a course, type, advance a lesson, persist a preference, restart, and confirm the setting survives. Confirm disabled editor/preferences entries fail cleanly and no KF6 library appears in the HAP's dependency closure.

---
{
  "schema_version": 1,
  "id": "qt-ohos-qml-module-bundling",
  "title": "Bundle the Qt QML module tree into the HAP",
  "topic_key": "build.qt-ohos-qml-module-bundling",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Generate a QRC from the Qt SDK's QML module roots, patch incompatible QML in a copied tree, embed it under the Qt import prefix, and package the QML plugin libraries.",
  "when_to_use": "A QML app on OH shows a blank page, reports that a Qt Quick Controls or Layouts module is not installed, or crashes on a QML property the OH Qt build does not support.",
  "keywords": [
    "QML",
    "Qt Quick",
    "qmldir",
    "QRC",
    "qt-project.org/imports",
    "QML plugin",
    "Controls.2",
    "Layouts",
    "qml cache",
    "Qt 5.15",
    "Qt 6"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Qt QML apps packaged for OH with a bundled Qt-for-OH SDK. The module list and QML patches are tied to the SDK version.",
  "related": [
    "qt-ohos-kde-shims",
    "qt-plugin-deployment-layout",
    "qt-ohos-resource-dir"
  ]
}
---

# Bundle the Qt QML module tree into the HAP

## When to use

A QML app on OH shows a blank page, reports that a Qt Quick Controls or Layouts module is not installed, or crashes on a QML property the OH Qt build does not support.

## Recommended approach

Generate a QRC from the Qt SDK's QML module roots, patch incompatible QML in a copied tree, embed it under the Qt import prefix, and package the QML plugin libraries next to the app.

## Implementation steps

1. Enumerate the QML module roots the app imports (for example QtQuick Controls 2, Layouts, Templates, Window, QtQml and GraphicalEffects).
2. Generate a QRC that maps each file under `/qt-project.org/imports`, preserving the `qmldir` files and relative aliases.
3. Copy QML files into a build-time temp tree and patch the specific properties the OH Qt build rejects or renders incorrectly (for example unsupported font-weight/bold expressions or `origin.x`/`origin.y`). Do not edit the SDK in place.
4. Collect the module's `.so` QML plugins separately and copy them into the app's plugin directory after linking.
5. Keep `qmldir` metadata and plugin registration intact; a QRC without the matching plugin or qmldir can still fail to import.
6. Link the Qt modules used by the QML files and package their runtime libraries with the HAP.
7. Validate each module import at build time (qmllint or a runtime smoke screen), not only the top-level QML file.

## Conditions and alternatives

The module list and patches follow one Qt-for-OH SDK. Another SDK version may support the removed properties or require different ones. Embedding the modules in the QRC is an alternative to installing them into the device's QML import path.

## Common mistakes

- Packaging only the app's QML and assuming Qt Quick Controls come from the system.
- Editing QML files inside the SDK so the change disappears on the next SDK update.
- Forgetting the `.so` QML plugins or their `qmldir` entries.
- Patching properties without checking the module's aliases and relative imports.
- Testing only the main window while a dialog imports a different module.

## Check it works

Launch the app and open each major screen that imports a different QML module. Confirm Controls, Layouts, Templates and Window modules load, dialogs render, and no property warnings or blank pages appear. Rebuild with a clean temp tree to prove the generated QRC is reproducible.

---
{
  "schema_version": 1,
  "id": "qt-ohos-resource-dir",
  "title": "Resolve Qt resources from the HAP resource directory",
  "topic_key": "storage.qt-ohos-resource-dir",
  "area": "storage",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Ask the OH QPA for its resource directory and use it; do not infer an install prefix from the executable path, which points at the host shell in an OH process.",
  "when_to_use": "A Qt app starts on OH but cannot find bundled data, translations or templates, especially when the path is derived from the executable location.",
  "keywords": [
    "QOhosAppContext",
    "resourceDir",
    "resfile",
    "rawfile",
    "install path",
    "QStandardPaths",
    "applicationDirPath",
    "Qt for OpenHarmony",
    "resourceManager"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Qt 5.12 for OpenHarmony apps that read bundled files through native file I/O. The resource-directory access is private QPA API and may differ in another Qt generation.",
  "related": [
    "qt-ohos-plugin-loading",
    "sandbox-paths-and-app-identity",
    "qt-cmake-package-discovery"
  ]
}
---

# Resolve Qt resources from the HAP resource directory

## When to use

A Qt app starts on OH but cannot find bundled data, translations or templates, especially when the path is derived from the executable location.

## Recommended approach

Ask the OH QPA for its resource directory and use that path. Do not infer an install prefix from `/proc/<pid>/exe`, which points at the host shell in an OH process.

## Implementation steps

1. Remove executable-relative install-path logic. In the OH process, `/proc/<pid>/exe` resolves to the host entry library, not the application library.
2. Include the QtCore private header that exposes `QOhosAppContext`, link QtCore's private target (`QT_PRIVATE += core` for qmake), and read `QOhosAppContext::getProperty(QOhosAppContext::Type::resourceDir)`.
3. Set the app's resource-root environment variable before library initialization, and guard against an empty value because an empty path can be resolved as `/`.
4. Place files that native code opens with `std::ifstream`/`fopen` under `resources/resfile/<dir>`. Files under `resources/rawfile` remain inside the HAP and require `resourceManager`; `entry/libs/` accepts only `.so` files.
5. Make resource lookup failure explicit. Log the computed root and the missing relative path so a packaging error is distinguishable from a bad path join.

## Conditions and alternatives

`QOhosAppContext` is private API and its properties vary by Qt generation. Treat the bundled tree as install content: when the app must mutate data, copy it into the app's writable directory instead of writing back into `resfile`.

## Common mistakes

- Using `QCoreApplication::applicationDirPath()` as an install prefix.
- Putting native-readable resources in `rawfile` and expecting `fopen` to work.
- Forgetting the private include path or QtCore private linkage.
- Letting an unset environment variable become the filesystem root.

## Check it works

Start the app on a device and load a resource that uses native file I/O. Rename one packaged resource and confirm the app reports a clear missing-path error rather than silently using an empty path or crashing.

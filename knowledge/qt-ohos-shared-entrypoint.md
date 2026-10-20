---
{
  "schema_version": 1,
  "id": "qt-ohos-shared-entrypoint",
  "title": "Export the Qt entrypoint from the OH shared library",
  "topic_key": "build.qt-ohos-shared-entrypoint",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "OH Qt builds the application as a shared library; export main in the dynamic symbol table and make the library name in the ArkTS host match the packaged .so exactly.",
  "when_to_use": "A Qt application builds for OH, but launching the HAP does nothing or the host reports that the main symbol is null even though the application library loaded.",
  "keywords": [
    "dlsym",
    "main",
    "export-dynamic",
    "fvisibility=hidden",
    "APP_LIBRARY_NAME",
    "libqohos",
    "shared library",
    "QAbility",
    "Qt for OpenHarmony",
    "mkspec"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Qt 5.12 for OpenHarmony projects launched through the QPA shared-library host. The startup contract and symbol visibility flags must be checked against the selected Qt generation.",
  "related": [
    "qt-qpa-runtime-template-coherence",
    "qt-resource-registration-linkage",
    "qt-ohos-resource-dir"
  ]
}
---

# Export the Qt entrypoint from the OH shared library

## When to use

A Qt application builds for OH, but launching the HAP does nothing or the host reports that the `main` symbol is null even though the application library loaded.

## Recommended approach

OH Qt builds the application as a shared library. Export `main` in the dynamic symbol table and make the library name in the ArkTS host match the packaged `.so` exactly.

## Implementation steps

1. Inspect the build product: the OH mkspec forces an application template to `-shared`, producing `lib<app>.so` rather than an executable.
2. Export the entrypoint. With the stock mkspec visibility settings, add `-Wl,--export-dynamic` to the application link flags, or explicitly mark `main` as visible.
3. Verify the symbol with `nm -D lib<app>.so` (or the OH equivalent) before packaging; a stripped or hidden `main` produces a library that loads but never starts.
4. Set `APP_LIBRARY_NAME` in the ArkTS `QAbility` to the exact packaged filename, including the `lib` prefix and `.so` suffix.
5. Keep exactly one `main` and one entry shim. Check the host log for the library-open and `dlsym("main")` steps; a log that stops after opening the library points back to symbol visibility or a name mismatch.

## Conditions and alternatives

The shared-library startup contract belongs to the Qt for OpenHarmony integration; another Qt generation or host template may use different names. When the host supports a real executable, this workaround is unnecessary.

## Common mistakes

- Building a separate executable and expecting the HAP to launch it.
- Stripping the application library after linking.
- Using a display name instead of the on-disk `.so` filename in `APP_LIBRARY_NAME`.
- Adding a second `main` in a wrapper library.

## Check it works

Launch the app and confirm the host log shows the library opening and `main` being called. Independently confirm `main` is exported in the packaged library and that the host's library name resolves to the same file.

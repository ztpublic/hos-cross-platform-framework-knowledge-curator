---
{
  "schema_version": 1,
  "id": "qt-ohos-save-picker-name-and-format",
  "title": "Set the save-picker filename and default format separately",
  "topic_key": "storage.qt-ohos-save-picker-name-and-format",
  "area": "storage",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Pass a directory as the initial location, preselect the basename explicitly, and put the intended MIME/suffix choice first for the selected OH picker adapter.",
  "when_to_use": "A Qt OH save dialog ignores the suggested filename, produces files_3.ppm, or saves an image with a suffix that the system preview cannot open.",
  "applicability": "QFileDialog backed by the reviewed Qt 6.12 OH DocumentViewPicker helper. Applies to PC save/export windows; supported initial locations and authorization remain independent requirements.",
  "keywords": [
    "QFileDialog",
    "selectFile",
    "newFileNames",
    "files_3.ppm",
    "setMimeTypeFilters",
    "setDefaultSuffix",
    "PNG",
    "HiPreview"
  ],
  "related": [
    "qt-picker-initial-uri",
    "qt-picker-suffix-filter-mapping",
    "file-export-contract"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Set the save-picker filename and default format separately

## When to use

A Qt OH save dialog ignores the suggested filename, produces files_3.ppm, or saves an image with a suffix that the system preview cannot open.

## Recommended approach

Pass a directory as the initial location, preselect the basename explicitly, and put the intended MIME/suffix choice first for the selected OH picker adapter.

## Implementation steps

1. Keep a stable visible foreground parent while opening the native save dialog; do not leave it attached only to an already-hidden capture window.
2. Split a suggested full path into an allowed initial directory and basename. Construct QFileDialog with the directory and call selectFile(basename) so the adapter can fill newFileNames.
3. Build filters from formats the application can actually encode. Move the configured MIME type to the front; use a supported PNG fallback when appropriate.
4. Set the selected MIME filter and default suffix as well. The reviewed adapter applies its first native suffix choice even when a prefilled basename has another suffix.
5. After acceptance, use the returned target and the selected format consistently. Return cancellation normally and verify file bytes as well as the filename.

## Conditions and alternatives

Filter order is adapter-specific behavior, not a universal Qt rule. A correctly named file can still contain the wrong encoding or be unreadable by another app. Omit an unsupported initial location instead of assuming a private path is a valid document URI.

## Common mistakes

Do not pass a complete filename as the initial directory, rely on supportedMimeTypes enumeration order, or treat setDefaultSuffix alone as proof of the effective native choice.

## Check it works

Save PNG and JPEG with default and edited basenames, cancel, and invoke export after hiding the capture view. Inspect signatures and suffixes and open the results in the intended PC preview application.

---
{
  "schema_version": 1,
  "id": "ohos-fontconfig-sandbox-bootstrap",
  "title": "Give fontconfig a sandbox config before first font use on OH",
  "topic_key": "text.ohos-fontconfig-sandbox-bootstrap",
  "area": "text",
  "scope": "common",
  "frameworks": [],
  "summary": "Before any font library starts, write a fonts.conf listing /system/fonts plus a sandbox cache directory, and set FONTCONFIG_FILE and the HOME and XDG variables to writable paths.",
  "when_to_use": "A ported app using fontconfig finds no fonts, shows empty text, or crashes during startup resource or font registration.",
  "applicability": "Native libraries on OH that use fontconfig. The sandbox has no /etc/fonts and no default HOME or XDG directories according to the reviewed port. Environment must be set before the library initializes.",
  "keywords": [
    "fontconfig",
    "fonts.conf",
    "FONTCONFIG_FILE",
    "/system/fonts",
    "XDG_CACHE_HOME",
    "HOME",
    "no fonts",
    "font registry",
    "QT_QPA_FONTDIR"
  ],
  "related": [
    "sandbox-paths-and-app-identity",
    "qt-ohos-default-font-metrics",
    "startup-work-prerequisites"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Give fontconfig a sandbox config before first font use on OH

## When to use

A ported app using fontconfig finds no fonts, shows empty text, or crashes during startup resource or font registration.

## Recommended approach

Before any font library starts, write a fonts.conf listing /system/fonts plus a sandbox cache directory, and set FONTCONFIG_FILE and the HOME and XDG variables to writable paths.

## Implementation steps

1. Get the writable directory from context.filesDir and cacheDir and pass it to native code. The reviewed port hard-codes one module's path, which breaks under another module name.
2. Create the app font and cache directories, then write a minimal fonts.conf with <dir>/system/fonts</dir>, the app font directory and a sandbox <cachedir>.
3. Set FONTCONFIG_FILE, HOME, XDG_CACHE_HOME and XDG_DATA_HOME before the first fontconfig or framework font call. For Qt, QT_QPA_FONTDIR can also point at /system/fonts.
4. Check the write succeeded and log the number of fonts found, so an empty list is visible instead of surfacing later as a crash.

## Conditions and alternatives

Bundle fonts when rendering must be the same across devices; system font files are a product detail, so do not hard-code particular names. Check other HOME consumers before overriding it process-wide.

## Common mistakes

Setting variables after the framework has already loaded fonts. Pointing the cache at a read-only location, causing a slow rescan or failure at every launch. Treating Qt font metric differences as a fontconfig problem; see the related entry.

## Check it works

On a clean install, log the fontconfig result count, render CJK and Latin text, restart to confirm the cache is reused, and test under the real module name.

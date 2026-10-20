---
{
  "schema_version": 1,
  "id": "qt-ohos-plugin-loading",
  "title": "Load Qt application plugins from the install library directory",
  "topic_key": "plugins.qt-ohos-plugin-loading",
  "area": "plugins",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "OH refuses to load shared libraries from the writable application data area; package application plugin .so files in the install library directory and explicitly preload them from there.",
  "when_to_use": "Application plugins are packaged next to resources, but dlopen fails or every plugin reports that it cannot load even though the files exist.",
  "keywords": [
    "dlopen",
    "RTLD_NOW",
    "RTLD_GLOBAL",
    "db_plugins",
    "libs/arm64-v8a",
    "writable partition",
    "plugin loading",
    "el2",
    "Qt for OpenHarmony"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH app sandboxes that prohibit loading shared objects from the writable data area. The exact partition policy and paths are product/API dependent.",
  "related": [
    "qt-ohos-resource-dir",
    "qt-plugin-deployment-layout",
    "qt-ohos-link-gc-static-registration"
  ]
}
---

# Load Qt application plugins from the install library directory

## When to use

Application plugins are packaged next to resources, but `dlopen` fails or every plugin reports that it cannot load even though the files exist.

## Recommended approach

OH refuses to load shared libraries from the writable application data area. Put application plugin `.so` files in the install library directory and explicitly preload them from there.

## Implementation steps

1. Package application plugin libraries under `libs/arm64-v8a/` (the install-side native library directory), not under `resfile/` or another writable path.
2. Remove legacy plugin copies from the resource directory; they generate repeated failed loads and may shadow the real path.
3. At startup, enumerate the packaged plugin filenames and call `dlopen(path, RTLD_NOW | RTLD_GLOBAL)` from the install directory. Keep the order deterministic if plugins depend on one another.
4. Keep Qt's own plugins (QPA, SQL drivers, image formats) in the layout expected by the Qt SDK; do not conflate them with application plugins.
5. After each load, verify the plugin's registered objects or types. A successful `dlopen` does not prove registration.

## Conditions and alternatives

The exact writable/install partition policy is product and API dependent; verify on the target rather than assuming all data directories behave the same. This is not a substitute for packaging Qt's plugin dependency closure.

## Common mistakes

- Copying plugins beside resources because the resource directory is writable.
- Relying on a plugin search path that points into the writable area.
- Treating `dlopen` success as registration success.
- Deleting the plugin directory instead of moving it, leaving dangling loader configuration.

## Check it works

On a device, confirm the plugin files exist under the install library directory, the startup log shows a successful preload for each, and the application's registry count increases. Exercise one operation that requires a plugin and confirm it no longer falls back to "unknown format" or an empty registry.

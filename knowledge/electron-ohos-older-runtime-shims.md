---
{
  "schema_version": 1,
  "id": "electron-ohos-older-runtime-shims",
  "title": "Bridge an Electron app to an older OH runtime",
  "topic_key": "host.electron-ohos-older-runtime-shims",
  "area": "host",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Pin the runtime's Electron/Node versions, diff the app's API usage against the runtime exports, and add narrow compatibility modules for missing clipboard, context-menu, single-instance, updater and font APIs.",
  "when_to_use": "The app targets a newer Electron major than the OH runtime provides; it fails at module load on a missing export, uses unsupported single-instance or updater APIs, or enumerates fonts through desktop tools.",
  "keywords": [
    "Electron",
    "runtime version",
    "ClipboardItem",
    "clipboard.write",
    "context menu",
    "single instance",
    "electron-updater",
    "fc-list",
    "fonts",
    "compatibility shim"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH Electron apps whose desktop Electron major is newer than the ported runtime. The support matrix and shims must be built from the actual target runtime.",
  "related": [
    "electron-ohos-platform-capability-gates",
    "electron-ohos-native-addon-abi",
    "electron-ohos-statusbar-openurl"
  ]
}
---

# Bridge an Electron app to an older OH runtime

## When to use

The app targets a newer Electron major than the OH runtime provides; it fails at module load on a missing export, uses unsupported single-instance or updater APIs, or enumerates fonts through desktop tools.

## Recommended approach

Pin the runtime's actual Electron, Node and Chromium versions, diff the app's API usage against the runtime's export list, and add narrow compatibility modules at the call sites rather than forking the app. Keep desktop behavior unchanged.

## Implementation steps

1. Record `process.versions` and `process.platform` from a minimal HAP on the target; the platform string may be `openharmony` or `ohos` depending on the runtime build.
2. Build an API support matrix for the modules the app imports, including named exports and methods, not just module presence.
3. Inject the Electron module into compatibility helpers instead of importing missing named exports at module scope. For clipboard images, choose `ClipboardItem` when available and fall back to `clipboard.write({image, html})` otherwise.
4. Vendor or patch small dependencies that assume newer APIs; keep each patch minimal and covered by tests for both API generations.
5. Gate `requestSingleInstanceLock` and `second-instance`, and disable or isolate `electron-updater`; route file-open and second launch through the host's Ability/Want path.
6. Replace desktop font enumeration (`fc-list`) with direct reads of the system font directories and parsing of the sfnt `name` table.
7. Verify export windows (`capturePage`, `printToPDF`, hidden `BrowserWindow`), clipboard formats, context menus and file dialogs on the target, recording the runtime version for every result.

## Conditions and alternatives

A version gap may be too large for shims; an unsupported API may have no equivalent. Prefer upgrading a consistent runtime/engine set. Host-side Ability/Want routing is required where single-instance semantics are absent.

## Common mistakes

- Statically importing a named export that the runtime does not provide.
- Assuming one platform string across runtime builds.
- Treating a successful build as proof of API compatibility.
- Leaving the desktop updater enabled.
- Calling `fc-list` on a device image that does not include it.

## Check it works

Launch offline with no module-load error. Exercise text, image and rich clipboard writes/reads, a context menu, a file dialog, an export window and the font list. Record `process.versions` and confirm each compatibility branch was actually taken.

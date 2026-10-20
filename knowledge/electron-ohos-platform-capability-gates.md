---
{
  "schema_version": 1,
  "id": "electron-ohos-platform-capability-gates",
  "title": "Gate Electron features that OH does not provide",
  "topic_key": "host.electron-ohos-platform-capability-gates",
  "area": "host",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Detect process.platform === openharmony and guard desktop-only capabilities at their call sites: window frame, tray/window visibility, updater, dock and default paths.",
  "when_to_use": "An Electron app starts on OH but the window cannot be hidden and restored, the custom title bar has no controls, the updater errors, or default folder lookups throw.",
  "keywords": [
    "Electron",
    "openharmony",
    "frameless window",
    "Tray",
    "auto-update",
    "appData",
    "userData",
    "app.dock",
    "2in1",
    "platform gate"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH 2in1/tablet devices running a ported Electron runtime. Capability availability can change with the runtime/API level, so keep the checks feature-based.",
  "related": [
    "electron-ohos-builder-packaging",
    "electron-ohos-non-pty-exec",
    "jit-executable-memory-authorization"
  ]
}
---

# Gate Electron features that OH does not provide

## When to use

An Electron app starts on OH but the window cannot be hidden and restored, the custom title bar has no controls, the updater errors, or default folder lookups throw.

## Recommended approach

Detect the OH runtime with `process.platform === 'openharmony'` and guard each desktop-only capability at its call site: window frame, tray/window visibility, updater, dock and default paths.

## Implementation steps

1. Centralize a platform flag and use capability checks (`app.dock`, `app.getPath` in try/catch), not product or bundle names.
2. Use the system title bar on OH. A frameless window has no native minimize/maximize/close controls, so a custom title bar can leave the window impossible to close or restore.
3. Create the tray before hiding or showing the main window. OH couples window visibility to the tray; hiding a window without a tray can make the hide/show pair ineffective.
4. Disable auto-update on OH and route distribution through the store. Do not call the desktop updater or show its errors.
5. Treat `appData` as optional: `app.getPath('appData')` may throw. Fall back to `userData` and migrate only after checking that the old directory exists.
6. Do not default output to `pictures`; OH may not grant that directory. Use the app sandbox or a directory the user selects through the picker.
7. Guard `app.dock` calls and test minimize, restore, close and tray menu actions.

## Conditions and alternatives

Some capabilities may become available in a later OH runtime or with additional ACLs; keep the checks capability-based so the app can adopt them. A store-distributed app may not be allowed to self-update at all.

## Common mistakes

- Calling `app.getPath('appData')` unguarded and crashing at startup.
- Keeping a frameless window with a renderer-drawn title bar.
- Creating the window before the tray and then hiding it.
- Leaving the desktop updater enabled and treating its network failure as an app failure.
- Writing to `pictures` without checking access.

## Check it works

On a 2in1 device, confirm the system title bar has working controls, the tray can hide and restore the window, the updater is inactive, startup does not read `appData` unprotected, and the default output directory is writable or chosen by the user.

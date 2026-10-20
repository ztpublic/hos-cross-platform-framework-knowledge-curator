---
{
  "schema_version": 1,
  "id": "electron-ohos-statusbar-openurl",
  "title": "Attach an Electron window to the OH status bar",
  "topic_key": "window.electron-ohos-statusbar-openurl",
  "area": "window",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Register the status-bar item before the host attaches secondary abilities, restore the first live window on click, and bridge file-open Want URIs into the app second-instance path.",
  "when_to_use": "The OH host launches secondary abilities only after a status-bar item is registered, or file-open Want events never reach the running Electron app.",
  "keywords": [
    "Electron",
    "Tray",
    "status bar",
    "ATTACH_TO_STATUS_BAR_ITEM",
    "open-url",
    "second-instance",
    "file association",
    "Want",
    "2in1"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH Electron apps paired with a host that attaches secondary abilities through a status-bar item. Host API names and file-open behavior vary.",
  "related": [
    "electron-ohos-platform-capability-gates",
    "external-deep-action-contract",
    "electron-ohos-older-runtime-shims"
  ]
}
---

# Attach an Electron window to the OH status bar

## When to use

The OH host launches secondary abilities only after a status-bar item is registered, or file-open Want events never reach the running Electron app.

## Recommended approach

Create the status-bar Tray before the host needs to attach, restore and focus the first live window on click, destroy it on quit, and bridge `open-url` file URIs into the app's second-instance/open-file path.

## Implementation steps

1. On the OH platform string, create a `Tray` with a valid icon early, before the host launches a secondary ability; the host's `ATTACH_TO_STATUS_BAR_ITEM` contract needs the item to exist.
2. On tray click, find a non-destroyed `BrowserWindow` and show/focus it; do not create a duplicate window if one already exists.
3. Destroy the tray on `will-quit` and guard re-initialization so a repeated call does not leak items.
4. Hook `app.on('open-url')`; accept only `file://` URLs, convert them with `fileURLToPath`, and emit the app's existing second-instance/open-file flow with its normal authorization and new-window policy.
5. Ensure the app's file-open path is idempotent and handles a cold start (URL before the main window exists) and a warm start.
6. Test with the host's secondary ability, a file association open, tray restore and normal quit.

## Conditions and alternatives

The host must implement the status-bar attachment contract. If the runtime provides a different tray/status API, use that surface instead. File URI routing should reuse the app's existing authorization path rather than opening arbitrary paths.

## Common mistakes

- Creating the tray only after the first window, so the host cannot attach.
- Using an empty or missing icon and failing silently.
- Not destroying the tray on quit.
- Accepting non-file URLs or bypassing existing file-authorization checks.
- Emitting a second-instance event with a shape the app's handler does not understand.

## Check it works

Launch the app and trigger the host's secondary ability; confirm it attaches only after the status-bar item exists. Click the tray and verify the existing window is shown and focused. Open a file from the host while the app is running and while it is closed, and confirm both reach the same validated open-file path.

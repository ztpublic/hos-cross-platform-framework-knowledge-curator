---
{
  "schema_version": 1,
  "id": "electron-ohos-window-types",
  "title": "Create sub and floating windows in an Electron OH app",
  "topic_key": "window.electron-ohos-window-types",
  "area": "window",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Use the host's windowInfo type (mainWindow, subWindow, floatWindow), create OH subwindows with a parent and bounds, and reuse/minimize them instead of creating independent abilities.",
  "when_to_use": "An Electron app opens child windows, popups or floating panels on OH, but they fail to appear, lose their parent, accumulate after closing, or a floating window is blocked by a missing permission.",
  "keywords": [
    "Electron",
    "BrowserWindow",
    "windowInfo",
    "subWindow",
    "floatWindow",
    "createSubWindowWithOptions",
    "SYSTEM_FLOAT_WINDOW",
    "WINDOW_TOPMOST",
    "multi-window",
    "2in1"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH Electron apps using a host that maps BrowserWindow windowInfo types to OH subwindows. Floating-window permissions and multi-display APIs depend on the API level.",
  "related": [
    "electron-ohos-process-isolation",
    "electron-ohos-statusbar-openurl",
    "electron-ohos-platform-capability-gates"
  ]
}
---

# Create sub and floating windows in an Electron OH app

## When to use

An Electron app opens child windows, popups or floating panels on OH, but they fail to appear, lose their parent, accumulate after closing, or a floating window is blocked by a missing permission.

## Recommended approach

Use the host's `windowInfo.type` (`mainWindow`, `subWindow`, `floatWindow`) and create OH subwindows with the correct parent and bounds. Reuse or minimize closed subwindows instead of creating independent abilities for each one.

## Implementation steps

1. Pass `windowInfo.type` in the `BrowserWindow` options. Use `subWindow` for child windows and `floatWindow` for panels that must stay above other windows; set `parent` to the owning window.
2. Request `SYSTEM_FLOAT_WINDOW` / `WINDOW_TOPMOST` only for floating windows that need them; declare the permission where the host reads it and handle denial with a normal window fallback.
3. In the host adapter, create the OH subwindow with `createSubWindowWithOptions`, load its page, apply bounds, and forward `windowSizeChange`/move events back to the Electron window.
4. Set focusable state for the subwindow and never let a hidden/cached window keep input focus.
5. Cache closed subwindows by parent and reuse them; cap the cache (the template uses a global limit) and destroy the oldest rather than leaking windows.
6. Move across displays with the API-level-appropriate call: a display-aware move on newer APIs, the older `moveWindowTo` otherwise.
7. On cancel/close, minimize and cache the subwindow; on app quit, destroy all cached and live subwindows.

## Conditions and alternatives

The host must implement the windowInfo mapping; a plain second `BrowserWindow` may become an independent ability. Floating windows require product/ACL approval. Multi-display behavior depends on the API level.

## Common mistakes

- Creating a new UIAbility for every child window and losing parent ownership.
- Omitting the parent, so the child floats above every app window.
- Requesting floating-window permissions for every window, including normal children.
- Never removing cached subwindows and exhausting the window limit.
- Treating a denied float permission as a fatal error instead of falling back.

## Check it works

Open a child window and confirm it stays above its parent and moves with it. Minimize, close and reopen it several times; the cache should reuse a window and not grow without bound. Open a floating panel, verify topmost behavior and permission handling, then quit and confirm no subwindow remains.

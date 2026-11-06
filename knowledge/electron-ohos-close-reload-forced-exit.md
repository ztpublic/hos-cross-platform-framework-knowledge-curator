---
{
  "schema_version": 1,
  "id": "electron-ohos-close-reload-forced-exit",
  "title": "End an OH Electron app when window close does not exit it",
  "topic_key": "lifecycle.electron-ohos-close-reload-forced-exit",
  "area": "lifecycle",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Intercept close, finish any renderer handshake with a timeout, allow BrowserWindow.close and then SIGKILL the process, because the reviewed host may not exit through app.quit.",
  "when_to_use": "After closing the only window the process survives, shutdown handlers never finish, or a timer meant to kill the process never fires after app.quit().",
  "applicability": "One reviewed Electron port on a 37.2.x host and Electron 37.2.0 on OH PC. Comments state app.quit stalls the event loop there. Confirm on your runtime first, since a forced kill skips normal cleanup.",
  "keywords": [
    "window close",
    "app.quit",
    "event loop stuck",
    "SIGKILL",
    "process.kill",
    "closing_window",
    "terminateSelf",
    "reload after close",
    "process survives"
  ],
  "related": [
    "electron-ohos-platform-capability-gates",
    "electron-ohos-process-isolation",
    "foreground-recovery-and-save-lifetime"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# End an OH Electron app when window close does not exit it

## When to use

After closing the only window the process survives, shutdown handlers never finish, or a timer meant to kill the process never fires after app.quit().

## Recommended approach

Intercept close, finish any renderer handshake with a timeout, allow BrowserWindow.close and then SIGKILL the process, because the reviewed host may not exit through app.quit.

## Implementation steps

1. Reproduce: close the window and check whether the process and its children remain, and whether app.quit returns control.
2. In the close handler call preventDefault once, ask the renderer to save with a bounded timeout (12 s in the reviewed code), and proceed on timeout.
3. Flush and stop anything you own, because a kill runs no later hooks. Then call BrowserWindow.close, which the reviewed comment says reaches the host window close and terminateSelf.
4. Schedule process.kill(process.pid, 'SIGKILL') immediately after, without depending on a delayed timer after app.quit.
5. If you see the page reload rather than close, watch for a load after the closing state and treat it as exit, with a flag exempting reloads you started yourself.

## Conditions and alternatives

Do not port this to other platforms. The reviewed device logs show the intercept sequence, but the reload-after-close branch was never recorded in them, so that branch is defensive, not an observed failure.

## Common mistakes

Using app.quit plus setTimeout as the kill path. Killing before saved state is flushed. Treating every reload during close as a bounce without an exemption for intended reloads.

## Check it works

Close with unsaved work, a hung renderer and a quick reopen. Confirm the save handshake, no remaining processes, no crash record, and that a user-initiated reload still works.

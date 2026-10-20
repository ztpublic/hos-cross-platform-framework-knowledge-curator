---
{
  "schema_version": 1,
  "id": "electron-ohos-main-inspect",
  "title": "Inspect the Electron main process on OH",
  "topic_key": "host.electron-ohos-main-inspect",
  "area": "host",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Append --inspect to the host's Electron relaunch arguments, forward the port with hdc fport, and attach Chrome DevTools to debug the main process.",
  "when_to_use": "The OH Electron main process crashes or hangs before the renderer is usable, or renderer DevTools works but the main process cannot be inspected.",
  "keywords": [
    "Electron",
    "debugging",
    "inspect",
    "9229",
    "hdc fport",
    "DevTools",
    "main process",
    "WebWindow",
    "launch args"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Debug builds of an OH Electron app whose host exposes Electron launch arguments. The debug port must not ship in release builds.",
  "related": [
    "electron-ohos-statusbar-openurl",
    "electron-ohos-older-runtime-shims",
    "electron-ohos-window-types"
  ]
}
---

# Inspect the Electron main process on OH

## When to use

The OH Electron main process crashes or hangs before the renderer is usable, or renderer DevTools works but the main process cannot be inspected.

## Recommended approach

Append `--inspect=9229` to the Electron relaunch arguments in the host, forward the port with `hdc fport`, and attach Chrome DevTools to the main process.

## Implementation steps

1. Locate where the host builds the Electron launch argument vector (in the template this is `buildArgs()` in the web engine's `WebWindow`), and append `--inspect=9229` to the app-provided relaunch arguments.
2. Keep the bundle-installation argument and any required host arguments in their original order; only append the inspect flag.
3. Rebuild, sign and install the debug HAP.
4. Forward the device port: `hdc fport tcp:9229 tcp:9229`.
5. Open `chrome://inspect`, configure `localhost:9229`, and attach to the Electron main target; use the same flow for a second debug port if needed.
6. Capture the console/stack from the main process, and remove the inspect flag from release builds and CI artifacts.

## Conditions and alternatives

The debug port requires a debuggable build and a working `hdc` connection. If the host does not expose launch arguments, add a debug-only hook rather than patching the engine binary. Renderer DevTools and main-process inspection are separate targets.

## Common mistakes

- Leaving `--inspect` in a release build and exposing the port.
- Forgetting `hdc fport`, then expecting localhost:9229 to connect.
- Appending the flag before the host's required arguments and breaking startup.
- Trying to inspect the main process from renderer DevTools.
- Reusing a stale installed HAP after changing the launch arguments.

## Check it works

Install the debug HAP, forward the port, attach DevTools and confirm the main target appears. Trigger a known main-process error and see the stack. Rebuild without the inspect flag for release and confirm the debug port is not open.

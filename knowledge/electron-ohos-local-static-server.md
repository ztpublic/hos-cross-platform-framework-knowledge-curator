---
{
  "schema_version": 1,
  "id": "electron-ohos-local-static-server",
  "title": "Serve a web IDE from inside the Electron app",
  "topic_key": "host.electron-ohos-local-static-server",
  "area": "host",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Run a loopback HTTP server in the Electron main process that serves the bundled frontend and a minimal compatible REST surface, then load it in the app window.",
  "when_to_use": "A desktop web IDE depends on a local Python or Node server, but the OH device runtime lacks the native extension stack or cannot run the desktop service model.",
  "keywords": [
    "Electron",
    "local server",
    "node-static",
    "loopback",
    "REST",
    "Pyodide",
    "Skulpt",
    "notebook",
    "web IDE",
    "native extensions"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH Electron apps that bundle a web frontend and need a local service, when the native backend cannot run on the device. The served REST surface must match what the frontend actually calls.",
  "related": [
    "electron-ohos-runtime-and-jit",
    "electron-ohos-process-isolation",
    "electron-ohos-private-hnp"
  ]
}
---

# Serve a web IDE from inside the Electron app

## When to use

A desktop web IDE depends on a local Python or Node server, but the OH device runtime lacks the native extension stack or cannot run the desktop service model.

## Recommended approach

Run a small HTTP server inside the Electron main process that serves the bundled frontend and a minimal compatible REST surface, then load `127.0.0.1` in the BrowserWindow. Keep an optional external-server mode; avoid native Python extensions where they crash.

## Implementation steps

1. Bundle the frontend assets and the browser-side kernel runtime in the app resource directory.
2. Start an HTTP server on loopback with a token and a bounded port selection; serve static files with correct MIME types and no-store caching.
3. Implement only the REST endpoints the frontend needs (contents, sessions, kernels, settings/workspaces); keep state in memory or under `userData`.
4. Execute Python in-browser through the bundled kernel (Pyodide or similar), installing a small prelude for the IPython APIs the frontend expects; disable frontend extensions that require a real server.
5. Start the server, wait for readiness, then load the URL; on failure show an error page with the captured server tail instead of a blank window.
6. Support an external mode that skips the local server and loads a configured server URL.
7. Shut down the server and workers on quit, and handle port conflicts and repeated launches.

## Conditions and alternatives

The local REST surface is a compatibility shim; the frontend may use more endpoints than the initial screen reveals. A device Python or external server mode is the alternative when the full backend is required. Native extensions may still crash even in a subprocess.

## Common mistakes

- Requiring the full Python native-extension stack before the UI can start.
- Serving without a token or on a non-loopback interface.
- Blocking the Electron main thread with synchronous server work.
- Using a fixed port that is already occupied.
- Leaving the server or workers running after quit.
- Treating a visible window as proof that notebook execution works.

## Check it works

Launch offline, open a notebook, execute a Python cell, save and reload it. Switch to external mode and confirm it skips the local server. Quit and verify no server process or port remains; relaunch and confirm the port/token handling is stable.

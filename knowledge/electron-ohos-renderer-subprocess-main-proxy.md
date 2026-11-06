---
{
  "schema_version": 1,
  "id": "electron-ohos-renderer-subprocess-main-proxy",
  "title": "Run renderer subprocess calls from the OH Electron main process",
  "topic_key": "process.electron-ohos-renderer-subprocess-main-proxy",
  "area": "process",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "If a renderer cannot execute a packaged tool, replace its child_process with an IPC proxy and spawn in the main process, streaming stdio and exit back.",
  "when_to_use": "execFile or spawn from a renderer reports ENOENT for a tool in a private HNP, though the same path runs from the main process.",
  "applicability": "Electron OH apps with node integration whose renderer libraries spawn tools. Source comment attributes this to the render sandbox not seeing the HNP directory. A cross-process design has a security surface of its own, so limit it to known executables.",
  "keywords": [
    "renderer spawn ENOENT",
    "child_process",
    "execFile",
    "HNP",
    "ipcMain",
    "webpack loader",
    "NormalModuleReplacementPlugin",
    "dugite",
    "render sandbox"
  ],
  "related": [
    "electron-ohos-non-pty-exec",
    "electron-ohos-private-hnp",
    "electron-ohos-process-isolation"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Run renderer subprocess calls from the OH Electron main process

## When to use

execFile or spawn from a renderer reports ENOENT for a tool in a private HNP, though the same path runs from the main process.

## Recommended approach

If a renderer cannot execute a packaged tool, replace its child_process with an IPC proxy and spawn in the main process, streaming stdio and exit back.

## Implementation steps

1. Confirm with a test: spawn the same absolute path from main and renderer.
2. In the renderer bundle, swap the library's require of child_process with a webpack loader, as NormalModuleReplacementPlugin does not apply to a Node builtin.
3. Implement spawn in main behind ipcMain, sending stdout, stderr and exit events per request ID and accepting stdin writes.
4. Kill children when the sender fires destroyed, and restrict what can be launched to your packaged tools.
5. Fill missing variables such as HOME, PATH and CA paths from main's environment. Replace cwd only where it is a neutral bundle location, so a really missing repository still returns ENOENT.

## Conditions and alternatives

If you control the code, move the work into main or a utility service and call it explicitly, which is cleaner than emulating child_process. The source reports clone, commit and push on a device in its changelog.

## Common mistakes

Exposing a general-purpose spawn to page content. Mapping every missing cwd to a safe path, which hides real errors. Leaving children alive after window destruction.

## Check it works

From the renderer run success, non-zero exit, a missing cwd, large stdout, stdin input and a window closed mid-run. Confirm results match desktop and no process remains.

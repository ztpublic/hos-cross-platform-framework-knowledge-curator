---
{
  "schema_version": 1,
  "id": "electron-ohos-non-pty-exec",
  "title": "Execute commands without a PTY on OH",
  "topic_key": "lifecycle.electron-ohos-non-pty-exec",
  "area": "host",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Use a resident non-PTY shell with pipes and unguessable completion markers, because PTY allocation is denied and child exit events are not delivered.",
  "when_to_use": "A terminal or command feature relies on node-pty, PTY allocation fails, child processes never report exit, or zombie processes accumulate.",
  "keywords": [
    "PTY",
    "node-pty",
    "forkpty",
    "openpty",
    "SELinux",
    "child_process.spawn",
    "sentinel",
    "zombie",
    "command execution",
    "shell",
    "Electron"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH app sandboxes where PTY allocation is denied but non-PTY fork/exec and pipes remain available. Provides batch command execution, not interactive terminal semantics.",
  "related": [
    "electron-ohos-runtime-and-jit",
    "async-bridge-shutdown-wait",
    "native-event-loop-handle-teardown"
  ]
}
---

# Execute commands without a PTY on OH

## When to use

A terminal or command feature relies on `node-pty`, PTY allocation fails, child processes never report exit, or zombie processes accumulate.

## Recommended approach

Use a single long-lived, non-PTY shell with pipes. Frame each command with unguessable begin/end markers and read completion and the exit status from the end marker, because child exit events are not delivered on this platform.

## Implementation steps

1. Probe PTY availability and treat a denial (`forkpty`/`openpty` rejected by SELinux) as final; do not fall back to another PTY API.
2. Spawn one resident `/system/bin/sh` with stdin/stdout/stderr pipes and reuse it for all commands. Serialize commands so output from one cannot be mistaken for another.
3. Before each command, print a begin marker containing a fresh random token. Run the command, capture stderr into stdout, capture `$?`, then print an end marker with the same token and the status.
4. Parse the stream until the matching end line. Treat the marker line as the only completion signal; `proc:exit` and `proc:close` may never fire, and `child_process.exec` callbacks may never run.
5. Set the working directory and environment explicitly. Bound the command by a timeout that still drains to the end marker, cap retained output, and reset the shell only as a last resort; each reset can leak a zombie.
6. Recognize that the shell is not a sandbox. Enforce path policy in the tool layer before dispatch, fail closed, and state that the fence is bypassable by shell syntax.

## Conditions and alternatives

Non-PTY `fork` + `execve` + pipes work, but there is no TTY, no job control and no reliable process-exit event. Interactive programs and tools that require a terminal are not supported. Apps that do not need command execution should omit the feature entirely.

## Common mistakes

- Spawning one process per command and waiting for an exit event that never arrives.
- Parsing output without a marker, so a command that prints no trailing newline corrupts the next result.
- Using a fixed marker that command output can forge.
- Treating a tool-layer path check as a kernel sandbox.

## Check it works

Run a multi-line command, a command with a non-zero exit code, and a command that exceeds its timeout. Confirm each returns the intended output and status, the next command is not confused by the previous output, and the process table shows no new zombie except after an explicit reset.

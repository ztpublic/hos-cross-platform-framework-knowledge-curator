---
{
  "schema_version": 1,
  "id": "electron-ohos-conditional-pty-adapter",
  "title": "Implement a PTY adapter only where the app can allocate one",
  "topic_key": "integration.electron-ohos-conditional-pty-adapter",
  "area": "integration",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Probe PTY capability in the actual app context, then implement only the terminal API subset supported by that runtime; retain a non-PTY or remote terminal fallback when allocation is denied.",
  "when_to_use": "A packaged OH Electron terminal needs node-pty behavior, but the desktop addon crashes when ptsname returns null or the selected app context denies PTY allocation.",
  "applicability": "OH Electron runtime and signed application identity with genuinely working forkpty/exec and permitted shell artifacts. Source device claims include different app identities; ordinary-app PTY availability is not established here.",
  "keywords": [
    "node-pty",
    "forkpty",
    "ptsname",
    "TIOCSWINSZ",
    "waitpid",
    "pthread",
    "napi_threadsafe_function",
    "terminal",
    "executableBinaryPaths"
  ],
  "related": [
    "electron-ohos-non-pty-exec",
    "electron-ohos-napi-symbol-owner",
    "native-dependency-closure"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Implement a PTY adapter only where the app can allocate one

## When to use

A packaged OH Electron terminal needs node-pty behavior, but the desktop addon crashes when ptsname returns null or the selected app context denies PTY allocation.

## Recommended approach

Probe PTY capability in the actual app context, then implement only the terminal API subset supported by that runtime; retain a non-PTY or remote terminal fallback when allocation is denied.

## Implementation steps

1. Exercise forkpty and shell startup inside the intended app using its own bundle identity and legitimate signing/profile. SDK exports or hdc shell success do not establish application sandbox permission.
2. If allocation is denied, keep the feature unavailable or use the established non-PTY/remote path. Do not adopt another application's privileged identity as a portability technique.
3. Where allocation works, audit the consumer's actual addon API subset. The reviewed custom adapter implements fork, resize and process lookup without relying on ptsname(master) for the terminal name.
4. Launch only the intended permitted shell with a controlled argument/environment/working-directory contract. Package/register executable artifacts as required by the selected product SDK; this is separate from PTY authorization.
5. Implement resize with the terminal size ioctl. Reap the child with waitpid, decode exit/signal status, and deliver completion to the correct JS thread through a retained TSFN.
6. Define failure and teardown paths for fork, exec, watcher creation, TSFN delivery, master descriptor close and engine destruction. A detached wait thread must not retain an invalid environment.

## Conditions and alternatives

This is a conditional PTY integration. The source reports device terminal success but also describes a system-app identity variant; the available report does not isolate which privilege/profile enables each behavior. Existing non-PTY guidance remains valid where allocation or child-exit delivery fails.

## Common mistakes

Do not equate compiled POSIX symbols with runtime permissions, dereference a null ptsname result, assume the full node-pty API is supported, or claim a non-PTY batch shell provides interactive terminal semantics.

## Check it works

On the target PC and intended app identity verify input/echo, resize, Unicode, EOF, shell exit/status, signals, route closure and engine teardown. Record product/API/profile and repeat with the unprivileged supported configuration before making portability claims.

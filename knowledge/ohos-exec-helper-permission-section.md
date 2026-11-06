---
{
  "schema_version": 1,
  "id": "ohos-exec-helper-permission-section",
  "title": "Build an exec-able OH helper with a permission section and no UI",
  "topic_key": "process.ohos-exec-helper-permission-section",
  "area": "process",
  "scope": "common",
  "frameworks": [],
  "summary": "Build a helper a tool spawns as a real ELF executable, add a .permission section with llvm-objcopy, and proxy any UI request over a local socket to the main app.",
  "when_to_use": "A tool such as ssh cannot launch its SSH_ASKPASS helper from an OH app, because the helper was built as a .so by the framework macro, lacks the manifest or tries to show its own UI.",
  "applicability": "OH PC apps that fork and exec a bundled non-UI binary under current app signing rules. Only build configuration and helper code were reviewed; execution on a device, ssh bundling and code-signing for your distribution channel were not established. Permission availability depends on product policy.",
  "keywords": [
    "SSH_ASKPASS",
    "execve",
    "helper executable",
    ".permission",
    "llvm-objcopy",
    "INHERIT_PARENT_PERMISSION",
    "hapsigner elf",
    "QLocalServer",
    "qt6_add_executable"
  ],
  "related": [
    "hap-executable-declaration",
    "electron-ohos-non-pty-exec",
    "jit-executable-memory-authorization"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Build an exec-able OH helper with a permission section and no UI

## When to use

A tool such as ssh cannot launch its SSH_ASKPASS helper from an OH app, because the helper was built as a .so by the framework macro, lacks the manifest or tries to show its own UI.

## Recommended approach

Build a helper a tool spawns as a real ELF executable, add a .permission section with llvm-objcopy, and proxy any UI request over a local socket to the main app.

## Implementation steps

1. Create the helper with plain add_executable, and confirm with readelf that it has an entry point. Framework executable macros such as qt6_add_executable can produce a shared library on OH.
2. Keep it free of UI libraries. Source comments state that UI-bearing executables cannot be run directly. Link libhilog_ndk.z.so if you need logs; the .z. in the name needs a full path or -l:libhilog_ndk.z.so.
3. Write a JSON manifest such as {"requestPermissions":[{"name":"ohos.permission.INHERIT_PARENT_PERMISSION"}]} and add it after linking with llvm-objcopy --add-section .permission=<file>. Fail, rather than warn, if the tool or file is missing in a release build.
4. Sign the ELF in the packaging step (hapsigner elf mode) and declare it as the executable-declaration entry requires.
5. For prompts, listen with a local socket in the main app and pass its path through an environment variable. The helper sends the prompt line and prints the reply; an empty reply means cancel. Never log the prompt, socket path or secret.

## Conditions and alternatives

Use a direct library API when one exists, since no helper is simpler. A UI-free helper only proxies to the running main app, so it fails when the app is not alive. Reachability of the socket path from the child process must be checked on the target product.

## Common mistakes

Skipping the objcopy step silently, as a warn-only build rule permits. Using qt6_add_executable for a program that must be exec-able. Storing the entered secret in logs or environment variables.

## Check it works

Inspect the packaged ELF for type, entry point, .permission section and signature. On a device run the real tool, enter correct and wrong passwords, cancel, and close the main window while the prompt is open.

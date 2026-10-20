---
{
  "schema_version": 2,
  "id": "terminal-guest-working-directory",
  "title": "Resolve terminal working directories in the selected host or guest namespace",
  "topic_key": "native.terminal-guest-working-directory",
  "area": "native",
  "keywords": ["terminal", "working directory", "QEMU", "vroot", "PTY", "VFS", "guest path", "directory not mounted", "终端目录", "来宾文件系统", "工作目录不存在"],
  "summary": "Select the terminal backend before resolving its working directory, and translate guest locators without sending them through the native host filesystem resolver.",
  "when_to_use": "An OH app offers both a native shell and a Linux guest terminal, but starting the guest terminal fails because its directory is treated as a missing native mount.",
  "status": "scoped"
}
---

# Resolve terminal working directories in the selected host or guest namespace

## When to use

An OH app offers both a native shell and a Linux guest terminal, but starting the guest terminal fails because its directory is treated as a missing native mount.

## Recommended approach

Select the terminal backend before resolving its working directory, and translate guest locators without sending them through the native host filesystem resolver. Keep app-owned storage resolution available for directories that belong to the host.

## Implementation steps

1. Represent the terminal implementation and supported interpreter type explicitly. Reject unsupported combinations before creating a session.
2. Let the selected terminal host own working-directory resolution. Pass it a separate callback for resolving app or host VFS directories rather than resolving every path centrally first.
3. Recognize a guest mount by its complete mount name and segment boundary. Map its root to the guest root and its descendants to guest-relative absolute paths. A similar-looking prefix must remain outside that mapping.
4. For native terminals and host-owned workspace locators, invoke the host resolver and preserve its permission or missing-mount error. A host path is usable inside a guest only if the guest launcher exposes a matching bind/mount arrangement.
5. Forward this resolution contract through wrappers that publish terminal state or manage sessions. Record the owning backend for each session and route later I/O and cleanup to that backend.
6. Include the selected backend, interpreter and requested directory in a startup error. Await startup and present the failure without substituting another directory or silently switching terminal backends.

## Conditions and alternatives

This applies to an application-managed guest runtime, such as a configured QEMU/vroot environment. Path translation does not install the guest, grant filesystem access or prove that a PTY can run on the target product. Verify the runtime assets, root filesystem and shared-directory mapping separately.

If there is only a native shell, use its host filesystem resolver directly. If a selected document URI cannot provide a path required by the terminal, use a supported access adapter or [an explicit app-owned import](sandbox-import-as-access-fallback.md).

## Common mistakes

Do not require a guest-only mount to exist in the host VFS. Do not strip prefixes without a segment-boundary check, reinterpret all absolute paths as guest paths, or hide a host permission error by falling back to the guest root. Choosing a guest shell does not make every interpreter type supported.

## Check it works

Test guest root and descendants, spaces in names, a similar-prefix host path, a shared workspace, native-shell startup, unsupported types, missing guest assets and failed mount resolution. Confirm the terminal's actual current directory and that session wrappers preserve the same resolution policy.

---
{
  "schema_version": 1,
  "id": "hap-executable-declaration",
  "title": "Diagnose EACCES when launching a packaged native executable",
  "topic_key": "native.hap-executable-declaration",
  "area": "native",
  "scope": "common",
  "frameworks": [],
  "summary": "Validate executable packaging, platform signing authorization, process-visible path and ELF dependencies as separate requirements.",
  "when_to_use": "An ELF executable exists in the HAP but execve returns EACCES or chmod returns EPERM.",
  "keywords": [
    "execve",
    "EACCES",
    "executableBinaryPaths",
    "EPERM",
    "执行权限"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Executable packaging and permission policy depend on the SDK/product. A normal .so loaded by dlopen is not automatically an executableBinaryPaths entry.",
  "related": []
}
---

# Diagnose EACCES when launching a packaged native executable

## When to use

An ELF executable exists in the HAP but execve returns EACCES or chmod returns EPERM.

## Recommended approach

Validate executable packaging, platform signing authorization, process-visible path and ELF dependencies as separate requirements.

## Implementation steps

1. Check the target SDK module schema for executableBinaryPaths and extraction requirements; declare only the binaries actually executed.
2. Rebuild and sign with the correct application identity and authorized profile, then inspect the installed execution attributes.
3. Resolve the binary from the application namespace and verify architecture, interpreter and dynamic dependencies.
4. Test launch in a clean installation or isolated test identity without removing valuable application data. Use documented runtime embedding when subprocess execution is unsuitable.

## Conditions and alternatives

Executable packaging and permission policy depend on the SDK/product. A normal .so loaded by dlopen is not automatically an executableBinaryPaths entry.

## Common mistakes

Do not chmod the installed bundle or assume a generic signing command grants every permission. Do not equate shell execution with app execution.

## Check it works

Check declaration, signing and native loader diagnostics independently; run the intended entrypoint with resources and child-process cleanup.

---
{
  "schema_version": 1,
  "id": "application-path-namespace",
  "title": "Diagnose paths in the process that actually uses them",
  "topic_key": "storage.application-path-namespace",
  "area": "storage",
  "scope": "common",
  "frameworks": [],
  "summary": "Resolve paths from the owning application context and inspect visibility in the actual process namespace.",
  "when_to_use": "A path visible through hdc shell is absent or inaccessible inside the application.",
  "keywords": [
    "namespace",
    "hdc",
    "mount",
    "路径"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to sandboxed PC and other device classes; specific mount prefixes are implementation details, not portable constants.",
  "related": [
    "sandbox-paths-and-app-identity",
    "terminal-guest-working-directory"
  ]
}
---

# Diagnose paths in the process that actually uses them

## When to use

A path visible through hdc shell is absent or inaccessible inside the application.

## Recommended approach

Resolve paths from the owning application context and inspect visibility in the actual process namespace.

## Implementation steps

1. Record which subject performs the operation: application, child process, shell or system service.
2. Resolve app bundle and data locations through that subject's platform API rather than importing shell absolute paths.
3. Pass resolved roots explicitly to native services and verify any child-process shared mount arrangement.
4. Separate not-found, denied access, loader rejection and missing native dependencies in diagnostics.

## Conditions and alternatives

Applies to sandboxed PC and other device classes; specific mount prefixes are implementation details, not portable constants.

## Common mistakes

A successful shell ls does not prove that application I/O or execution is permitted.

## Check it works

Test the same intended resource from the app and shell, then verify child-process access only where that route is used.

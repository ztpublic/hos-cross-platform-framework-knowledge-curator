---
{
  "schema_version": 1,
  "id": "windows-build-environment",
  "title": "Keep Windows build subprocess paths in the expected shell format",
  "topic_key": "build.windows-build-environment",
  "area": "build",
  "scope": "common",
  "frameworks": [],
  "summary": "Inspect the shell and environment actually inherited by the failing subprocess and use a compatible native command environment.",
  "when_to_use": "A Windows native build loses backslashes in tool paths or Hvigor cannot spawn Java despite an installed JBR.",
  "keywords": [
    "spawn java ENOENT",
    "Git Bash",
    "PATH",
    "mingw32-make"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to Windows build hosts; it does not require all OH builds to use one shell. MinGW make quoting and Windows process PATH are distinct diagnostics.",
  "related": []
}
---

# Keep Windows build subprocess paths in the expected shell format

## When to use

A Windows native build loses backslashes in tool paths or Hvigor cannot spawn Java despite an installed JBR.

## Recommended approach

Inspect the shell and environment actually inherited by the failing subprocess and use a compatible native command environment.

## Implementation steps

1. For make-generated Windows commands, check whether an unintended sh consumes backslashes; use the generator's supported Windows shell or regenerate correctly quoted commands.
2. For packaging Java, verify executable lookup from the same Windows process environment that runs Node/Hvigor, including the PATH separator convention.
3. Retain a complete JBR installation; setting JAVA_HOME alone may not fix a tool that searches PATH.
4. Rerun the failing stage and inspect its selected executable before changing source code.

## Conditions and alternatives

Applies to Windows build hosts; it does not require all OH builds to use one shell. MinGW make quoting and Windows process PATH are distinct diagnostics.

## Common mistakes

Do not replace all backslashes in a generated Makefile or infer a general Node spawn limitation from one PATH conversion failure.

## Check it works

Validate compiler and Java lookup in the chosen host environment and complete native compilation plus packaging.

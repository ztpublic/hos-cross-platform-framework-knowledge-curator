---
{
  "schema_version": 1,
  "id": "hvigor-native-rebuild-evidence",
  "title": "Verify that a requested native rebuild actually ran",
  "topic_key": "build.hvigor-native-rebuild-evidence",
  "area": "build",
  "scope": "common",
  "frameworks": [],
  "summary": "Confirm toolchain inputs, task execution and artifact hashes before treating a successful packaging command as a native rebuild.",
  "when_to_use": "Hvigor finishes quickly but packages stale native libraries or skips the intended native build.",
  "keywords": [
    "BuildNativeWithNinja",
    "stale",
    "Hvigor",
    "native skip"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to Hvigor-driven framework hosts; CLI and IDE success need equivalent inputs and output checks.",
  "related": []
}
---

# Verify that a requested native rebuild actually ran

## When to use

Hvigor finishes quickly but packages stale native libraries or skips the intended native build.

## Recommended approach

Confirm toolchain inputs, task execution and artifact hashes before treating a successful packaging command as a native rebuild.

## Implementation steps

1. Inspect selected SDK/JBR and externalNativeOptions configuration in the actual build environment.
2. Compare native task logs and output hashes with the edited source or intended prebuilt input.
3. If stale derived state is demonstrated, regenerate only the relevant build output after preserving source, user data and signing configuration.
4. Verify the new artifact is included in the final package and launched.

## Conditions and alternatives

Applies to Hvigor-driven framework hosts; CLI and IDE success need equivalent inputs and output checks.

## Common mistakes

Do not copy only java.exe out of its runtime, blindly clear source directories, or conclude CLI packaging is universally broken from one failing setup.

## Check it works

Use a traceable native change, observe compilation and final packaged hash, then verify launch with the rebuilt artifact.

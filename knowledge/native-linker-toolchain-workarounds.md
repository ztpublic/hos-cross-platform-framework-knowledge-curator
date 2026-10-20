---
{
  "schema_version": 1,
  "id": "native-linker-toolchain-workarounds",
  "topic_key": "native.native-linker-toolchain-workarounds",
  "scope": "common",
  "frameworks": [],
  "title": "Resolve an OH linker or optimization-probe incompatibility",
  "area": "native",
  "summary": "Reduce the failure to the specific toolchain interaction and change only the incompatible flag or probe.",
  "when_to_use": "A native build fails in an optimization/link step that succeeds on another platform, or a nested CMake probe cannot use the OH toolchain.",
  "keywords": [
    "CMake",
    "ICF",
    "LTO",
    "IPO",
    "linker",
    "crtn.o",
    "SHT_LLVM_ADDRSIG",
    "链接器",
    "ICF crtn.o",
    "IPO probe fails",
    "OH链接失败"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": []
}
---

# Resolve an OH linker or optimization-probe incompatibility

## When to use

A native build fails in an optimization/link step that succeeds on another platform, or a nested CMake probe cannot use the OH toolchain.

## Recommended approach

Reduce the failure to the specific toolchain interaction and change only the incompatible flag or probe.

## Implementation steps

1. Preserve the full compile/link command and the first relevant diagnostic. Identify whether the failure occurs in a configuration probe or the final artifact link.
2. Check which compiler, linker, runtime objects and sysroot the failing command actually uses; nested probes may differ from the main build.
3. If the error identifies identical-code-folding interaction with runtime object metadata, test disabling that ICF mode without disabling unrelated optimization globally.
4. If only an IPO/LTO capability probe is invalid, repair or narrowly bypass the probe for that toolchain; do not assume all final-link optimization is unsupported.
5. Keep the workaround conditional on the verified toolchain/symptom and reevaluate it after upgrades.

## Conditions and alternatives

These are diagnostic branches, not default OH compiler settings. A workaround is justified by a matching failure, not merely by the target being OH.

## Common mistakes

Disabling all optimization can hide the cause and change size/performance unnecessarily. A successful configuration probe is also not proof the final library links.

## Check it works

Rebuild the final artifact, inspect ABI/dependencies and run native functionality. Compare size/performance when changing optimization behavior.

---
{
  "schema_version": 1,
  "id": "native-ohos-shared-library-elf-layout",
  "title": "Preserve the ELF layout when reducing OH shared-library size",
  "topic_key": "native.native-ohos-shared-library-elf-layout",
  "area": "native",
  "scope": "common",
  "frameworks": [],
  "summary": "Strip staged native debug data with the OH SDK tool while preserving a valid ELF section table; compress HAP ZIP entries rather than packing the shared-library payload.",
  "when_to_use": "An OH HAP contains the expected native library but loading fails with Invalid argument before any native export initializes, especially after UPX packing.",
  "keywords": [
    "UPX",
    "ELF",
    "section table",
    "Invalid argument",
    "dlopen",
    "compressNativeLibs",
    "llvm-strip",
    "native library compression"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH native shared libraries on loader versions with the observed section-table requirement. The source reports emulator execution; physical PC loader and install behavior need separate checks.",
  "related": [
    "native-dependency-completeness",
    "tauri-ohos-ability-host",
    "ohos-native-toolchain-inputs"
  ]
}
---

# Preserve the ELF layout when reducing OH shared-library size

## When to use

An OH HAP contains the expected native library but loading fails with `Invalid argument` before any native export initializes, especially after UPX packing.

## Recommended approach

Strip staged native debug data with the OH SDK tool while preserving a valid ELF section table; compress HAP ZIP entries rather than packing the shared-library payload.

## Implementation steps

1. Compare the original target `.so` with the staged and HAP-contained bytes. Identify whether a post-link packer transformed the ELF payload.
2. Disable UPX-style packing for the OH shared library on the affected loader. An executable packaging path can have a different contract; success there does not establish shared-library compatibility.
3. Run the target SDK's `llvm-strip --strip-unneeded` on a staged copy, preserving Cargo's cached artifacts and the ordinary ELF layout.
4. Inspect ELF class, endianness and machine as well as the section table. For the reviewed ELF64 little-endian guard, require a nonzero section offset/count, 64-byte section entries and an end offset within the library size. That guard is specific to this artifact format, not a complete validator for every ELF encoding.
5. When supported by the chosen OH packaging toolchain, set `module.compressNativeLibs` to compress the archive entries and let installation extract intact libraries. Do not confuse ZIP compression with an executable packer that rewrites the ELF.
6. Inspect the final HAP before signing and retain the resulting native-library hash. Repeat integrity inspection after signing or repackaging if the payload changes.

## Conditions and alternatives

The section-table requirement and UPX-related loader failure come from a source report and its packaging guard. They were not reproduced during curation. SDK stripping, ZIP compression support and actual extraction/loading must be validated on the selected product and OS version. A valid header check does not prove imports, relocations or application initialization are correct.

## Common mistakes

Packing `.so` files with the same options used for CLI executables; verifying only architecture; stripping the shared Cargo cache through a staged symlink; assuming a smaller valid ZIP contains a loadable ELF; claiming PC execution from an emulator report.

## Check it works

Inspect and hash the staged and HAP-contained library, confirm the section table is in bounds and confirm required dependencies. On the intended PC, load the module, observe a native initialization marker and exercise an actual feature. Compare the stripped and unmodified builds if the loader still fails.

---
{
  "schema_version": 1,
  "id": "ohos-musl-symbol-interposition-localize",
  "title": "Localize bundled C symbol names that an OH system library also exports",
  "topic_key": "native-linking.ohos-musl-symbol-interposition-localize",
  "area": "native-linking",
  "scope": "common",
  "frameworks": [],
  "summary": "Hide a bundled dependency's private C symbols with a linker version script, because the OH musl loader does not honor DF_SYMBOLIC and a same-named system export can replace your internal calls.",
  "when_to_use": "A library crashes in its own bundled code, such as SIGSEGV in qiodevice64_seek_file_func, or tries to allocate tens of GiB and is SIGKILLed, though the same build logic works with glibc and -Bsymbolic.",
  "applicability": "Native shared libraries on OH that bundle a copy of code also exported by a system library, such as minizip inside a zip wrapper versus the system libshared_libz.z.so. The cause is stated in the source comments, not independently traced. Applies across frameworks.",
  "keywords": [
    "SIGSEGV",
    "DF_SYMBOLIC",
    "-Bsymbolic",
    "musl",
    "symbol interposition",
    "version script",
    "minizip",
    "libshared_libz",
    "unzGoToFirstFile",
    "llvm-readelf",
    "QuaZip"
  ],
  "related": [
    "qt-ohos-system-lib-duplication",
    "native-dependency-closure",
    "musl-portable-libc-boundaries"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Localize bundled C symbol names that an OH system library also exports

## When to use

A library crashes in its own bundled code, such as SIGSEGV in qiodevice64_seek_file_func, or tries to allocate tens of GiB and is SIGKILLed, though the same build logic works with glibc and -Bsymbolic.

## Recommended approach

Hide a bundled dependency's private C symbols with a linker version script, because the OH musl loader does not honor DF_SYMBOLIC and a same-named system export can replace your internal calls.

## Implementation steps

1. Check the crashing library with llvm-readelf --dyn-syms and look for plain C names also exported by loaded system libraries.
2. Write a version script that marks only those names local, for example unz*, zip* and fill_*filefunc*, with the rest of the API left global.
3. Link with -Wl,--version-script=<file>. Do not rely on -Bsymbolic alone on OH.
4. Add post-link checks that fail the build if a private symbol is still GLOBAL, or if a known public C++ symbol stopped being exported.
5. Consider using one implementation only if the system library has a stable, documented ABI you can target.

## Conditions and alternatives

Static linking with hidden visibility is equivalent when you control the build. A rename at compile time also works, with more patching. Localization only protects calls inside that library; other libraries using the same names are unaffected.

## Common mistakes

Broad wildcards such as *qiodevice* also match mangled C++ names and make dependents fail to load. Assuming a glibc-tested -Bsymbolic link is safe. Blaming a corrupt archive for garbage sizes from a mismatched struct layout.

## Check it works

Inspect dynamic symbols of the linked library, then load real archives on a device while monitoring RSS. Confirm dependents still load and the private symbols are absent from the dynamic table.

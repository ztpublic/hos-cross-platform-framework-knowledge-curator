---
{
  "schema_version": 1,
  "id": "kmp-ohos-lldb-native-attach",
  "title": "Attach LLDB to Kotlin/Native code on an OH device",
  "topic_key": "debugging.kmp-ohos-lldb-native-attach",
  "area": "debugging",
  "scope": "framework-specific",
  "frameworks": [
    "kotlin-multiplatform"
  ],
  "summary": "Connect through the remote-ohos platform and point exec search paths at unstripped libraries, because stripped HAP libraries leave Kotlin breakpoints unbound.",
  "when_to_use": "Kotlin/Native breakpoints never bind on the device, or stepping skips cinterop code.",
  "applicability": "Kotlin/Native on OH with an SDK LLDB and an lldb-server on the device. Taken from an IDE plugin's attach pipeline, and paths depend on hvigor and plugin versions. The pipeline includes device server steps not repeated here.",
  "keywords": [
    "LLDB",
    "lldb-server",
    "remote-ohos",
    "platform.sock",
    "exec-search-paths",
    "source-map",
    "Kotlin/Native breakpoint",
    "step-in-avoid-nodebug",
    "aa process -D"
  ],
  "related": [
    "kmp-ohos-compose-host",
    "ohos-native-toolchain-inputs",
    "hvigor-native-rebuild-evidence"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Attach LLDB to Kotlin/Native code on an OH device

## When to use

Kotlin/Native breakpoints never bind on the device, or stepping skips cinterop code.

## Recommended approach

Connect through the remote-ohos platform and point exec search paths at unstripped libraries, because stripped HAP libraries leave Kotlin breakpoints unbound.

## Implementation steps

1. Run platform select remote-ohos, then platform connect unix-abstract-connect:///lldb-server/platform.sock, with lldb-server running on the device.
2. Clear target.exec-search-paths and target.source-map if LLDB stays alive across sessions, as old paths persist.
3. Add unstripped library locations to exec-search-paths: hvigor intermediates/libs/default/<abi>, HAR libraries, entry/libs and the KMP debugShared output.
4. Set target.process.thread.step-in-avoid-nodebug false to stop inside sourceless cinterop stubs.
5. With aa process -D, expect libentry.so to load only after the first continue, so set pending breakpoints and continue once.

## Conditions and alternatives

Debug-signed builds are required for attach. Prefer an existing IDE integration where it is available. Exact setting names should be checked against your LLDB.

## Common mistakes

Using stripped libraries from the installed HAP. Keeping search paths from a previous project. Concluding breakpoints are broken before the library has loaded.

## Check it works

Set breakpoints in Kotlin shared code, a cinterop function and a C++ wrapper, attach with a fresh and a reused LLDB session, then step between them.

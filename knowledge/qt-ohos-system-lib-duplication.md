---
{
  "schema_version": 1,
  "id": "qt-ohos-system-lib-duplication",
  "title": "Keep OH system libraries out of the HAP",
  "topic_key": "native.qt-ohos-system-lib-duplication",
  "area": "native",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Bundling device-provided libraries can make Qt initialization crash with SIGSEGV; resolve system libraries from the system partition and ship only application, Qt and C++ runtime libraries.",
  "when_to_use": "A Qt HAP exits immediately with a segmentation fault during Qt initialization after system libraries were copied into the package to satisfy DT_NEEDED entries.",
  "keywords": [
    "SIGSEGV",
    "DT_NEEDED",
    "libohenvironment.so",
    "libicu.so",
    "libace_napi.z.so",
    "system library",
    "HAP",
    "startup crash",
    "libc++_shared"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH 2in1/tablet packages that bundle native libraries. The correct set depends on the device image and SDK; verify each library rather than excluding by name.",
  "related": [
    "native-dependency-closure",
    "qt-plugin-deployment-layout",
    "qt-ohos-plugin-loading"
  ]
}
---

# Keep OH system libraries out of the HAP

## When to use

A Qt HAP exits immediately with a segmentation fault during Qt initialization after system libraries were copied into the package to satisfy `DT_NEEDED` entries.

## Recommended approach

Do not redistribute libraries that the device already provides. Bundle only application, Qt and C++ runtime libraries, and let the dynamic loader resolve system libraries from the system partition.

## Implementation steps

1. List every `.so` in the HAP and classify it as application, Qt runtime, C++ runtime, or system-provided.
2. Remove system-provided libraries such as the OH environment, ICU, NAPI, hilog and deviceinfo libraries from the package. Keep system resolution for their `DT_NEEDED` entries.
3. Keep the C++ runtime only when the toolchain requires it, and keep the Qt modules the app actually loads.
4. Rebuild and inspect the packaged ELF dependencies. Use a known-good application library as a control: if the same packaging chain crashes with it, the package contents, not the app, are the cause.
5. Verify startup on a device and check that unresolved symbols produce a loader error rather than a silent crash.

## Conditions and alternatives

System libraries must match the device's ABI and version; bundling a copy can override the system version with an incompatible one. Excluding too much causes missing-library or missing-symbol failures, so verify each library's origin instead of excluding by name alone.

## Common mistakes

- Bundling every `DT_NEEDED` entry to make the build pass.
- Assuming a library with the same name is interchangeable with the device copy.
- Removing the C++ runtime that the app itself needs.
- Debugging a startup SIGSEGV as an app logic bug when a packaging control would isolate it.

## Check it works

Install on a 2in1 device and confirm the process starts without SIGSEGV. Compare the HAP's native libraries with the system set, and confirm the remaining `DT_NEEDED` entries resolve. Repeat the known-good control build through the same packaging chain.

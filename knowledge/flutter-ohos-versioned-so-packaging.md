---
{
  "schema_version": 1,
  "id": "flutter-ohos-versioned-so-packaging",
  "title": "Package versioned native libraries in a Flutter OH build",
  "topic_key": "build.flutter-ohos-versioned-so-packaging",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "summary": "Patch the OH toolchain's native-asset copy filter so libraries such as libmpv.so.2 are packaged, then verify the HAP library set and the AOT engine artifacts.",
  "when_to_use": "A Flutter OH HAP builds successfully but white-screens or crashes at startup because a versioned native library such as libmpv.so.2 is missing from the packaged libs.",
  "keywords": [
    "Flutter",
    "OHOS",
    "native assets",
    "libmpv.so.2",
    "flutter_tools",
    "hvigor",
    "so packaging",
    "white screen",
    "AOT",
    "gen_snapshot"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Flutter OH builds whose native dependencies use versioned shared-library names. The patch anchors are tied to the oh-flutter SDK version.",
  "related": [
    "flutter-ohos-pc-form-factor",
    "native-dependency-closure",
    "native-code-load-location"
  ]
}
---

# Package versioned native libraries in a Flutter OH build

## When to use

A Flutter OH HAP builds successfully but white-screens or crashes at startup because a versioned native library such as `libmpv.so.2` is missing from the packaged libs.

## Recommended approach

Patch the OH toolchain's native-asset copy filter so libraries such as `libmpv.so.2` are packaged, then verify the HAP library set and the AOT engine artifacts.

## Implementation steps

1. Compare the native assets output directory with the HAP's `entry/libs/<abi>` contents; list every `.so` and `.so.<number>` file and find the missing ones.
2. Patch both copy sites in the oh-flutter SDK: `ohos_builder.dart` and the hvigor plugin's native-asset loop. Replace an `endsWith('.so')` test with a suffix regex such as `\.so(\.\d+)?$`.
3. Delete the flutter_tools stamp/snapshot so the patched tool is rebuilt, then rebuild the HAP.
4. Re-inspect the HAP and confirm the versioned libraries and their dependency closure are present; do not rename `libfoo.so.2` to `libfoo.so` because the SONAME/loader contract expects the versioned name.
5. If a release/profile build fails with a kernel or snapshot version error, align the `gen_snapshot` artifact and engine HAR versions as a separate compatibility fix.
6. Keep the SDK patch idempotent and re-apply it after an SDK update; fail loudly when the anchor text is missing.

## Conditions and alternatives

Only versioned native libraries are affected; plain `.so` names already pass. The patch points are specific to the oh-flutter toolchain version. A custom build step that copies native assets after the toolchain can be an alternative, but it must run before HAP packaging.

## Common mistakes

- Renaming a versioned library and breaking its SONAME.
- Patching one copy site and leaving the other unchanged.
- Rebuilding without clearing the flutter_tools cache, so the old filter still runs.
- Ignoring a separate AOT/gen_snapshot mismatch because the library issue was fixed.
- Verifying only the build success and not the packaged HAP contents.

## Check it works

Install the HAP and confirm startup no longer reports a missing library. Inspect the packaged libs for the versioned filename and its closure. Rebuild after an SDK update with the patch re-applied and repeat the check.

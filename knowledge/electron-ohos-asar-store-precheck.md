---
{
  "schema_version": 1,
  "id": "electron-ohos-asar-store-precheck",
  "title": "Ship the OH Electron payload as app.asar to clear source-file checks",
  "topic_key": "packaging.electron-ohos-asar-store-precheck",
  "area": "packaging",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Pack the application into one resources/app.asar with the upstream builder file exclusions instead of loose resfile sources, and fail the build if any source file remains in the HAP.",
  "when_to_use": "Store pre-review warns that the package must not contain source files, and the HAP holds thousands of loose .js files under resources/resfile/resources/app.",
  "applicability": "Electron OH hosts whose runtime accepts --app-path pointing at an asar. Store policy and check wording belong to the distribution channel and can change. Native addons and files opened by path may need to stay outside the archive.",
  "keywords": [
    "app.asar",
    "asar",
    "source files in package",
    "软件包中不应包含源码文件",
    "resfile",
    "AGC pre-review",
    "AppGallery"
  ],
  "related": [
    "electron-ohos-builder-packaging",
    "electron-ohos-resfile-injection",
    "electron-ohos-native-addon-abi"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Ship the OH Electron payload as app.asar to clear source-file checks

## When to use

Store pre-review warns that the package must not contain source files, and the HAP holds thousands of loose .js files under resources/resfile/resources/app.

## Recommended approach

Pack the application into one resources/app.asar with the upstream builder file exclusions instead of loose resfile sources, and fail the build if any source file remains in the HAP.

## Implementation steps

1. Apply the desktop build's asar and files configuration to the OH staging step, so development sources, source maps and test assets are excluded for the same reasons as on desktop.
2. Write the payload to resources/app.asar. Keep only files that must remain loose, such as license texts, next to it and remove the old loose app directory so a stale tree cannot be packaged.
3. Check that the host starts Electron with the asar as its app path, and that any code that resolves files under __dirname still works inside the archive.
4. Add a build check that lists HAP entries and rejects .js, .ts and similar source extensions outside the archive.
5. Record the asar digest in the build manifest so a rebuilt package can be compared with what was validated.

## Conditions and alternatives

An asar is packaging, not protection: its contents remain readable. Native .node files cannot be loaded from inside an archive without unpack support; see the addon and load-location entries. The reviewed staging script writes the archive itself to avoid a dependency, and the standard asar tool is an equally valid choice.

## Common mistakes

Adding app.asar while leaving the loose tree still fails the check. Dropping the upstream exclusion list ships development sources inside the archive. Assuming the check is cleared without re-running pre-review.

## Check it works

Unzip the built HAP and search for source extensions outside app.asar. Start the app cold and exercise file-relative features such as templates, printing and shape libraries, then re-run the store pre-review.

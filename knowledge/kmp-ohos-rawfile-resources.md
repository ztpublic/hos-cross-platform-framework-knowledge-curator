---
{
  "schema_version": 1,
  "id": "kmp-ohos-rawfile-resources",
  "title": "Load Compose rawfile images through the OH resource manager",
  "topic_key": "storage.kmp-ohos-rawfile-resources",
  "area": "storage",
  "scope": "framework-specific",
  "frameworks": [
    "kotlin-multiplatform"
  ],
  "summary": "Pass the UIAbility resource manager into Kotlin/Native through NAPI, read bundled rawfile images with the OH resource APIs, and decode them into Compose image bitmaps off the main thread.",
  "when_to_use": "Compose resources or images bundled in rawfile fail to load in a Kotlin/Native OH app, or the UI shows blank images even though the files exist in the HAP.",
  "keywords": [
    "Compose Resources",
    "rawfile",
    "NativeResourceManager",
    "OH_ResourceManager_OpenRawFile",
    "cinterop",
    "Kotlin/Native",
    "Skia",
    "ImageBitmap",
    "NAPI"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "KMP/Compose apps on OH that load bundled images from rawfile through Kotlin/Native. The resource manager lifetime and cinterop package are app-specific.",
  "related": [
    "kmp-ohos-dependency-and-renderer",
    "kmp-ohos-compose-host",
    "qt-ohos-resource-dir"
  ]
}
---

# Load Compose rawfile images through the OH resource manager

## When to use

Compose resources or images bundled in rawfile fail to load in a Kotlin/Native OH app, or the UI shows blank images even though the files exist in the HAP.

## Recommended approach

Pass the UIAbility resource manager into Kotlin/Native through NAPI, read bundled rawfile images with the OH resource APIs, and decode them into Compose image bitmaps off the main thread.

## Implementation steps

1. In the ArkTS ability, call the native initializer with `this.context.resourceManager` before the first composition.
2. In the NAPI module, convert the ArkTS object with `OH_ResourceManager_InitNativeResourceManager(env, args[0])` and pass the pointer to a Kotlin `@CName("initResourceManager")` function that stores it.
3. Declare a cinterop def for `raw_file_manager.h` and `raw_file.h`; import `OH_ResourceManager_OpenRawFile`, `OH_ResourceManager_GetRawFileSize`, `OH_ResourceManager_ReadRawFile` and `OH_ResourceManager_CloseRawFile`.
4. In Kotlin, open the rawfile by its resource path, read exactly the reported size into a pinned byte array, close the file, and decode with Skia into a Compose image bitmap.
5. Run the read/decode on an IO dispatcher and update Compose state when done; keep a 1×1 placeholder for null/empty results.
6. Resolve the rawfile path from the Compose resource item and handle a missing file without crashing the composition.
7. Close every opened rawfile and clear the stored manager when the ability is destroyed if its lifetime ends earlier than the process.

## Conditions and alternatives

The resource manager pointer is only valid while the ability/resource context is alive. A different image pipeline may use ArkUI image decoding instead of Skia. Large images should be downsampled rather than fully decoded into memory.

## Common mistakes

- Calling the reader before the resource manager is initialized.
- Reading a fixed buffer instead of the reported file size.
- Forgetting to close the rawfile on success and error paths.
- Decoding on the main thread and janking the UI.
- Using a Compose resource path that does not match the packaged rawfile path.

## Check it works

Display a bundled image on OH and confirm it renders with the expected dimensions. Request a missing resource and confirm the placeholder path instead of a crash. Repeat navigation and rotation to check that no rawfile or bitmap leaks, and verify the same screen still works on another platform.

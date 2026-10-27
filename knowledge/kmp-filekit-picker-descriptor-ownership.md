---
{
  "schema_version": 1,
  "id": "kmp-filekit-picker-descriptor-ownership",
  "title": "Keep picker files alive with owned descriptors in KMP",
  "topic_key": "storage.kmp-filekit-picker-descriptor-ownership",
  "area": "storage",
  "scope": "framework-specific",
  "frameworks": [
    "kotlin-multiplatform"
  ],
  "summary": "Open the granted URI through the OH file API, duplicate its descriptor into the KMP file object, and close each file or stream owner explicitly.",
  "when_to_use": "A Kotlin file API cannot read a selected OH URI as a sandbox path, or selection becomes unreadable after the native picker closes its file.",
  "applicability": "Experimental FileKit OH adapter with CPF Kotlin 2.2.21-1.0.0 and ohosArm64/ohosX64. PC file-provider grants, restart behavior and directory selection require separate verification.",
  "keywords": [
    "FileKit",
    "PlatformFile",
    "fromFileDescriptor",
    "dup",
    "pread",
    "pwrite",
    "Picker URI has no sandbox path",
    "bookmark",
    "close"
  ],
  "related": [
    "document-picker-uri-access",
    "sandbox-import-as-access-fallback",
    "picker-cancellation-concurrency"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Keep picker files alive with owned descriptors in KMP

## When to use

A Kotlin file API cannot read a selected OH URI as a sandbox path, or selection becomes unreadable after the native picker closes its file.

## Recommended approach

Open the granted URI through the OH file API, duplicate its descriptor into the KMP file object, and close each file or stream owner explicitly.

## Implementation steps

1. Initialize the dialog adapter on the ArkTS/UI thread with the current napi_env and owning UIAbilityContext before selecting. Dispose it from that ability's destruction callback on the same thread.
2. Use the OH file module to open the granted URI. Call PlatformFile.fromFileDescriptor(fd, uri, name), which duplicates the descriptor; the caller still closes the original OH file.
3. Read through source() or write through sink(). Each opened stream owns a further duplicate; close the stream even when the PlatformFile has already been closed.
4. Close the PlatformFile after use. Do not persist its /proc/self/fd/<number> representation or call toKotlinxIoPath() on a picker-backed object.
5. Copy into FileKit.filesDir before storing a bookmark or requiring restart durability. A descriptor number cannot preserve the original temporary grant across processes.

## Conditions and alternatives

The reviewed streams use pread/pwrite with private offsets and retry EINTR. This permits separate read positions despite dup sharing the underlying open file description. Directory selection separately checks whether the selected directory is accessible as a real path; it is not covered by the file-descriptor remedy. Camera, sharing and physical-device permission differences were not established by this review.

## Common mistakes

Do not construct PlatformFile(uri), treat ctime as creation time, serialize a live descriptor as a durable path, or rely on a cleaner to release scarce descriptors promptly.

## Check it works

On the intended PC, select external files, read with two simultaneous streams, close the parent object while one stream is still open, and check independent read positions. Test partial multi-selection failure, cancellation, missing grants and restart after an explicit sandbox copy.

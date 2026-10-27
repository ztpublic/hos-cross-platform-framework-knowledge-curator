---
{
  "schema_version": 1,
  "id": "kmp-native-image-core-without-compose",
  "title": "Reuse KMP image loading when the Compose decoder is unavailable",
  "topic_key": "media.kmp-native-image-core-without-compose",
  "area": "media",
  "scope": "framework-specific",
  "frameworks": [
    "kotlin-multiplatform"
  ],
  "summary": "Build the non-UI fetch/cache core for the OH target and send encoded image bytes to an ArkTS system decoder with an explicit feature contract.",
  "when_to_use": "A KMP image library cannot compile its Compose or Skia UI on the selected OH toolchain, while its fetch and cache logic remains reusable.",
  "applicability": "KMP libraries with separable fetch/cache and Compose/Skia layers. Requires compatible OH Kotlin, coroutines, IO and HTTP artifacts; PC display correctness must be measured on the selected decoder.",
  "keywords": [
    "Compose",
    "Skia",
    "Kamel",
    "PixelMap",
    "image kit",
    "ohosArm64",
    "ohosX64",
    "source sets",
    "base64",
    "runBlocking"
  ],
  "related": [
    "kmp-ohos-dependency-and-renderer",
    "native-curl-explicit-ca-bundle",
    "kmp-napi-string-allocator-ownership"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Reuse KMP image loading when the Compose decoder is unavailable

## When to use

A KMP image library cannot compile its Compose or Skia UI on the selected OH toolchain, while its fetch and cache logic remains reusable.

## Recommended approach

Build the non-UI fetch/cache core for the OH target and send encoded image bytes to an ArkTS system decoder with an explicit feature contract.

## Implementation steps

1. Audit source imports and separate Compose/Skia types from the reusable core. Place unsupported UI sources outside every active target source set; do not leave an indirect Compose dependency in the core API.
2. Add compatible OH targets and platform actuals for filesystem/cache and the HTTP engine. Align the compiler metadata generation across all OH dependencies.
3. Export the core through one Kotlin/Native bridge library and a registered C++ NAPI wrapper. Define byte payloads, cache metadata and errors rather than pretending to return a Compose Painter.
4. Run a synchronous runBlocking/network bridge on a supported ArkTS worker. Transfer serializable request/results and respect the native library's concurrency and shutdown contract.
5. Decode the returned encoded bytes with image.createImageSource and createPixelMap, then publish the PixelMap to ArkUI. Release replaced image sources and pixel maps.
6. Expose retained and lost features: raw-byte caching is distinct from bitmap caching; a single response lacks streaming progress, and a first GIF frame is distinct from animation.

## Conditions and alternatives

This restores a loading/caching capability through an ArkUI adapter. It does not establish compatibility of KamelImage, asyncPainterResource or a full Compose renderer. Base64 expands payloads and raises memory peaks; prefer bounded binary transfer when the bridge supports it.

## Common mistakes

Do not count compilation of JVM tests as OH runtime validation, label a worker-backed call nonblocking at the C ABI, or report a cache hit merely because the HTTP engine labels the result network.

## Check it works

Test HTTPS, corrupt images, cancellation/late responses, SVG differences, animated formats and cache behavior across process restart. Measure peak memory and verify resize/DPI behavior in the actual PC window.

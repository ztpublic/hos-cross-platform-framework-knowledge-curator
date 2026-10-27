---
{
  "schema_version": 1,
  "id": "kmp-ohos-coil-native-decoder",
  "title": "Register an OH native decoder in a KMP image loader",
  "topic_key": "media.kmp-ohos-coil-native-decoder",
  "area": "media",
  "scope": "framework-specific",
  "frameworks": [
    "kotlin-multiplatform"
  ],
  "summary": "Supply an OH-specific Coil Decoder.Factory through the platform component registry and bind the native ImageSource/PixelMap APIs with the matching Kotlin/Native target.",
  "when_to_use": "A KMP image loader has an OH target but no usable image decoder, or native image symbols fail to link.",
  "applicability": "Coil/OvCompose forks with an actual ohosArm64 target and platform registry hook. PC raster size, memory use, format/alpha and display behavior require separate tests.",
  "keywords": [
    "Coil",
    "OhosImageDecoder",
    "Decoder.Factory",
    "addOhosComponents",
    "OH_ImageSourceNative",
    "OH_PixelmapNative",
    "cinterop"
  ],
  "related": [
    "kmp-native-image-core-without-compose",
    "kmp-ohos-dependency-and-renderer",
    "hdr-output-surface-and-capability"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Register an OH native decoder in a KMP image loader

## When to use

A KMP image loader has an OH target but no usable image decoder, or native image symbols fail to link.

## Recommended approach

Supply an OH-specific Coil Decoder.Factory through the platform component registry and bind the native ImageSource/PixelMap APIs with the matching Kotlin/Native target.

## Implementation steps

1. Select an OH-capable Kotlin/Compose/Coil dependency cohort; keep the decoder in the OH source set and register its Factory through the platform component hook.
2. Define matching cinterop headers and native link libraries (`image_source` and `pixelmap` in the reviewed binding).
3. Create and release ImageSource, decoding options, PixelMap and info objects using explicit ownership and error checks; Kotlin memScoped alone does not release native resource objects.
4. Before transferring pixels to Skia, negotiate/query actual pixel format, alpha, dimensions and buffer size. Do not assume AUTO dynamic range yields RGBA8888 or that width×height×4 is always valid.

## Conditions and alternatives

The reviewed decoder demonstrates the integration hook but assumes four-byte RGBA and unpremultiplied alpha while requesting AUTO dynamic range. Its complete pixel conversion is not promoted as a validated recipe. Network fetcher support is a separate component.

## Common mistakes

A registered decoder does not prove safe HDR conversion, downsampling or correct network fetching; performance claims require measurement.

## Check it works

Test malformed/empty input, large dimensions, transparent images, SDR/HDR sources, repeated cancellation and missing-library packaging. Compare pixel output to a known reference.

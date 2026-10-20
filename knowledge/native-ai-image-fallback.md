---
{
  "schema_version": 1,
  "id": "native-ai-image-fallback",
  "topic_key": "integration.native-ai-image-fallback",
  "scope": "common",
  "frameworks": [],
  "title": "Add optional native image enhancement without blocking reading",
  "area": "integration",
  "summary": "Display the original content immediately, invoke enhancement through a bounded binary-data bridge, and replace the image only with a still-relevant successful result.",
  "when_to_use": "You want an OH-native AI image feature, but capability availability, latency or failures must not make the original image unusable.",
  "keywords": [
    "CoreVisionKit",
    "AI",
    "BinaryCodec",
    "BasicMessageChannel",
    "图像增强",
    "timeout",
    "AI enhancement fallback",
    "图像增强失败",
    "原图回退"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": []
}
---

# Add optional native image enhancement without blocking reading

## When to use

You want an OH-native AI image feature, but capability availability, latency or failures must not make the original image unusable.

## Recommended approach

Display the original content immediately, invoke enhancement through a bounded binary-data bridge, and replace the image only with a still-relevant successful result.

## Implementation steps

1. Probe the actual capability required by the feature; distinguish an installed channel handler from an available native service/model.
2. Keep original image data/rendering available as the default and failure fallback.
3. Use a suitable binary channel or native file/handle route for image data rather than unnecessary text encoding.
4. Tag work with image/session identity and define timeout/cancellation behavior. Ignore stale results after navigation or a settings change.
5. Precache or prepare the enhanced result before swapping it into the visible view to avoid flicker.
6. Bound cache/resource use and clean up failed or abandoned work.

## Conditions and alternatives

Named Flutter/Dart APIs below are adapter examples. The shared contract applies only through an implementation with equivalent native semantics; it does not establish other frameworks' support.

Native AI features depend on the device, product and SDK. A dedicated native renderer is an alternative only if it materially improves the required experience.

## Common mistakes

BinaryCodec does not itself establish zero-copy performance. Do not replace the original image with a blank loading state or let an old enhancement overwrite a new page.

## Check it works

Test unavailable capability, large images, timeout, rapid navigation, cache invalidation, output quality and measured memory/frame costs.

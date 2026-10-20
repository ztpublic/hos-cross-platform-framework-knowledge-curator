---
{
  "schema_version": 1,
  "id": "gallery-image-format-preservation",
  "title": "Preserve encoded image format across a file-based gallery API",
  "topic_key": "permissions.gallery-image-format-preservation",
  "area": "permissions",
  "scope": "common",
  "frameworks": [],
  "summary": "Match validated encoded bytes to suffix and MIME type, await gallery consumption and clean up the owned staging file.",
  "when_to_use": "An image saved through a temporary file is misclassified or loses its original format.",
  "keywords": [
    "gallery",
    "剪贴板",
    "READ_PASTEBOARD",
    "photo",
    "媒体保存",
    "clipboard",
    "gallery save failed",
    "clipboard denied",
    "相册保存",
    "剪贴板权限"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Permission requirements differ by operation, SDK and device. A clipboard text fallback can be valid if the UI clearly limits the feature to text.",
  "related": [
    "media-save-and-clipboard-permissions"
  ]
}
---

# Preserve encoded image format across a file-based gallery API

## When to use

An image saved through a temporary file is misclassified or loses its original format.

## Recommended approach

Match validated encoded bytes to suffix and MIME type, await gallery consumption and clean up the owned staging file.

## Implementation steps

When an OH gallery adapter requires a temporary file, identify an allowed encoded image format from validated bytes and use a matching suffix/MIME type. A PNG or GIF written unchanged under a JPEG filename can be misclassified. Retain original bytes when the operation promises lossless saving, await the actual gallery result, and clean up a uniquely owned temporary directory after the consumer finishes.

Normalize the selected backend's specific cancellation/permission result at the adapter boundary; keep unsupported format, invalid bytes and native failure separate. A historical error-message string is version-sensitive and must not become a blanket catch-to-cancel rule. Test supported formats, invalid/truncated payloads, cancellation and cleanup failure without masking the original save outcome.

## Conditions and alternatives

Permission requirements differ by operation, SDK and device. A clipboard text fallback can be valid if the UI clearly limits the feature to text.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Save PNG, GIF and JPEG bytes with matching metadata; test invalid/truncated input, cancellation, native failure and cleanup failure. Compare gallery bytes and staging lifetime.

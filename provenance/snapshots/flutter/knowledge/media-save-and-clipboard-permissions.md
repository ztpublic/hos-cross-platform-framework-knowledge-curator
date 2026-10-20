---
{
  "id": "media-save-and-clipboard-permissions",
  "title": "Adapt gallery saving and clipboard access to OH",
  "topic_key": "permissions.media-save-and-clipboard-permissions",
  "area": "permissions",
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
  "summary": "Choose the OH media/clipboard operation first, then request only the permissions and user interaction that operation requires.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "An Android-oriented photo-save or clipboard path fails on OH, requests irrelevant permissions, or silently supports fewer data types."
}
---

# Adapt gallery saving and clipboard access to OH

## When to use

An Android-oriented photo-save or clipboard path fails on OH, requests irrelevant permissions, or silently supports fewer data types.

## Recommended approach

Choose the OH media/clipboard operation first, then request only the permissions and user interaction that operation requires.

## Implementation steps

1. Specify the required data types: plain text versus clipboard files/images; one photo versus a batch of media assets.
2. Select an OH-capable plugin or native API implementing those types. Use the system media creation/selection dialog where that is the intended access route.
3. Declare permissions with appropriate user-facing reasons and request runtime grants only when needed. Keep Android-only permission branches on Android.
4. Handle denied, empty and cancelled results distinctly. For batches, preserve per-item outcomes rather than reporting the entire batch as successful.
5. Close native descriptors and release temporary resources on partial failure.

## Conditions and alternatives

Permission requirements differ by operation, SDK and device. A clipboard text fallback can be valid if the UI clearly limits the feature to text.

### Preserve encoded image format across a file-based gallery API

When an OH gallery adapter requires a temporary file, identify an allowed encoded image format from validated bytes and use a matching suffix/MIME type. A PNG or GIF written unchanged under a JPEG filename can be misclassified. Retain original bytes when the operation promises lossless saving, await the actual gallery result, and clean up a uniquely owned temporary directory after the consumer finishes.

Normalize the selected backend's specific cancellation/permission result at the adapter boundary; keep unsupported format, invalid bytes and native failure separate. A historical error-message string is version-sensitive and must not become a blanket catch-to-cancel rule. Test supported formats, invalid/truncated payloads, cancellation and cleanup failure without masking the original save outcome.

## Common mistakes

Do not copy permission lists from another platform or infer a grant from a manifest declaration. A save call returning successfully does not prove a gallery asset exists.

## Check it works

Check denied/empty clipboard reads, unsupported clipboard types, gallery visibility, saved bytes, cancellation and partial batch failure on the intended devices.

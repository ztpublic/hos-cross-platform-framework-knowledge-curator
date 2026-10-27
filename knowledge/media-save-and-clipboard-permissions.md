---
{
  "schema_version": 1,
  "id": "media-save-and-clipboard-permissions",
  "topic_key": "permissions.media-save-and-clipboard-permissions",
  "scope": "common",
  "frameworks": [],
  "title": "Adapt gallery saving and clipboard access to OH",
  "area": "permissions",
  "summary": "Choose the OH media/clipboard operation first, then request only the permissions and user interaction that operation requires.",
  "when_to_use": "An Android-oriented photo-save or clipboard path fails on OH, requests irrelevant permissions, or silently supports fewer data types.",
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
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": [
    "gallery-image-format-preservation",
    "qt-ohos-clipboard-gui-thread"
  ]
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
3. Declare permissions with appropriate user-facing reasons and request runtime grants only when needed. For a backend that checks `READ_PASTEBOARD` but never requests it, use the owning UIAbility context with `abilityAccessCtrl.createAtManager().requestPermissionsFromUser(...)`, await the result, and handle denial. A manifest declaration alone leaves a user-grant permission ungranted. Keep Android-only permission branches on Android.
4. Handle denied, empty and cancelled results distinctly. For batches, preserve per-item outcomes rather than reporting the entire batch as successful.
5. Close native descriptors and release temporary resources on partial failure.

## Conditions and alternatives

Permission requirements differ by operation, SDK and device. A clipboard text fallback can be valid if the UI clearly limits the feature to text.

See [Preserve encoded image format across a file-based gallery API](gallery-image-format-preservation.md) for this independent contract.

## Common mistakes

Do not copy permission lists from another platform or infer a grant from a manifest declaration. A save call returning successfully does not prove a gallery asset exists.

## Check it works

Check denied/empty clipboard reads, unsupported clipboard types, gallery visibility, saved bytes, cancellation and partial batch failure on the intended devices.

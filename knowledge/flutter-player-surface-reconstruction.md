---
{
  "schema_version": 1,
  "id": "flutter-player-surface-reconstruction",
  "title": "Serialize surface reconstruction with source loading",
  "topic_key": "media.flutter-player-surface-reconstruction",
  "area": "media",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "summary": "Serialize source and output changes, retain retry state and rebind all controller-owned subscriptions.",
  "when_to_use": "Changing renderer options races loading or leaves a new controller bound to old views.",
  "keywords": [
    "Texture",
    "video_player",
    "media_kit",
    "FVP",
    "MDK",
    "Erika",
    "OHNativeWindow",
    "黑屏",
    "video black screen",
    "audio without video",
    "Texture issue",
    "有声音没画面"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Different media backends have different OH support and integration requirements. A backend excluded by one dependency graph may be viable through another supported implementation.",
  "related": [
    "hdr-output-surface-and-capability",
    "video-surface-backend-contract"
  ]
}
---

# Serialize surface reconstruction with source loading

## When to use

Changing renderer options races loading or leaves a new controller bound to old views.

## Recommended approach

Serialize source and output changes, retain retry state and rebind all controller-owned subscriptions.

## Implementation steps

If a renderer or HDR option is fixed at player creation, put source loading and surface reconstruction on the same queue. Recheck the desired versus applied configuration at execution time so rapid PiP or source changes do not act on stale requests. After a failed rebuild, keep an explicit retry state: the old player may already be gone, so a later request must be able to recreate it even when no current player exists.

Publish controller replacement to the UI. If a video or subtitle widget subscribes only in `initState`, changing its controller property is insufficient; use a generation key or a supported rebind path to replace those subscriptions. Announce controller allocation separately from completion of source opening. Restore position, pause state and speed without invoking an unrelated autoplay policy; reattach tracks and other player-bound work only after the new source is ready.

For dynamic-range output, also preserve the [HDR surface, signaling and display contract](hdr-output-surface-and-capability.md).

## Conditions and alternatives

Different media backends have different OH support and integration requirements. A backend excluded by one dependency graph may be viable through another supported implementation.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Change source/output options rapidly, fail a rebuild, retry with no player and replace the controller after widget mount. Verify pause, position, speed and track rebinding.

---
{
  "schema_version": 1,
  "id": "qt6-ohos-subtitle-overlay",
  "title": "Composite subtitles over a Qt video texture on OH",
  "topic_key": "media.qt6-ohos-subtitle-overlay",
  "area": "media",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Draw text subtitles into a second RGBA texture and publish libass RGBA regions through a versioned C ABI bridge registered before video output starts.",
  "when_to_use": "A subtitle track is selected on OH but no text appears, text subtitles work while ASS fails, or embedded ASS fails although an external ASS file works.",
  "keywords": [
    "subtitle",
    "SRT",
    "ASS",
    "libass",
    "QImage",
    "RGBA",
    "external texture",
    "ABI bridge",
    "letterbox",
    "Qt 6"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Qt 6 OH PC media players using an external-texture video overlay. Text subtitles and full ASS typesetting use different producer paths.",
  "related": [
    "qt6-ohos-native-video-texture",
    "qt6-ohos-avsession-bridge",
    "native-surface-owner"
  ]
}
---

# Composite subtitles over a Qt video texture on OH

## When to use

A subtitle track is selected on OH but no text appears, text subtitles work while ASS fails, or embedded ASS fails although an external ASS file works.

## Recommended approach

Draw text subtitles as a transparent image into a second RGBA texture above the video texture. For libass-produced RGBA regions, publish them through a small C ABI bridge registered before the video output is created. Keep text and bitmap subtitle ownership separate.

## Implementation steps

1. Add a second RGBA texture and an alpha-blending shader; draw the external video texture first, then the subtitle texture.
2. Text path: convert decoded text regions to a transparent `QImage`, position them against the letterboxed video rectangle (not the whole window), upload, and expire the overlay when heartbeats stop.
3. Bitmap path: define C ABI structs for regions, a frame and a callback table. Validate struct sizes, RGBA chroma, dimensions and pitch; composite valid regions into one transparent image.
4. Register the bridge address on both the library root object and the playlist before creating the video surface/output. Make the output retry lookup if it starts before registration.
5. Keep a text owner and a bitmap owner. A bitmap clear must not erase a text overlay, and vice versa.
6. For ASS, verify the libass plugin and its shared-library dependencies load, provide a font fallback path when fontconfig is absent, and map the container's ASS codec ID correctly. Embedded ASS packets may already be libass chunks, not `Dialogue:` text.
7. Verify each stage separately: track demuxed, decoder produced regions, bridge published, overlay uploaded.

## Conditions and alternatives

The text renderer covers plain text subtitles decoded by the engine; full ASS typesetting and bitmap subtitles need the bitmap path. The bridge is an ABI contract and must be versioned if the structs change. A video backend without an external texture may not need this overlay.

## Common mistakes

- Treating "track selected" as proof that subtitles were decoded.
- Positioning subtitles against the window instead of the video letterbox.
- Clearing a text overlay when bitmap subtitles clear.
- Registering the bridge after the output has already started.
- Assuming external ASS support proves embedded ASS support.

## Check it works

Play a file with SRT and confirm text appears at the right position and disappears on cue. Repeat with external ASS and an embedded ASS track. Check that no stale subtitle remains after seeking or stop, and that subtitle delay changes take effect.

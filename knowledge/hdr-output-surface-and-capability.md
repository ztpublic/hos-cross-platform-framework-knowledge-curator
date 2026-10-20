---
{
  "schema_version": 1,
  "id": "hdr-output-surface-and-capability",
  "topic_key": "media.hdr-output-surface-and-capability",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "title": "Select HDR output for the active video surface and display",
  "area": "media",
  "summary": "Choose the output surface, HDR signaling and tone-mapping target together; use an explicit SDR fallback when the selected path cannot carry HDR.",
  "when_to_use": "An HDR source looks washed out or dark on OH, or loses its picture when switching between embedded playback, fullscreen and picture-in-picture.",
  "keywords": [
    "HDR",
    "HDR10",
    "HDR Vivid",
    "PQ",
    "HLG",
    "media_kit",
    "XComponent",
    "platform view",
    "hdrFormats",
    "ohosHdrMode",
    "targetPeak",
    "washed out video",
    "PiP HDR",
    "HDR泛白",
    "高光发暗",
    "色调映射"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Flutter OH with the selected embedding/plugin versions; PC behavior requires the stated acceptance checks.",
  "related": [
    "video-surface-backend-contract"
  ]
}
---

# Select HDR output for the active video surface and display

## When to use

An HDR source looks washed out or dark on OH, or loses its picture when switching between embedded playback, fullscreen and picture-in-picture.

## Recommended approach

Choose the output surface, HDR signaling and tone-mapping target together. With an OH media backend whose texture path is composed as SDR, use its supported native platform-view path for HDR and explicitly tone-map to SDR when that path is unavailable.

## Implementation steps

1. Resolve a compatible player, video-widget and native-library graph. Confirm that the exact OH backend implements the required surface and HDR controls; `usePlatformView`, `ohosHdrMode` and `ohosHdrTargetPeak` are fork-specific configuration examples, not universal `media_kit` APIs.
2. Classify the source's dynamic range and query the active display capability where supported, for example through `display.getDefaultDisplaySync().hdrFormats`. Keep successful unsupported results separate from a failed or unavailable query. Define an explicit policy for unknown capability; do not cache a transient exception as permanent unsupported status. Validate the returned format values, including any no-HDR sentinel, against the selected SDK.
3. Select a surface that preserves the backend's intended HDR metadata and color path. Check the actual composition mode: an underlay may need transparent Flutter content over the video rectangle, while a hybrid platform-view/overlay path can have different ordering. Avoid making the entire application transparent based only on an assumed layer order. Paint regions outside the video deliberately. Fullscreen is a layout state, not proof that the surface supports HDR.
4. Treat system PiP and an in-app mini-player as separate surface/lifecycle decisions. An overlay inside the same Flutter view may preserve native HDR composition; do not downgrade it merely because a generic PiP flag is true. If the selected native view cannot move to a different host, switch to a supported texture path with SDR tone mapping, then restore the appropriate path on return. Derive reconstruction from actual host/output changes. Do not assume every OH embedding has the same restriction.
5. Configure signaling and tone mapping before creating the player when those properties are creation-time settings. In a backend where omission means automatic HDR, use its explicit disabled value for an HDR source sent to an SDR path. A null option can preserve unwanted automatic signaling.
6. Match format signaling to the source and the native backend's metadata contract. HDR10, HDR Vivid and Dolby Vision are not interchangeable labels. A backend-specific conversion needs verified support; otherwise select its documented conversion or SDR fallback.
7. If the tone mapper needs target peak luminance, use validated information for the intended display. Apply that setting only on the HDR output path that expects it; an HDR peak value applied to an SDR target can produce incorrect brightness. Keep user calibration distinct from measured display capability.
8. Serialize source changes and surface reconstruction, comparing desired versus applied surface and signaling when the operation reaches the queue head. Preserve position, paused/playing state and speed, and rebind controller-dependent views and subscriptions. See [video-surface replacement](video-surface-backend-contract.md).

## Conditions and alternatives

This applies to a native OH player integration with explicit HDR/surface control. It does not establish that all Flutter textures are SDR or that every device, codec, embedding and native library supports HDR. An SDR tone-mapped path remains a valid fallback when native HDR is unavailable or unverified.

Do not infer actual HDR output from a source-quality badge, a hardware-decoder setting or a successful controller constructor. Verify the decoder, output signaling, compositor and display together.

Keep source peak estimation, display target peak and a source-preserving output mode distinct. A backend may use an omitted target to preserve source luminance, and an explicit target for compression; omission can have different semantics on the SDR fallback. Apply peak measurement only to the path that consumes it, keep settings in the desired/applied output comparison, and validate both bright and dim sources rather than treating a default nit value as a measured display property. Theme/view recreation must also reattach the active native surface; changing a package revision is not evidence that this lifecycle defect is fixed on a device.

## Common mistakes

Changing HDR content without updating creation-time signaling can leave the next source tagged with the previous format. Disposing the player without updating the widget's controller can leave a black view. Recreating a paused player through an autoplay initializer can unexpectedly resume it. An optimistic capability fallback must not be presented as confirmed device support.

## Check it works

Test SDR and each supported HDR format, supported/unsupported/unknown display results, embedded and fullscreen layouts, system PiP and in-app mini-player round trips, rapid source changes, pause preservation and rebuild failure recovery. Check actual native output metadata and visible results on the target display; verify that fallback output is tone-mapped rather than merely relabeled.

---
{
  "schema_version": 1,
  "id": "electron-ohos-harmonyshare-image-receive",
  "title": "Route received OH images to the owning Electron window",
  "topic_key": "integration.electron-ohos-harmonyshare-image-receive",
  "area": "integration",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Map the Electron/internal window to its system window ID, register the product-specific image receive capability there, stage the received data in the sandbox and deliver a renderer-compatible file.",
  "when_to_use": "A PC editor cannot receive a nearby shared image, sends it to the wrong window, or passes an authority file URI or HEIC image that the renderer cannot consume.",
  "applicability": "Ported Electron hosts with a native HarmonyShare adapter on Huawei products exposing Collaboration.HarmonyShare. This is not generic OpenHarmony support and may require product/service/API authorization.",
  "keywords": [
    "HarmonyShare",
    "dataReceive",
    "general.image",
    "windowId",
    "HEIC",
    "ImagePacker",
    "file URI",
    "Electron window"
  ],
  "related": [
    "document-picker-uri-access",
    "gallery-image-format-preservation",
    "electron-ohos-platform-capability-gates"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Route received OH images to the owning Electron window

## When to use

A PC editor cannot receive a nearby shared image, sends it to the wrong window, or passes an authority file URI or HEIC image that the renderer cannot consume.

## Recommended approach

Map the Electron/internal window to its system window ID, register the product-specific image receive capability there, stage the received data in the sandbox and deliver a renderer-compatible file.

## Implementation steps

1. Check `SystemCapability.Collaboration.HarmonyShare`, obtain the intended UIAbility context and map the live internal window to its system window ID.
2. Register dataReceive for that system window and the supported UTD/count. Receive into an app-owned directory and wait for the actual received record.
3. Open the URI using the file API that understands its authority. If the renderer lacks the format, decode and create a bounded, aspect-preserving compatible image without mislabelling its extension.
4. Route pending/success/failure to the originating live renderer window with request identity. Unregister before losing the native window and release file/image/packer resources on every path.

## Conditions and alternatives

The source produces JPEG despite an older comment saying PNG. Its fixed 1280 maximum edge is an application choice, not a platform limit. Consumer guidance adds bounded reads, cleanup and request/lifecycle guards that are not all implemented in the source.

## Common mistakes

Stripping an authority URI into an arbitrary path bypasses its access contract. A pending placeholder needs a terminal failure outcome as well as success.

## Check it works

Test two editor windows, unsupported service, HEIC and transparent input, oversized/corrupt data, failure during conversion and closing the target window while receiving.

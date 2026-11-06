---
{
  "schema_version": 1,
  "id": "pasteboard-image-pixelmap-plus-encoded-record",
  "title": "Publish a pasteboard image as both PixelMap and encoded bytes",
  "topic_key": "clipboard.pasteboard-image-pixelmap-plus-encoded-record",
  "area": "clipboard",
  "scope": "common",
  "frameworks": [],
  "summary": "Create the PasteData from the encoded image, add a PixelMap record so it becomes primary, and keep the encoded bytes under the MIME key so readers of either form succeed.",
  "when_to_use": "A copied image loses its PNG or JPEG format when read back, or readers using hasImage or getPrimaryPixelMap do not see it.",
  "applicability": "OH pasteboard image copy with @kit.BasicServicesKit. Reading requires ohos.permission.READ_PASTEBOARD, or the product's secure paste path; how other apps use these records was not tested.",
  "keywords": [
    "pasteboard",
    "createData",
    "addRecord",
    "MIMETYPE_PIXELMAP",
    "getPrimaryPixelMap",
    "image copy",
    "READ_PASTEBOARD",
    "image/png"
  ],
  "related": [
    "pasteboard-serialize-setdata",
    "media-save-and-clipboard-permissions",
    "gallery-image-format-preservation"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Publish a pasteboard image as both PixelMap and encoded bytes

## When to use

A copied image loses its PNG or JPEG format when read back, or readers using hasImage or getPrimaryPixelMap do not see it.

## Recommended approach

Create the PasteData from the encoded image, add a PixelMap record so it becomes primary, and keep the encoded bytes under the MIME key so readers of either form succeed.

## Implementation steps

1. Decode the input once to a PixelMap and keep the original ArrayBuffer and MIME type.
2. Call pasteboard.createData(mime, arrayBuffer), then pasteData.addRecord(pasteboard.MIMETYPE_PIXELMAP, pixMap). In the reviewed code addRecord inserts at the head, so the PixelMap is primary.
3. On the record from getRecord(0), set record.data[mime] = arrayBuffer so the encoded form can be found.
4. Write through the serialized queue and release the PixelMap on success, failure and skipped paths.

## Conditions and alternatives

Record order behavior is from source comments for the reviewed API. A text-only or PixelMap-only flow is simpler if format fidelity does not matter. Large images duplicate memory while on the pasteboard.

## Common mistakes

Releasing the PixelMap before setData finishes. Leaking it on error. Asking for the read permission for the write path, which does not need it.

## Check it works

Copy PNG with transparency and JPEG, read with both APIs in this app, then paste into a system or other app. Check the format and alpha, and watch memory across repeated copies.

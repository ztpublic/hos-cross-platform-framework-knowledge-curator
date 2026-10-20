---
{
  "id": "file-export-contract",
  "title": "Save a file where the user can actually access it",
  "topic_key": "storage.file-export-contract",
  "area": "storage",
  "keywords": [
    "saveFile",
    "export",
    "DocumentViewPicker",
    "导出",
    "文件选择",
    "sandbox",
    "SAF",
    "export missing file",
    "save dialog",
    "save bytes",
    "文件保存",
    "导出成功找不到"
  ],
  "summary": "Separate private persistence, destination selection and export completion. Report export success only after the selected destination contains the intended bytes.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "An export reports success but the user cannot find the file, or a save dialog returns a destination without writing the content."
}
---

# Save a file where the user can actually access it

## When to use

An export reports success but the user cannot find the file, or a save dialog returns a destination without writing the content.

## Recommended approach

Separate private persistence, destination selection and export completion. Report export success only after the selected destination contains the intended bytes.

## Implementation steps

1. Decide whether the feature saves app-private data, exports to a user-selected destination, or shares with another app. Give these operations distinct service methods.
2. For export, obtain a destination using an OH-capable save API. Determine whether that API writes supplied bytes or merely returns a URI/handle.
3. If writing is the caller's responsibility, open the destination through the appropriate native file API, stream the bytes, await completion, and close the handle in `finally`.
4. Keep any staging copy until the final write succeeds. Treat cancellation separately and define whether a cancelled export keeps or deletes that copy.
5. Return a result describing the completed operation; a private path displayed in a message is not a substitute for user-visible export.

## Conditions and alternatives

Use a system share sheet when the user wants another app to consume a file. For large files, prefer native/streaming transfer over collecting the entire file in Dart memory.

### Treat the selected URI as a destination, then complete the write

A native `DocumentViewPicker.save` selection is only destination acquisition. Acquire a presentation guard before opening it, return the public cancellation value for no selection, open the returned URI with the appropriate mode and finish writing/flushing/closing before reporting saved. Validate the typed binary argument, check actual write counts and handle partial writes; do not acknowledge requested bytes that were not written. Release the guard in finally and keep native write errors distinct from cancellation.

If an adapter wraps a native result into a shared URI/result type, preserve access identity and the selected backend's error categories. Test repeated taps, cancel-then-retry, zero bytes, short writes and destination failure.

## Common mistakes

Do not assume a URI is a usable `File` path. Do not reject a valid zero-byte file. Filename sanitization should preserve the extension needed by the receiving app; do not infer OS-wide restrictions from one backend failure.

## Check it works

Export empty, small and large files; cancel and deny access; simulate a full destination. Open the successful result from another app and compare its contents.

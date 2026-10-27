---
{
  "schema_version": 1,
  "id": "picker-cancellation-concurrency",
  "topic_key": "channels.picker-cancellation-concurrency",
  "scope": "common",
  "frameworks": [],
  "title": "File picker hangs after cancellation or repeated taps",
  "area": "channels",
  "summary": "Model cancellation as a normal result, settle each channel request once, and serialize access to the native picker.",
  "when_to_use": "A file-selection future never completes after the user closes the native dialog, or repeated taps open overlapping pickers.",
  "keywords": [
    "picker",
    "cancel",
    "busy",
    "exactly once",
    "取消",
    "concurrency",
    "MethodResult",
    "file picker",
    "hang",
    "Future never completes",
    "cancelled",
    "文件选择取消",
    "取消后卡住"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": [
    "kmp-filekit-picker-descriptor-ownership"
  ]
}
---

# File picker hangs after cancellation or repeated taps

## When to use

A file-selection future never completes after the user closes the native dialog, or repeated taps open overlapping pickers.

## Recommended approach

Model cancellation as a normal result, settle each channel request once, and serialize access to the native picker.

## Implementation steps

1. Define four outcomes at the bridge boundary: selected value, cancelled, busy, and error. Match the public adapter API: cancellation might be `null` for one file or an empty list for multiple files.
2. Acquire a picker-in-progress guard before presenting the dialog. Reject or queue another request explicitly.
3. Handle empty selections and the API's documented cancellation result before resolving file metadata. Return immediately after completing the request.
4. Translate permission and I/O failures into errors, not cancellation. Catch failures inside asynchronous native work.
5. Release the presentation guard when the native dialog actually settles, and clean temporary files and presentation changes on the matching outcome. Cancelling a coroutine or timing out the caller may not dismiss system UI: keep the guard until that still-open dialog settles, discard its late selection, and close any abandoned descriptors. On owner teardown, cancel or invalidate the pending continuation so a late callback cannot reply twice.

Protocol sketch:

```text
idle → picking → selected | cancelled | error → idle
       another request → busy
```


## Conditions and alternatives

Use the actual cancellation semantics of the selected picker API. An application timeout can protect the UI, but it does not stop the native dialog or release its resources.

## Common mistakes

Do not catch every exception and return an empty result. Do not keep one untracked result callback that a second request can overwrite.

## Check it works

Cancel, deny permission, double-tap, navigate away and trigger an I/O error. Every Future must settle once; a subsequent picker request must succeed.

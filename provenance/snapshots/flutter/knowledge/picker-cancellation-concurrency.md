---
{
  "id": "picker-cancellation-concurrency",
  "title": "File picker hangs after cancellation or repeated taps",
  "topic_key": "channels.picker-cancellation-concurrency",
  "area": "channels",
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
  "summary": "Model cancellation as a normal result, settle each channel request once, and serialize access to the native picker.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "A Flutter file-selection Future never completes after the user closes the native dialog, or repeated taps open overlapping pickers."
}
---

# File picker hangs after cancellation or repeated taps

## When to use

A Flutter file-selection Future never completes after the user closes the native dialog, or repeated taps open overlapping pickers.

## Recommended approach

Model cancellation as a normal result, settle each channel request once, and serialize access to the native picker.

## Implementation steps

1. Define four outcomes at the bridge boundary: selected value, cancelled, busy, and error. Match the public Dart API: cancellation might be `null` for one file or an empty list for multiple files.
2. Acquire a picker-in-progress guard before presenting the dialog. Reject or queue another request explicitly.
3. Handle empty selections and the API's documented cancellation result before resolving file metadata. Return immediately after completing the request.
4. Translate permission and I/O failures into errors, not cancellation. Catch failures inside asynchronous native work.
5. Release the guard, temporary files and any temporarily changed orientation in `finally`. On owner teardown, settle or invalidate its pending request so a late callback cannot reply twice.

Protocol sketch:

```text
idle → picking → selected | cancelled | error → idle
       another request → busy
```


## Conditions and alternatives

Use the actual cancellation semantics of the selected picker API. A Dart timeout can protect the UI, but it does not stop the native dialog or release its resources.

## Common mistakes

Do not catch every exception and return an empty result. Do not keep one untracked result callback that a second request can overwrite.

## Check it works

Cancel, deny permission, double-tap, navigate away and trigger an I/O error. Every Future must settle once; a subsequent picker request must succeed.

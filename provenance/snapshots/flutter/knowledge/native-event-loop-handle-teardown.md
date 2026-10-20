---
{
  "schema_version": 2,
  "id": "native-event-loop-handle-teardown",
  "title": "Release native handles after event-loop use has stopped",
  "topic_key": "ffi.native-event-loop-handle-teardown",
  "area": "ffi",
  "keywords": ["mpv_handle", "mpv_wait_event", "mpv_terminate_destroy", "NativeCallable", "isolate", "shutdown callback", "use after free", "double free", "native crash", "动态截图闪退", "句柄销毁", "事件循环释放"],
  "summary": "Separate callback detachment, worker shutdown and native destruction; release each handle once after every event-loop user has finished.",
  "when_to_use": "A native conversion or playback task crashes on cancellation or shutdown, particularly after changing a native library or event-loop implementation.",
  "status": "scoped"
}
---

# Release native handles after event-loop use has stopped

## When to use

A native conversion or playback task crashes on cancellation or shutdown, particularly after changing a native library or event-loop implementation.

## Recommended approach

Give the native handle one release owner. Detach application callbacks, request backend shutdown if needed, wait for all event-loop use to finish, and then destroy it exactly once. Use the selected backend's supported initializer/disposal pair.

## Implementation steps

1. Trace all handle users: the Dart caller, native callbacks, a waiting isolate/thread and any loop that continues after a callback returns. A callback subscription being removed does not prove those users have stopped.
2. Resolve the native library, generated bindings and event-loop helper as a compatible set. Avoid copying an old private initializer into a newer backend without reconciling shutdown and handle-registration behavior. A dependency version or branch label alone does not establish compatibility.
3. Mark the operation releasing before detaching events. Reject new work and late result publication. Unify user cancellation, completion, shutdown events and repeated dispose into one idempotent release operation.
4. If cancellation must stop active work, request quit through the backend's supported operation while the handle is valid. When handling an already-received shutdown event, avoid sending a second quit blindly. Settle the public completion once, separately from native cleanup completion.
5. Await event-loop exit or a backend acknowledgement before destruction. Do not synchronously destroy the handle inside a callback whose dispatcher will use it again on return. If the exact backend supplies only a deferred-destruction workaround, keep it explicitly scoped: a fixed timer is a grace period, not proof that every in-flight user finished.
6. Handle cancellation during asynchronous creation. If no handle existed at cancellation time, the creation continuation must observe the cancelled generation and release the newly returned handle. Keep cleanup ownership until it finishes, even after the UI disappears.

## Conditions and alternatives

Prefer the backend's public lifetime API. If private FFI integration is necessary, document its ownership preconditions and validate them against the resolved implementation. Keep handle teardown distinct from [shared-buffer retain/release](native-ffi-shared-buffer-ownership.md) and from [video-widget/controller replacement](video-surface-backend-contract.md).

## Common mistakes

Closing a Dart stream is not native event-loop termination. Completing a Future is not resource release. Delaying destruction can hide a race without eliminating it; duplicate release requests must not schedule duplicate destruction.

## Check it works

Exercise normal completion, cancellation before creation returns, cancellation during an event callback, shutdown racing cancellation, repeated dispose, and slow/stalled event-loop exit. Observe handle lifetime and callback activity with native diagnostics; a test that only checks timer order does not establish freedom from use-after-free.

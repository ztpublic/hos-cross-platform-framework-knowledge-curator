---
{
  "schema_version": 1,
  "id": "napi-handle-scope-without-js-frame",
  "title": "Open a handle scope around Node-API work outside a JS call frame",
  "topic_key": "native-bridge.napi-handle-scope-without-js-frame",
  "area": "native-bridge",
  "scope": "common",
  "frameworks": [],
  "summary": "Native code that runs from a uv_async or other event-loop callback has no default handle scope, so wrap each call that creates napi_values in an RAII handle scope.",
  "when_to_use": "ArkTS strings and closures grow steadily across page entries and GC never reclaims them; heap snapshots show objects retained through a local handle root, and views passed to native code leak.",
  "applicability": "Node-API on the ArkTS runtime, called from native tasks posted to the main loop rather than from a JS-invoked native method. Applies to any framework bridge using this path.",
  "keywords": [
    "napi_open_handle_scope",
    "napi_close_handle_scope",
    "uv_async",
    "handle leak",
    "LocalHandleRoot",
    "ArkTS memory leak",
    "heap snapshot",
    "RAII"
  ],
  "related": [
    "napi-thread-and-handle-lifetime",
    "native-event-loop-handle-teardown",
    "kmp-napi-string-allocator-ownership"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Open a handle scope around Node-API work outside a JS call frame

## When to use

ArkTS strings and closures grow steadily across page entries and GC never reclaims them; heap snapshots show objects retained through a local handle root, and views passed to native code leak.

## Recommended approach

Native code that runs from a uv_async or other event-loop callback has no default handle scope, so wrap each call that creates napi_values in an RAII handle scope.

## Implementation steps

1. List native entry points by how they are entered: a native method called from JS already has a scope, but a uv_async callback, timer or posted main-thread task does not.
2. Create a small RAII type that opens a scope in its constructor, records whether that succeeded, and closes it in its destructor. Delete copy operations.
3. Declare it at the top of each scope-less function, before the first napi_value is created, so every early return closes it.
4. Search for unpaired napi_open_handle_scope calls and convert them. Any value needed after the scope ends must become a napi_ref.
5. For long loops creating many values in one call, add a nested scope per iteration.

## Conditions and alternatives

This complements the thread and lifetime rule: that entry covers which thread and reference to use, and this one covers when local handles get released. Do not open a scope on a thread that does not own the env.

## Common mistakes

Returning a napi_value created inside a scope you close first. Opening a scope manually and missing an early-return path. Assuming GC can collect objects still held by handles from an unclosed scope.

## Check it works

Repeat the page or feature entry and exit many times, let the app idle, and compare object counts in heap snapshots. String and closure counts should stop growing with the round count, and callbacks should still receive valid values.

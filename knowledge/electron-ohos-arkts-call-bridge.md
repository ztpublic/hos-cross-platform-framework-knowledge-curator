---
{
  "schema_version": 1,
  "id": "electron-ohos-arkts-call-bridge",
  "title": "Call ArkTS from an Electron addon through a bound adapter",
  "topic_key": "native-addon.electron-ohos-arkts-call-bridge",
  "area": "native-addon",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Reach OS capabilities that Electron does not expose through an adapter library linked to the runtime and an ArkTS function binding, with a bounded wait on asynchronous replies.",
  "when_to_use": "Electron JavaScript needs a capability such as a device identifier or screen capture, which only ArkTS or a system kit provides, and an ordinary Node addon cannot reach the ArkTS VM.",
  "applicability": "Electron OH hosts where you control both the ArkTS host and a native addon. Reviewed sources build against a runtime tree and link libelectron.so and an AKI binding library. The ArkTS registration side was not in the reviewed repository, and no runtime version compatibility was established. Some runtimes are documented by the source as offering systemPreferences.callArkTSFunction, which should be checked first.",
  "keywords": [
    "ArkTS bridge",
    "AKI",
    "JSBind",
    "GetJSFunction",
    "node-addon-api",
    "AsyncWorker",
    "callArkTSFunction",
    "libaki_jsbind",
    "Electron addon"
  ],
  "related": [
    "electron-ohos-napi-symbol-owner",
    "electron-ohos-native-addon-abi",
    "napi-thread-and-handle-lifetime"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Call ArkTS from an Electron addon through a bound adapter

## When to use

Electron JavaScript needs a capability such as a device identifier or screen capture, which only ArkTS or a system kit provides, and an ordinary Node addon cannot reach the ArkTS VM.

## Recommended approach

Reach OS capabilities that Electron does not expose through an adapter library linked to the runtime and an ArkTS function binding, with a bounded wait on asynchronous replies.

## Implementation steps

1. Check whether your runtime provides a direct API, and use that if it does, as a custom binding has more to maintain.
2. On the ArkTS host, register named functions with the binding library before the Electron main script needs them.
3. Build an adapter shared library that links the binding library and libelectron.so, and look functions up with aki::JSBind::GetJSFunction, checking for null.
4. Wrap the adapter with a node-addon-api .node. Run asynchronous calls in a Napi::AsyncWorker that waits on a condition variable with a timeout, so the Electron main thread is never blocked by a missing reply.
5. Return a structured reply such as JSON {code, data, msg}, and map timeouts and missing functions to explicit errors.

## Conditions and alternatives

The reviewed waits are 15 s, with 10 s for capture. Avoid single global promise slots like the reviewed authorization path, which are unsafe for concurrent calls. Rebuild the adapter whenever the runtime or its ABI changes.

## Common mistakes

Calling a VM value from the wrong environment. Waiting with no timeout. Assuming the ArkTS function exists at startup. Linking the addon against a different runtime build than the one that ships.

## Check it works

Invoke each function synchronously and asynchronously, including with a missing registration, a thrown ArkTS error and a slow handler. Run concurrent calls and repeat after a window reload.

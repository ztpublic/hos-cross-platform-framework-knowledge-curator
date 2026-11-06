---
{
  "schema_version": 1,
  "id": "arkweb-javascriptproxy-deferred-reply",
  "title": "Defer ArkWeb runJavaScript replies out of a javaScriptProxy callback",
  "topic_key": "webview.arkweb-javascriptproxy-deferred-reply",
  "area": "webview",
  "scope": "common",
  "frameworks": [],
  "summary": "From a javaScriptProxy method, return quickly and schedule native dispatch and the runJavaScript reply with setTimeout on the UI loop.",
  "when_to_use": "A page-to-native bridge fails or behaves erratically when its handler dispatches work and calls runJavaScript directly from inside the proxy callback.",
  "applicability": "ArkWeb hosts on OH with a request/reply bridge over javaScriptProxy and runJavaScript. The thread statement is a code comment from one example host, not a documented guarantee that was checked. Only the example host implements this in the reviewed repository.",
  "keywords": [
    "ArkWeb",
    "javaScriptProxy",
    "runJavaScript",
    "setTimeout",
    "bridge reply",
    "WebviewController",
    "onPageEnd",
    "UI thread"
  ],
  "related": [
    "arkweb-document-start-registration",
    "webview-controller-and-lifetime",
    "tauri-ohos-ability-white-screen"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Defer ArkWeb runJavaScript replies out of a javaScriptProxy callback

## When to use

A page-to-native bridge fails or behaves erratically when its handler dispatches work and calls runJavaScript directly from inside the proxy callback.

## Recommended approach

From a javaScriptProxy method, return quickly and schedule native dispatch and the runJavaScript reply with setTimeout on the UI loop.

## Implementation steps

1. Expose an object through javaScriptProxy({object, name, methodList, controller}) with one generic call method.
2. In that method, parse the input and queue the work with setTimeout(() => ..., 0) rather than running it inline.
3. In the deferred function, dispatch natively and call controller.runJavaScript with the reply script, catching and reporting failures.
4. Correlate replies with a request ID and ignore replies after the controller or page is gone.
5. Install protocol script before first use. Injecting in onPageEnd misses calls made by early page scripts; see the document-start entry.

## Conditions and alternatives

Prefer a message port for frequent or large traffic if it fits your API level. Serialize replies with a proper JSON encoder rather than string concatenation, since script injection is otherwise possible.

## Common mistakes

Touching UI or controller state from the proxy callback. Assuming onPageEnd registration covers page startup. Building reply scripts from untrusted strings.

## Check it works

Call the bridge during page load, in bursts and across reload and destruction. Confirm each promise resolves once and stale replies do not reach a new page.

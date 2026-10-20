---
{
  "schema_version": 2,
  "id": "arkweb-document-start-registration",
  "title": "Make a JavaScript bridge available before an ArkWeb page starts",
  "topic_key": "web.arkweb-document-start-registration",
  "area": "web",
  "keywords": ["ArkWeb", "document start", "runJavaScriptOnDocumentStart", "JavaScript bridge undefined", "FrameCallback", "postFrameCallback", "addUserScript", "首个脚本", "桥接未就绪", "页面启动竞态"],
  "summary": "Register the message receiver and native document-start scripts before navigation, and acknowledge registration only after the owning Web component applies the script list.",
  "when_to_use": "The first inline script cannot find a Flutter bridge even though the same bridge works after the page finishes loading.",
  "status": "scoped"
}
---

# Make a JavaScript bridge available before an ArkWeb page starts

## When to use

The first inline script cannot find a Flutter bridge even though the same bridge works after the page finishes loading.

## Recommended approach

Register the message receiver and native document-start scripts before navigation, and acknowledge registration only after the owning Web component applies the script list. Keep registration readiness separate from evidence that a particular document executed the script.

## Implementation steps

1. Attach the native view and await registration of the JavaScript message channel. Prepare the bridge's method descriptors before starting the document rather than discovering them asynchronously from its first script.
2. Use an OH backend that exposes native document-start injection. For an ArkUI Web host, maintain the scripts in component state and apply them with `runJavaScriptOnDocumentStart`; a call to `runJavaScript` after navigation is too late for the first inline script.
3. Correlate each channel request with its engine, view and script handle. If the component has not attached, wait for its script host; reject the request if that view is destroyed.
4. Acknowledge an updated script list after the component applies it. An ArkUI integration can use `UIContext.postFrameCallback` and a settling `FrameCallback` as its registration barrier. Validate the barrier on the selected SDK instead of assuming a Dart channel reply proves native injection order.
5. Serialize script replacement and removal, await successful registration, then navigate. Check the owner's lifetime after waits. Propagate failures, and recover the serialization queue so a rejected update does not silently prevent all subsequent attempts.
6. Make installation idempotent within a document. Keep unrelated persistent scripts, such as page-zoom policy, separate from application bridge handles. Removing bridge scripts affects future documents; remove or invalidate already-created objects explicitly if current-document revocation is required.
7. Reject pending registrations when the component disappears or the engine is detached. Remove owned script handles and native channel handlers during disposal.

Protocol sketch: `receiver ready → script host attached → script list applied → registration acknowledged → navigate → first document script uses bridge`.

## Conditions and alternatives

The backend and SDK must support the required injection timing, matching rules and frame scope. Restrict privileged bridges to the intended origins and frames; a wildcard registration is unsuitable for arbitrary remote content. Keep the corresponding native operation authorization in place.

If the app owns the complete HTML, placing the bootstrap before all page scripts is an alternative. It does not cover arbitrary URL navigation, frames or later documents automatically. If deterministic injection is unavailable, use an explicit page-ready handshake and postpone dependent page operations; report that capability honestly.

See [WebView controller ownership](webview-controller-and-lifetime.md) and [native local-content hosting](native-arkweb-game-host.md).

## Common mistakes

Registering at `onPageFinished` cannot repair a failed startup call. Updating ArkUI state and immediately replying can race the next navigation. Waiting indefinitely for a host that was destroyed leaves Dart stuck. Registration acknowledgement is not proof of execution or a successful business operation.

## Check it works

Use a document whose first inline script invokes the bridge. Test cold creation, rapid registration followed by navigation, reload, redirects, allowed and disallowed frames/origins, replacement, removal, and disposal while registration is pending. Record actual first-script execution on the target ArkWeb backend.

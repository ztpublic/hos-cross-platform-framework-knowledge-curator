---
{
  "schema_version": 1,
  "id": "webview-controller-and-lifetime",
  "topic_key": "web.webview-controller-and-lifetime",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "title": "Replace a WebView backend without losing application behavior",
  "area": "web",
  "summary": "Adapt the controller/event contract and view ownership explicitly; replacing the package source alone is insufficient.",
  "when_to_use": "An OH-capable WebView compiles but login, JavaScript callbacks, navigation or off-page content parsing stops working.",
  "keywords": [
    "WebViewController",
    "JavaScript",
    "OverlayEntry",
    "navigation",
    "网页",
    "controller",
    "WebView plugin replacement",
    "JS callback missing",
    "WebView生命周期"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Flutter OH with the selected embedding/plugin versions; PC behavior requires the stated acceptance checks.",
  "related": [
    "arkweb-document-start-registration"
  ]
}
---

# Replace a WebView backend without losing application behavior

## When to use

An OH-capable WebView compiles but login, JavaScript callbacks, navigation or off-page content parsing stops working.

## Recommended approach

Adapt the controller/event contract and view ownership explicitly; replacing the package source alone is insufficient.

## Implementation steps

1. List the WebView behaviors the app uses: navigation policy, JS calls/handlers, cookies, initialization, errors and source-resolution events.
2. Map each behavior to the selected backend's API and preserve the shared service interface where possible.
3. Wait for native/controller initialization before loading or injecting scripts.
4. Give long-lived parsing/login work an owner that survives as long as the operation needs; do not accidentally tie it to an unrelated visible page.
5. Route native results through the same shared completion/update path as other backends.
6. Cancel subscriptions, pending work and native views when their actual owner is disposed.
7. Bound retries and timeouts. Give each operation a generation identifier and settle it once; check that identifier and owner state after every asynchronous wait before publishing results. Cancel timers at completion and disposal.

## Conditions and alternatives

An overlay or dedicated native page may be appropriate when work must outlive a screen. That changes view ownership, not background-execution eligibility.

If the page needs a host bridge in its first inline script, use [native document-start registration](arkweb-document-start-registration.md) and await its completion before navigation. Controller construction and `onPageFinished` injection do not establish this ordering.

### Preserve optional hooks and JavaScript result semantics

Enable native request interception only when the application needs it and the selected backend implements the required requests. If enabling the hook breaks a page, compare the same operation with it disabled and isolate the backend/version interaction. Disabling an unused hook can be a scoped workaround; it must not silently remove an application authorization or resource-loading policy.

Agree on JavaScript evaluation results across the shared controller and every backend. A legitimate JSON `null` result should remain distinct from a script exception or transport failure when the public contract allows it. Propagate nullable signatures through callers instead of converting every null to a bridge error or double-encoding JSON text. Preserve that distinction in regression cases on each selected backend.

## Common mistakes

Do not assume JS channel APIs, cookie access or loading callbacks have the same names and timing across WebView packages. A page recreation can invalidate in-flight requests.

Cancelling a timer does not cancel an already running JavaScript call or delayed navigation callback. Late callbacks must not overwrite a newer operation or turn a timeout into success. Keep existing data when page readiness or parsing fails; an unrecognized page is not evidence of an empty result.

## Check it works

Test navigation, errors, cookies, JS round trips, delayed initialization, leaving the initiating screen and disposal while callbacks are pending.

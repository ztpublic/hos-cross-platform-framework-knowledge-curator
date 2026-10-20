---
{
  "schema_version": 1,
  "id": "oauth-browser-callback-correlation",
  "topic_key": "security.oauth-browser-callback-correlation",
  "scope": "common",
  "frameworks": [],
  "title": "Complete browser OAuth without losing or accepting the wrong callback",
  "area": "security",
  "summary": "Arm an attempt-scoped callback receiver before opening the external browser, then validate and settle the authorization flow once.",
  "when_to_use": "Browser authorization returns to the app but login times out, is cancelled too early, or consumes a stale deep link.",
  "keywords": [
    "OAuth",
    "PKCE",
    "state",
    "redirect_uri",
    "onNewWant",
    "browser login",
    "登录回跳",
    "登录取消",
    "冷启动回调"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": [
    "incoming-wants-before-runtime-ready"
  ]
}
---

# Complete browser OAuth without losing or accepting the wrong callback

## When to use

Browser authorization returns to the app but login times out, is cancelled too early, or consumes a stale deep link.

## Recommended approach

Use an external browser or a browser-controlled authentication session for native OAuth. Arm an attempt-scoped callback receiver before launch, preserve PKCE and state, and settle each attempt once after validating its redirect.

## Implementation steps

1. Register the supported redirect URI with the authorization server and the OH ability's URI routing configuration. Match its scheme, authority and path when handling the callback; an OS scheme match alone is insufficient.
2. Create a fresh attempt identifier, unpredictable state and PKCE verifier. Send the S256 challenge, state and exact redirect URI in the authorization request; retain the verifier for that attempt's token exchange.
3. Install the application receiver before invoking `launchUrl`. Normalize initial launch Wants and `onNewWant` into a native mailbox. Drain it after the application runtime announces readiness, then acknowledge accepted events. A broadcast stream does not retain events for future subscribers. See [incoming launch delivery](incoming-wants-before-runtime-ready.md).
4. Open the authorization endpoint using a verified external-browser backend. Treat launch acceptance as pending authorization. A plugin reporting that every launch mode is supported does not establish which native surface it opens.
5. Accept only the active attempt's redirect and state. Reject unrelated links, expired attempts and duplicate callbacks. Handle OAuth error responses separately; exchange a nonempty authorization code using that attempt's verifier and original redirect URI. Commit authenticated state only after the required exchange and account/storage operations succeed.
6. Define explicit cancellation and timeout outcomes. App resume alone does not prove the user cancelled: native delivery and application processing can finish later. Do not use a short fixed resume delay as the sole success/cancellation arbiter.
7. Release subscriptions and attempt resources on every terminal path. Invalidate delayed timers and callbacks using the attempt identifier. If process loss destroys the verifier/state, restart authorization rather than accepting a callback that cannot be correlated. Exclude authorization codes, full callback queries and tokens from logs.

## Conditions and alternatives

A public native client must not rely on an embedded client secret. Use a supported browser authentication API when it provides a separate security domain and reliable completion events. An app-controlled WebView does not become such a browser session merely by naming its launch mode `inAppBrowserView`.

Warm-start handling alone does not cover callbacks that start a new process. If recovery is required, specify protected storage and expiry for pending authorization state; otherwise clearly return to a fresh sign-in attempt.

## Common mistakes

Opening the browser before subscribing can lose a fast redirect. A successful `startAbility`, a resumed app or a duplicate URI suppression cache is not proof of successful authorization. A global cache can also suppress a legitimate later event when it lacks attempt lifetime.

## Check it works

Exercise a fast callback, delayed runtime readiness, cold launch, unrelated deep links, wrong state/path, duplicate callbacks, server denial, explicit cancel, timeout and a late callback after retry. Confirm one token exchange per accepted attempt and no authenticated state after failure.

---
{
  "schema_version": 1,
  "id": "native-web-login-cookie-completion",
  "topic_key": "web.native-web-login-cookie-completion",
  "scope": "common",
  "frameworks": [],
  "title": "Wait for authenticated state in a native Web login flow",
  "area": "web",
  "summary": "Define authentication completion by the required session state, and give success, cancellation and failure distinct one-time outcomes.",
  "when_to_use": "An OH-native login page closes before session cookies are written, returns incomplete credentials, or leaves a request waiting after the user goes back.",
  "keywords": [
    "cookies",
    "ArkTS Web",
    "login",
    "emitter",
    "polling",
    "登录",
    "session",
    "login cookie missing",
    "native login never completes",
    "登录Cookie",
    "登录返回空"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": []
}
---

# Wait for authenticated state in a native Web login flow

## When to use

An OH-native login page closes before session cookies are written, returns incomplete credentials, or leaves a request waiting after the user goes back.

## Recommended approach

Define authentication completion by the required session state, and give success, cancellation and failure distinct one-time outcomes.

## Implementation steps

1. Specify the trusted login origins, cookie URLs and exact keys or other signals required for successful login.
2. Open the native Web page with a request/session identifier. Do not treat first page load or any redirect as authentication success.
3. Observe cookie/state changes through supported callbacks or bounded polling when asynchronous writes require it.
4. Return only the intended allowlisted session values after the success condition is met.
5. On back, disappearance, timeout or native error, complete the request appropriately and stop timers/listeners.
6. Store credentials through the application's selected credential-storage policy, independently from the page transport.

## Conditions and alternatives

Use the actual authentication service contract; cookie presence alone may be insufficient to establish a valid session.

## Common mistakes

A cancelled page must not return stale cookies from a prior login. Do not expose unrelated cookies or describe ordinary native preferences as protected credential storage.

## Check it works

Test delayed cookies, redirects, cancel/back, repeated login, missing success keys, expiry and cleanup after the page has disappeared.

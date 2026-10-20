---
{
  "schema_version": 1,
  "id": "local-auth-policy-and-attempt-lifetime",
  "topic_key": "security.local-auth-policy-and-attempt-lifetime",
  "scope": "common",
  "frameworks": [],
  "title": "Adapt local authentication without bypassing the application lock",
  "area": "security",
  "summary": "Preserve the authentication policy across capability checks, native attempts and fallback; grant access only for a successful current attempt.",
  "when_to_use": "An OH application lock uses biometrics or device PIN, but unavailable hardware, cancellation or fallback can accidentally unlock the app or leave a pending request.",
  "keywords": [
    "userAuth",
    "UserAuthenticationKit",
    "getAvailableStatus",
    "getUserAuthInstance",
    "ACCESS_BIOMETRIC",
    "AuthTrustLevel",
    "fingerprint",
    "face",
    "PIN",
    "biometricOnly",
    "本地认证",
    "指纹",
    "人脸",
    "认证取消",
    "应用锁"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": [
    "secure-storage-backend-selection"
  ]
}
---

# Adapt local authentication without bypassing the application lock

## When to use

An OH application lock needs face, fingerprint or device PIN authentication through a plugin or app-owned bridge.

## Recommended approach

Keep the lock policy independent of capability discovery. Treat success, cancellation, unavailable authentication and operational failure as distinct results. Grant access only for the successful attempt that still owns the current lock screen.

## Implementation steps

1. Define allowed authentication types, required trust level and whether PIN fallback is permitted. A biometric-only policy must not silently acquire a PIN fallback.
2. Declare `ohos.permission.ACCESS_BIOMETRIC` and select APIs supported by the target SDK. With the API 9+ module-level `getAvailableStatus`, check the exact type/trust-level pair using exception handling; this differs from the deprecated numeric-return API. Distinguish missing enrollment, permission errors and unsupported types.
3. Use the same type/trust-level pair when constructing `AuthParam` for `getUserAuthInstance` (API 10+). Use an appropriate per-attempt challenge when validating a token; a fixed sample challenge is unsuitable for replay protection.
4. Serialize attempts and retain the active native instance plus an attempt ID. Subscribe to its result before `start()`. Only `SUCCESS` from the current attempt can satisfy the lock.
5. Offer only policy-approved fallback. `CANCELED_FROM_WIDGET` means the user selected an application-defined alternate flow; it is not authentication success. Do not automatically start another dialog after ordinary cancellation.
6. On cancellation or owner teardown, cancel the started native instance, unsubscribe its result listener and settle the application request once. Reject late results from replaced attempts. A `UserAuthInstance` starts only one authentication attempt.

## Conditions and alternatives

SDK support, credential enrollment and trust-level availability differ by device. Querying at one level and authenticating at another does not establish support. Keep the required level fixed by policy rather than lowering it until a call works.

If local authentication becomes unavailable after the lock was enabled, keep the protected content locked and provide an explicit approved recovery path. A capability failure must not call the success callback. An optional convenience prompt can have a different policy, but that decision belongs to the product contract.

## Common mistakes

A boolean wrapper can erase cancellation and permission failures. Authentication UI does not itself encrypt stored credentials; use an appropriate [credential-storage backend](secure-storage-backend-selection.md) for that guarantee.

## Check it works

Test absent enrollment, unsupported trust levels, permission errors, biometric-only policy, approved PIN fallback, cancellation, repeated taps, leaving the screen and stale callbacks. Verify that every non-success path leaves the lock intact and completes its request.

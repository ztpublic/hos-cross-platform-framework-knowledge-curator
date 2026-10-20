---
{
  "id": "continuation-cached-state-and-readiness",
  "title": "Transfer app state through OH continuation",
  "topic_key": "integration.continuation-cached-state-and-readiness",
  "area": "integration",
  "keywords": [
    "onContinue",
    "continuation",
    "流转",
    "跨设备",
    "reader state",
    "UIAbilityContext",
    "continuation restore race",
    "onContinue async",
    "跨设备恢复",
    "流转状态"
  ],
  "summary": "Maintain a small valid continuation snapshot ahead of time, and restore incoming state only after the destination services and navigation are ready.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "Continuation needs a synchronous native response, or an incoming reader/document state arrives before its Dart dependencies are initialized."
}
---

# Transfer app state through OH continuation

## When to use

Continuation needs a synchronous native response, or an incoming reader/document state arrives before its Dart dependencies are initialized.

## Recommended approach

Maintain a small valid continuation snapshot ahead of time, and restore incoming state only after the destination services and navigation are ready.

## Implementation steps

1. Define a serializable state contract containing the minimum identifiers and progress needed to resume the activity.
2. Update the cached native snapshot as the active activity changes; mark it inactive and clear stale data when the activity ends.
3. In the native continuation callback, validate and return the already prepared state according to that callback's synchronous/asynchronous contract.
4. Buffer incoming state until source providers, account context and routing are initialized. Then resolve identifiers and restore progress once.
5. Handle unavailable content, unsupported state versions and cancelled transfers with an explicit fallback screen/result.

## Conditions and alternatives

Continuation requires the native service and suitable device/product support. A cached snapshot should contain transferable state, not live controllers, native handles or credentials.

## Common mistakes

Do not start a late Dart round trip inside a callback that requires an immediate result. Do not restore an inactive or outdated page merely because a payload exists.

## Check it works

Test active/inactive activities, cold receiver startup, delayed dependencies, invalid versions, unavailable content and state changes during transfer.

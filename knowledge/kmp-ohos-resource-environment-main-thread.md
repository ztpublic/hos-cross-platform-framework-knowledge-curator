---
{
  "schema_version": 1,
  "id": "kmp-ohos-resource-environment-main-thread",
  "title": "Resolve Compose resource environments on the owning OH thread",
  "topic_key": "native.kmp-ohos-resource-environment-main-thread",
  "area": "native",
  "scope": "framework-specific",
  "frameworks": [
    "kotlin-multiplatform"
  ],
  "summary": "Dispatch resource lookup to the initialized OH main dispatcher when the selected Compose adapter queries ArkTS state through NAPI.",
  "when_to_use": "Calling getString(StringResource) from a worker coroutine terminates the OH process with ecma_vm cannot run in multi-thread!.",
  "applicability": "CPF Compose resource adapter that reads the resource environment over NAPI for each lookup. Source host is phone-oriented; PC locale/theme/DPI changes and dispatcher ownership need validation.",
  "keywords": [
    "ecma_vm cannot run in multi-thread!",
    "getString",
    "StringResource",
    "Dispatchers.Main",
    "withContext",
    "NAPI",
    "locale"
  ],
  "related": [
    "kmp-ohos-prepared-compose-resources",
    "kmp-ohos-compose-host"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Resolve Compose resource environments on the owning OH thread

## When to use

Calling getString(StringResource) from a worker coroutine terminates the OH process with ecma_vm cannot run in multi-thread!.

## Recommended approach

Dispatch resource lookup to the initialized OH main dispatcher when the selected Compose adapter queries ArkTS state through NAPI.

## Implementation steps

1. Confirm the failing lookup queries an ArkTS/NAPI environment; reading a native raw byte file does not by itself require the same UI-thread path.
2. Initialize the KMP main handler from the owning ArkTS thread before using Dispatchers.Main.
3. Use a suspend helper with withContext(Dispatchers.Main) { getString(resource, *formatArgs) } on OH. Resolve the string there and pass the immutable result back to background work.
4. Use the helper in non-Composable service/model code as well as UI callers. Avoid blocking waits from the UI thread back to a worker waiting for that thread.
5. Re-evaluate the workaround if the adapter changes to cache its platform environment. Any cache must still invalidate correctly for locale, theme and display changes.

## Conditions and alternatives

This applies to an adapter whose environment lookup crosses NAPI. It is not a rule that all Kotlin resource IO belongs on the UI thread. The source helper keeps other platforms on their original lookup path.

## Common mistakes

Do not move every image decode or file read onto the UI thread, use an uninitialized Main dispatcher, or hold a lock across a cross-thread lookup.

## Check it works

Resolve localized strings from UI and worker callers, then change locale/theme/display scale and retry. Confirm no VM-thread abort, correct values and responsive PC windows.

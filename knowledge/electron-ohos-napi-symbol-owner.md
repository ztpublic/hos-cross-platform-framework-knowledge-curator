---
{
  "schema_version": 1,
  "id": "electron-ohos-napi-symbol-owner",
  "title": "Resolve addon NAPI calls against the owning Electron VM",
  "topic_key": "native.electron-ohos-napi-symbol-owner",
  "area": "native",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "For the affected runtime, resolve the required NAPI entrypoints from the actual loaded Electron library and forward addon calls consistently to that owner.",
  "when_to_use": "An OH Electron native addon has the correct Node ABI but fails to resolve NAPI symbols or crashes after binding to the ArkTS VM instead of Electron's environment.",
  "applicability": "Electron OH fork loading libelectron.so with local symbol visibility and exposing multiple NAPI providers. PC behavior depends on the exact runtime, SDK, addon ABI and authorized loader policy.",
  "keywords": [
    "RTLD_LOCAL",
    "RTLD_NOLOAD",
    "dlsym",
    "libelectron.so",
    "napi-dyn",
    "NAPI",
    "ArkTS VM",
    "native addon"
  ],
  "related": [
    "electron-ohos-native-addon-abi",
    "ffi-library-and-symbol-availability",
    "native-event-loop-handle-teardown"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Resolve addon NAPI calls against the owning Electron VM

## When to use

An OH Electron native addon has the correct Node ABI but fails to resolve NAPI symbols or crashes after binding to the ArkTS VM instead of Electron's environment.

## Recommended approach

For the affected runtime, resolve the required NAPI entrypoints from the actual loaded Electron library and forward addon calls consistently to that owner.

## Implementation steps

1. Confirm architecture and Node ABI first, then inspect how the host loads Electron and which library supplies each NAPI symbol. Matching symbol names do not establish VM compatibility.
2. Retain a handle to the intended already-loaded libelectron.so, using RTLD_NOLOAD where supported. Resolve the explicitly required NAPI symbol set against that handle, not the process-wide default provider.
3. Route all addon NAPI calls through the resolved function table and keep direct undefined NAPI imports out of the addon when this runtime requires that strategy. Check every required resolution before module registration.
4. Keep node headers, signatures and function-pointer declarations compatible with the target ABI. Initialize before other addon constructors can call NAPI, and keep the provider library alive while the addon runs.
5. Handle missing runtime/symbols as a clear initialization failure. Do not silently load an unrelated second runtime or mix a napi_env from one VM with functions from another.
6. Test any runtime-specific addon preload requirement before renderer startup separately; an early require cache can mask a later environment-lifetime defect.

## Conditions and alternatives

This is a scoped adapter for local symbol visibility and multiple NAPI providers, not a general replacement for normal Node addon linkage. The reviewed implementation resolves in a constructor and aborts on missing symbols; a consumer must choose its intended failure policy and validate provider identity.

## Common mistakes

Do not bind to the first same-named NAPI export, assume linking another libelectron guarantees the correct environment, or infer symbol ownership from ABI equality.

## Check it works

Load the addon before and after renderer creation in the intended signed PC app. Inspect resolved provider addresses and exercise representative callbacks and teardown; reject a missing/mismatched provider without executing against another VM.

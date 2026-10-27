---
{
  "schema_version": 1,
  "id": "electron-ohos-sessionmanager-prerequisite",
  "title": "Check the Electron window-session prerequisite on OH",
  "topic_key": "host.electron-ohos-sessionmanager-prerequisite",
  "area": "host",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Check product window capabilities and native display initialization separately from library/symbol presence; ArkTS guards alone cannot repair an unsupported prebuilt Electron window backend.",
  "when_to_use": "An Electron OH HAP installs, aa start succeeds, ArkTS reports 801 window errors and CrBrowserMain then crashes with an invalid display.",
  "applicability": "The reviewed Electron v37 OH host on a default-form-factor OpenHarmony image lacking Window.SessionManager. A same-source 2in1 success report is not current PC verification.",
  "keywords": [
    "SystemCapability.Window.SessionManager",
    "801",
    "CrBrowserMain",
    "OH_WindowManager_GetWindowProperties",
    "display id -1",
    "SIGSEGV"
  ],
  "related": [
    "electron-ohos-platform-capability-gates",
    "ohos-native-form-factor-classification",
    "native-dependency-closure"
  ],
  "status": "unresolved",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Check the Electron window-session prerequisite on OH

## When to use

An Electron OH HAP installs, aa start succeeds, ArkTS reports 801 window errors and CrBrowserMain then crashes with an invalid display.

## Recommended approach

Check product window capabilities and native display initialization separately from library/symbol presence; ArkTS guards alone cannot repair an unsupported prebuilt Electron window backend.

## Implementation steps

1. Record the exact OS/API/product capability set and runtime revision; compare Window.SessionManager with WindowManager.Core support.
2. Capture ArkTS window errors, native display IDs and fault logs. Verify native library dependencies and symbols independently rather than assuming a dlopen problem.
3. Guard optional ArkTS window-decor/status calls with capability checks and async error handling, then inspect whether the native runtime still requires the absent service.
4. Use a compatible product image or a runtime with an implemented/tested fallback. Keep the source case unresolved until startup and window behavior pass on that combination.

## Conditions and alternatives

The source diagnosis is a maintainer report supported by host guards and logs; the prebuilt native runtime implementation was not independently audited. A display-ID fallback is proposed, not demonstrated.

## Common mistakes

An exported NDK function may be present while its product capability is unavailable. Catching 801 prevents only the ArkTS exception, not a later native failure.

## Check it works

On each intended PC/product image, verify process survival, a usable first frame, valid display identity, resize and window lifecycle. aa start acceptance alone is insufficient.

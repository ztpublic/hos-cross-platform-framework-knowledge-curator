---
{
  "schema_version": 1,
  "id": "ohos-input-injection-authorization",
  "title": "Request authorization before injecting OS input events on OH",
  "topic_key": "input.ohos-input-injection-authorization",
  "area": "input",
  "scope": "common",
  "frameworks": [],
  "summary": "Call OH_Input_RequestInjection and wait for its callback before OH_Input_InjectMouseEvent or OH_Input_InjectKeyEvent, and stamp events with monotonic time so click detection works.",
  "when_to_use": "Simulated mouse or keyboard events are rejected, or injected double-clicks register as separate single clicks.",
  "applicability": "Native apps using the OH input NDK (ohinput) for automation or remote control. A user authorization prompt appears; the reviewed code does not show a permission declaration, so check the current API docs and product policy. Coordinates assume display 0 in the reviewed code, which does not fit multi-monitor PCs.",
  "keywords": [
    "OH_Input_RequestInjection",
    "OH_Input_InjectMouseEvent",
    "OH_Input_InjectKeyEvent",
    "INPUT_INJECTION_AUTHORIZING",
    "OH_Input_GetPointerLocation",
    "double-click",
    "input injection",
    "remote control"
  ],
  "related": [
    "arkweb-synthetic-input-exclusions",
    "ohos-virtual-hid-device-injection",
    "electron-ohos-arkts-call-bridge"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Request authorization before injecting OS input events on OH

## When to use

Simulated mouse or keyboard events are rejected, or injected double-clicks register as separate single clicks.

## Recommended approach

Call OH_Input_RequestInjection and wait for its callback before OH_Input_InjectMouseEvent or OH_Input_InjectKeyEvent, and stamp events with monotonic time so click detection works.

## Implementation steps

1. Call OH_Input_RequestInjection with a callback. Treat INPUT_SUCCESS and INPUT_INJECTION_AUTHORIZING as pending until the callback delivers a status, INPUT_INJECTION_AUTHORIZED as granted, and anything else as an error.
2. Wait with a timeout long enough for a person to respond (60 s in the reviewed code) and report a denial to the caller.
3. Create events with OH_Input_CreateMouseEvent or OH_Input_CreateKeyEvent, and set action, button or key, and position.
4. Set action time from CLOCK_MONOTONIC in microseconds. The source comment says this makes double-clicks recognizable.
5. Choose the display explicitly with OH_Input_SetMouseEventDisplayId, using the display from OH_Input_GetPointerLocation or your mapping rather than a constant. Destroy each event after injection.

## Conditions and alternatives

Injection acts on the whole system, so tie it to an explicit user action. A virtual HID device is the alternative for hardware-like input.

## Common mistakes

Injecting while authorization is pending. Using a wall clock. Hard-coding display 0. Keeping one global promise so overlapping requests overwrite each other.

## Check it works

Test first-use grant, denial and timeout. Then inject single and double clicks, drags and key combinations across two displays, and check the target receives them in another window.

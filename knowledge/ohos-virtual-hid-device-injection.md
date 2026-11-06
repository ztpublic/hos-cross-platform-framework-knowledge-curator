---
{
  "schema_version": 1,
  "id": "ohos-virtual-hid-device-injection",
  "title": "Present peripheral input as a virtual HID device on OH",
  "topic_key": "peripheral.ohos-virtual-hid-device-injection",
  "area": "peripheral",
  "scope": "common",
  "frameworks": [],
  "summary": "Create a virtual device with OH_Hid_CreateDevice declaring its properties and events, then send event batches ending in SYN_REPORT with OH_Hid_EmitEvent from a dedicated thread.",
  "when_to_use": "Pen pressure or remapped keys read from a peripheral are not seen as system input by other applications.",
  "applicability": "Driver-extension apps with ACCESS_DDK_HID on HarmonyOS 2in1, linking libhid.z.so. Sample-level evidence with a pen pad and keyboard; pressure behavior in target apps was not established.",
  "keywords": [
    "OH_Hid_CreateDevice",
    "OH_Hid_EmitEvent",
    "OH_Hid_DestroyDevice",
    "HID_PROP_DIRECT",
    "HID_ABS_PRESSURE",
    "HID_BTN_TOOL_PEN",
    "SYN_REPORT",
    "pen pressure",
    "virtual device"
  ],
  "related": [
    "ohos-usb-driver-extension-binding",
    "ohos-input-injection-authorization",
    "device-gesture-capability"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Present peripheral input as a virtual HID device on OH

## When to use

Pen pressure or remapped keys read from a peripheral are not seen as system input by other applications.

## Recommended approach

Create a virtual device with OH_Hid_CreateDevice declaring its properties and events, then send event batches ending in SYN_REPORT with OH_Hid_EmitEvent from a dedicated thread.

## Implementation steps

1. Define a Hid_Device and Hid_EventProperties for each logical device: event types and key codes for a keyboard, and direct property, absolute axes and tool buttons for a pen pad.
2. Set each axis minimum and maximum to match the hardware scale, including pressure, and scale incoming reports accordingly.
3. Create devices with OH_Hid_CreateDevice and keep the returned ID, checking for failure.
4. Queue events and emit on a dedicated thread: tool button, ABS_X, ABS_Y, ABS_PRESSURE, then the sync report closing the batch, so USB reads are not blocked by injection.
5. Destroy devices with OH_Hid_DestroyDevice on disconnect and exit.

## Conditions and alternatives

This registers a device, unlike event injection, which needs per-app authorization and has no hardware-like identity. Prefer standard HID where the system already handles the device.

## Common mistakes

Omitting the sync report, so events are not applied. Using mismatched axis ranges. Leaving virtual devices behind after a crash or unplug.

## Check it works

In a pressure-sensitive app, draw with varied pressure and eraser, remap keys, then unplug and replug. Confirm no duplicate device or stuck tool state remains.

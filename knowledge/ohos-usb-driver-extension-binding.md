---
{
  "schema_version": 1,
  "id": "ohos-usb-driver-extension-binding",
  "title": "Claim a vendor USB device through an OH driver extension",
  "topic_key": "peripheral.ohos-usb-driver-extension-binding",
  "area": "peripheral",
  "scope": "common",
  "frameworks": [],
  "summary": "Declare a driver extension matched by bus, vid and pid, bind it from the UI with deviceManager and talk to it over RPC, then claim the interface in native USB DDK code.",
  "when_to_use": "A normal ability cannot claim or read a vendor USB device such as a pen tablet, and needs a driver-level path on a PC.",
  "applicability": "HarmonyOS 2in1 apps using the Driver Development Kit. Requires ACCESS_DDK_USB, ACCESS_EXTENSIONAL_DEVICE_DRIVER and for HID ACCESS_DDK_HID, which may need special approval for your distribution channel; check before designing around it. Sample-level evidence with one device.",
  "keywords": [
    "driver extension",
    "extensionAbilities driver",
    "ACCESS_DDK_USB",
    "deviceManager.queryDevices",
    "bindDeviceDriver",
    "OH_Usb_ClaimInterface",
    "OH_Usb_SendPipeRequest",
    "USB DDK",
    "vid pid"
  ],
  "related": [
    "ohos-virtual-hid-device-injection",
    "ohos-usbfs-preload-compat",
    "privileged-capability-gating"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Claim a vendor USB device through an OH driver extension

## When to use

A normal ability cannot claim or read a vendor USB device such as a pen tablet, and needs a driver-level path on a PC.

## Recommended approach

Declare a driver extension matched by bus, vid and pid, bind it from the UI with deviceManager and talk to it over RPC, then claim the interface in native USB DDK code.

## Implementation steps

1. In module.json5 declare an extensionAbilities entry of type driver with metadata for bus USB, vid and pid, and request the DDK permissions.
2. In the UI call deviceManager.queryDevices(BusType.USB), find the device, call bindDeviceDriver and keep the returned remote object. Re-bind in onRemoteDied.
3. In the extension handle onRemoteMessageRequest and pass requests to native code. Set context.area before using preferences if the driver can run before user unlock, as the sample sets EL1.
4. In native code call OH_Usb_Init, OH_Usb_GetConfigDescriptor and OH_Usb_ClaimInterface, then use OH_Usb_CreateDeviceMemMap and OH_Usb_SendPipeRequest for transfers. Find the interface from the descriptor rather than hard-coding it.
5. Release the interface and memory maps and finish the DDK on disconnect and unbind.

## Conditions and alternatives

For a device already handled as standard HID or for simple control, usbManager with user authorization may be enough. An extension runs separately from the UI process, so design for RPC failure.

## Common mistakes

Hard-coding the interface number from one device. Omitting re-bind after the extension dies. Assuming an app with the permission declared passes signing and store checks.

## Check it works

Plug and unplug during use, kill the extension, sleep and resume, and connect a non-matching device. Confirm rebind works and no transfers leak.

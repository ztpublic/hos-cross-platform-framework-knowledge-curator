---
{
  "schema_version": 1,
  "id": "ohos-usbfs-preload-compat",
  "title": "Give spawned libusb tools device access through a preload shim",
  "topic_key": "peripheral.ohos-usbfs-preload-compat",
  "area": "peripheral",
  "scope": "common",
  "frameworks": [],
  "summary": "Open the device in ArkTS with usbManager, pass its fd to the child over a local socket with SCM_RIGHTS, and use an LD_PRELOAD library to supply the sysfs view that libusb expects.",
  "when_to_use": "libusb tools such as adb, fastboot or avrdude find no devices in an OH app child process, because the sandbox blocks /sys/bus/usb listing and there are no /dev/ttyUSB nodes.",
  "applicability": "OH PC apps running dynamically linked command-line tools as child processes. Evidence is one port's source and its documented device tests. This is heavy interception, not a platform contract, and may break with OS updates. Store acceptability is not established.",
  "keywords": [
    "libusb",
    "LD_PRELOAD",
    "SCM_RIGHTS",
    "usbManager.getFileDescriptor",
    "statfs",
    "SYSFS_MAGIC",
    "adb",
    "fastboot",
    "avrdude",
    "CH34x",
    "ttyUSB",
    "pthread_atfork"
  ],
  "related": [
    "ohos-usb-driver-extension-binding",
    "hap-executable-declaration",
    "electron-ohos-non-pty-exec"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Give spawned libusb tools device access through a preload shim

## When to use

libusb tools such as adb, fastboot or avrdude find no devices in an OH app child process, because the sandbox blocks /sys/bus/usb listing and there are no /dev/ttyUSB nodes.

## Recommended approach

Open the device in ArkTS with usbManager, pass its fd to the child over a local socket with SCM_RIGHTS, and use an LD_PRELOAD library to supply the sysfs view that libusb expects.

## Implementation steps

1. In ArkTS, check usbManager.hasRight and call requestRight, then connectDevice and getFileDescriptor.
2. Serve fds on a Unix socket in the app sandbox, so the child receives them through recvmsg with SCM_RIGHTS and does its own ioctls, which keeps data out of ArkTS.
3. Build a preload library exposing a synthetic /sys and answering statfs on it with the sysfs magic number, as the libusb Linux backend checks that value.
4. Add a pthread_atfork guard so a child forked while a hook lock is held cannot deadlock.
5. For serial chips the system serial API does not list, serve a pty and translate termios settings into vendor requests, as the reviewed port does for CH34x.

## Conditions and alternatives

Static libusb binaries cannot be intercepted, so they need dynamic builds. The source documents Electron-as-Node children hanging under the preload, so use a separate pure-JS socket client for those and exclude them from LD_PRELOAD. Prefer a direct usbManager path when you can modify the tool.

## Common mistakes

Setting LD_PRELOAD for every child process. Assuming static tools work. Missing fork safety. Sending bulk data through ArkTS, which makes it a bottleneck.

## Check it works

List and use a real device from each tool, including permission denial, unplug during transfer and multi-threaded forks. Confirm non-USB children run without the preload.

---
{
  "schema_version": 1,
  "id": "native-qr-scan-session-contract",
  "topic_key": "media.native-qr-scan-session-contract",
  "scope": "common",
  "frameworks": [],
  "title": "Bridge native QR scanning without confusing startup with a result",
  "area": "media",
  "summary": "Model scanning as a capability-gated session with correlated result events, explicit cancellation and cleanup.",
  "when_to_use": "A cross-platform scanner lacks an OH backend, returns before recognition, or receives stale results after stopping and retrying.",
  "keywords": [
    "ScanKit",
    "scanBarcode",
    "startScanForResult",
    "QR",
    "camera",
    "EventChannel",
    "扫码",
    "扫描取消",
    "相机权限",
    "late scan result"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": [
    "capability-gated-backend-initialization",
    "picker-cancellation-concurrency"
  ]
}
---

# Bridge native QR scanning without confusing startup with a result

## When to use

A cross-platform scanner lacks an OH backend, returns before recognition, or receives stale results after stopping and retrying.

## Recommended approach

Choose a supported native scanning backend and represent one scan as a session. Keep presentation acceptance separate from recognition, and correlate every event and cleanup operation with that session.

## Implementation steps

1. Verify the selected SDK, product service and device support. HarmonyOS Scan Kit can provide a default scanning UI through `scanBarcode.startScanForResult`; it is a product-specific choice, not a universal OpenHarmony capability. Use a valid UIAbility context within the UI/component lifetime. Keep manual entry or another verified backend available.
2. Declare and handle camera authorization required by the chosen backend. Return permission denial distinctly; a registered channel or successful `ping` does not establish camera/service availability.
3. Acquire a native presentation guard before awaiting permissions or opening the scanner. Assign a session identifier. Reject or serialize additional starts until that native operation has actually ended.
4. Prefer returning the final native result through a single channel Future for a one-shot scanner. If the interface uses a start operation plus an event stream, subscribe and confirm native listener readiness before starting; include the session identifier in accepted/result/cancel/error/stopped messages. A true startup response is not recognized content.
5. Validate recognized content and barcode format before passing it to application routing or account operations. Deliver one terminal result, preserving native cancellation/error codes rather than relying on translated exception text.
6. Define stop precisely. If the backend cannot dismiss its presentation, invalidate the session and ignore its eventual result, but retain the presentation guard until it exits. A Boolean flag alone does not stop a native camera UI. Old completion handlers must not stop or clear a newer session.
7. Release application subscriptions, native event handlers and context references in terminal and detach paths. Put caller cleanup in `finally`, including start failures. A caller timeout also needs native cleanup or invalidation.

## Conditions and alternatives

A default native scanner owns its own UI and camera behavior. Do not advertise torch or preview control by emitting synthetic state events when no matching native operation exists. If an OH backend is unavailable, create a fallback camera controller only when that fallback supports the target; otherwise expose manual entry.

Use [picker cancellation and concurrency](picker-cancellation-concurrency.md) for a one-shot presentation protocol and [capability-gated initialization](capability-gated-backend-initialization.md) when choosing the fallback.

## Common mistakes

Eagerly creating an unsupported fallback controller can fail before backend selection. Repeated start calls can race through permission requests; a late result or `finally` block can then affect the wrong scan.

## Check it works

Exercise permission denial, user cancel, repeated taps, start failure, disposal during permission/presentation, stop followed by retry, missing event listeners and delayed old results. Confirm one application result per session and truthful controls. Verify the selected service on a supported device.

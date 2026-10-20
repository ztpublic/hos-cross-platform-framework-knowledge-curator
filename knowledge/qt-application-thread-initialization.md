---
{
  "schema_version": 1,
  "id": "qt-application-thread-initialization",
  "title": "Initialize Qt objects on the thread that owns its event loop",
  "topic_key": "lifecycle.qt-application-thread-initialization",
  "area": "lifecycle",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Trace early Qt object initialization and keep Qt application construction aligned with the OH adapter's designated event-loop thread.",
  "when_to_use": "QCoreApplication::notify asserts that the application main thread does not match the current thread.",
  "keywords": [
    "theMainThread",
    "QCoreApplication::notify",
    "QThread",
    "ArkTS"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to Qt OH dual-thread integration; exact startup callbacks and private state differ by version.",
  "related": []
}
---

# Initialize Qt objects on the thread that owns its event loop

## When to use

QCoreApplication::notify asserts that the application main thread does not match the current thread.

## Recommended approach

Trace early Qt object initialization and keep Qt application construction aligned with the OH adapter's designated event-loop thread.

## Implementation steps

1. Identify the ArkTS/JS thread and Qt event-loop thread in the selected integration.
2. Inspect static/global initializers and callbacks that touch QThread or QObject before QApplication exists.
3. Move premature Qt initialization into the owning Qt startup phase, and marshal cross-thread operations through supported gateways.
4. Preserve the adapter's shutdown ordering rather than resetting private main-thread state to hide the assertion.

## Conditions and alternatives

Applies to Qt OH dual-thread integration; exact startup callbacks and private state differ by version.

## Common mistakes

A JS callback running on the UI thread is not automatically running on the Qt application thread.

## Check it works

Observe thread identities through cold startup, callback delivery and shutdown; exercise the previously failing path.

---
{
  "schema_version": 1,
  "id": "tauri-ohos-platform-plugin-gates",
  "title": "Gate Tauri OH plugins across dependencies and commands",
  "topic_key": "plugins.tauri-ohos-platform-plugin-gates",
  "area": "plugins",
  "scope": "framework-specific",
  "frameworks": [
    "tauri"
  ],
  "summary": "Select OH-compatible plugin dependencies, gate plugin registration and command implementations together, and expose unsupported operations as explicit UI capability states.",
  "when_to_use": "A Tauri OH build selects Linux-only dependencies, or the app renders but tray, updater, dialog and opener calls fail at runtime.",
  "applicability": "Tauri OH experimental branches where target_os is linux and target_env is ohos. Individual plugin/backend support differs by revision; this guidance does not declare those features universally unsupported.",
  "keywords": [
    "target_env = ohos",
    "cfg desktop",
    "tauri-plugin-dialog",
    "updater",
    "capabilities",
    "ACL",
    "webkit2gtk"
  ],
  "related": [
    "rust-ohos-target-cfg",
    "tauri-ohos-dependency-cohort",
    "tauri-ohos-plugin-command-registration"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Gate Tauri OH plugins across dependencies and commands

## When to use

A Tauri OH build selects Linux-only dependencies, or the app renders but tray, updater, dialog and opener calls fail at runtime.

## Recommended approach

Select OH-compatible plugin dependencies, gate plugin registration and command implementations together, and expose unsupported operations as explicit UI capability states.

## Implementation steps

1. For desktop Linux dependencies, exclude `target_env = "ohos"` explicitly. Match the application manifest, lockfile and framework patch versions.
2. Gate imports, Builder.plugin registration and affected command bodies using the same actual backend availability; inspect how the selected fork defines desktop/mobile cfgs.
3. Use an OH capability file with the fork's `openHarmony` platform identifier and only supported permissions. An ACL entry cannot create a missing native implementation.
4. Return a clear unsupported result from unavailable operations and disable/explain the matching UI. Retain a functional quit/restore path if a tray backend is absent.

## Conditions and alternatives

Runtime feature probing may be needed even when compilation succeeds. A rejection from an unregistered plugin is not evidence that the entire application is broken.

## Common mistakes

Only gating Cargo dependencies leaves unconditional Rust imports or frontend invocations. Treating the Linux target_os label as desktop Linux selects the wrong stack.

## Check it works

Inspect resolved dependency sources and exercise each exposed command on the intended OS build. Regress supported desktop targets after changing conditional code.

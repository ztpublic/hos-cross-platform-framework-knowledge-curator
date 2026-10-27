---
{
  "schema_version": 1,
  "id": "rust-ohos-target-cfg",
  "title": "Distinguish OH target environment from desktop Linux cfg",
  "topic_key": "native.rust-ohos-target-cfg",
  "area": "native",
  "scope": "common",
  "frameworks": [],
  "summary": "Inspect target_os, target_env and framework-injected cfg separately and gate by the operation's actual platform semantics.",
  "when_to_use": "A Rust native dependency selects desktop integration for an OH target.",
  "keywords": [
    "Rust",
    "target_env",
    "target_os",
    "cfg",
    "linux",
    "Tauri",
    "GTK",
    "WebKit2GTK",
    "mobile cfg"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Common native-layer knowledge for applications embedding Rust; not evidence that every UI framework supplies an OH Rust bridge.",
  "related": []
}
---

# Distinguish OH target environment from desktop Linux cfg

## When to use

A Rust native dependency selects desktop integration for an OH target.

## Recommended approach

Inspect target_os, target_env and framework-injected cfg separately and gate by the operation's actual platform semantics.

## Implementation steps

1. Inspect rustc target cfg for the selected target, including target_env = "ohos" where applicable.
2. Review branches guarded only by Linux or desktop and identify the native operation they enable.
3. Exclude OH from desktop-only implementations or select an explicit OH adapter, without mechanically rewriting every Linux branch.
4. Build each supported target and confirm runtime backend selection.

## Conditions and alternatives

An OH Tauri adapter can define its framework `mobile` cfg from `CARGO_CFG_TARGET_ENV == "ohos"` while the compiler still reports `target_os = "linux"`. Desktop-only GTK/WebKit2GTK dependencies therefore need an explicit `not(target_env = "ohos")` condition. This framework classification describes its backend contract; it does not identify the physical device as a phone or remove the need for PC behavior checks.

Common native-layer knowledge for applications embedding Rust; not evidence that every UI framework supplies an OH Rust bridge.

## Common mistakes

Framework desktop/mobile flags and compiler target attributes are not interchangeable.

## Check it works

Compile cfg probes and exercise the selected storage/system backend on the actual OH application.

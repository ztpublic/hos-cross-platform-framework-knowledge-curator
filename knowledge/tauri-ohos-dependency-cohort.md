---
{
  "schema_version": 1,
  "id": "tauri-ohos-dependency-cohort",
  "title": "Pin a compatible Tauri OH runtime and host dependency set",
  "topic_key": "build.tauri-ohos-dependency-cohort",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "tauri"
  ],
  "summary": "Resolve the OH-capable Tauri, tao, wry, Ability Rust crates and ArkTS HAR as one compatibility set, including the CLI and generated host.",
  "when_to_use": "An OH build pulls GTK/WebKit2GTK, loses the webview feature, or stops compiling after updating one experimental dependency.",
  "keywords": [
    "Tauri",
    "Cargo.lock",
    "tao",
    "wry",
    "openharmony-ability",
    "@ohos-rs/ability",
    "GTK",
    "webview feature",
    "dependency drift"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Experimental Tauri v2 OH branches and forks; a PC HAP still needs its own window, keyboard, pointer and lifecycle checks. An experimental branch does not establish released framework support.",
  "related": [
    "rust-ohos-target-cfg",
    "tauri-ohos-ability-host",
    "tauri-ohos-ability-white-screen"
  ]
}
---

# Pin a compatible Tauri OH runtime and host dependency set

## When to use

An OH build pulls GTK/WebKit2GTK, loses the webview feature, or stops compiling after updating one experimental dependency.

## Recommended approach

Resolve the OH-capable Tauri, tao, wry, Ability Rust crates and ArkTS HAR as one compatibility set, including the CLI and generated host.

## Implementation steps

1. Identify the actual Tauri CLI source and the application runtime sources separately; their package version numbers can differ.
2. Inspect `Cargo.lock` for resolved Tauri family, tao, wry and Ability revisions. A branch name alone is a moving reference.
3. Apply compatible root-workspace Cargo patches to transitive crates where required; an application dependency override alone may leave another runtime copy.
4. Pair the Rust Ability crates with the host `@ohos-rs/ability` HAR and its generated Ability API. Keep the native library name and initialization macro consistent.
5. Preserve the lockfile and record source pins before updating. Review dependency API/features and host changes together.
6. Keep version-mismatch checks enabled; inspect the target dependency tree for accidental GTK/WebKit2GTK or unsupported plugin dependencies.

## Conditions and alternatives

A full application can source-build the HAR and compare its Rust crate-tree identity with the runtime source before packaging. This checks interface input agreement; it does not prove full ABI or runtime compatibility. Apply manifest overlays in a disposable build checkout so other targets retain their own dependency graph.

One source set pins Ability before a later webview API removal; another uses a maintained fork and a different host generation. These are separate compatibility sets. Do not update a Git dependency to its newest branch tip solely because the package name matches. `#[cfg(target_env = "ohos")]` selects the OH branch; the framework may classify it as mobile internally even on a physical PC.

## Common mistakes

Mixing stable crates with one experimental runtime crate; treating Android support as OH support; applying Cargo patches in a non-root manifest; using an unpinned HAR with old Rust bindings.

## Check it works

Resolve the dependency tree, inspect target artifacts and package the HAP. On the intended PC, verify first-page rendering, a real frontend-to-Rust command and response, resize and window-stage recreation. A compile or installed HAP alone does not establish those behaviors.

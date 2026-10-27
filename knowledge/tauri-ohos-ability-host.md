---
{
  "schema_version": 1,
  "id": "tauri-ohos-ability-host",
  "title": "Load the Tauri cdylib through its matching OH Ability host",
  "topic_key": "host.tauri-ohos-ability-host",
  "area": "host",
  "scope": "framework-specific",
  "frameworks": [
    "tauri"
  ],
  "summary": "Export the OH entry point from a Rust cdylib and let the matching Ability package own native loading, webview creation and lifecycle forwarding.",
  "when_to_use": "A Tauri OH HAP opens only a template page, cannot load the Rust module, or never starts its webview runtime.",
  "keywords": [
    "Tauri",
    "RustAbility",
    "NativeAbility",
    "moduleName",
    "cdylib",
    "mobile_entry_point",
    "libentry.so",
    "WindowStage"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Tauri OH adapters using RustAbility or NativeAbility. Include the intended PC device type in the module and validate window-stage, focus and resize behavior on that product.",
  "related": [
    "tauri-ohos-dependency-cohort",
    "tauri-ohos-ability-white-screen",
    "native-registration-and-handler-coverage"
  ]
}
---

# Load the Tauri cdylib through its matching OH Ability host

## When to use

A Tauri OH HAP opens only a template page, cannot load the Rust module, or never starts its webview runtime.

## Recommended approach

Export the OH entry point from a Rust cdylib and let the matching Ability package own native loading, webview creation and lifecycle forwarding.

## Implementation steps

1. Declare a Rust library with `crate-type` including `cdylib`; give it an unambiguous library name.
2. Annotate the runtime function with the adapter-supported `#[cfg_attr(mobile, tauri::mobile_entry_point)]`. The OH macro creates the Ability-facing initialization entry point.
3. Generate or maintain the OH host from the same compatible CLI generation. Its `EntryAbility` must extend the matching package class and set `moduleName` to the Rust library base name, without the `lib` prefix or `.so` suffix.
4. For the RustAbility generation, select `mode = "webview"` and the adapter-required default page. NativeAbility examples have a different host shape; follow that package contract instead of copying fields blindly.
5. Preserve required superclass lifecycle calls when overriding `onCreate` or `onWindowStageCreate`. Package the Rust `.so` under the correct ABI and declare native typing dependencies when importing it from ArkTS.
6. Verify the module entry points to this Ability and includes the intended device type. The template `libentry.so` is not the Tauri application library.

## Conditions and alternatives

The generated sample Index page can remain template scaffolding while the Ability package provides the runtime page. A visible Hello World template or successful native add function does not establish Tauri initialization. Fullscreen/layout policy belongs to the owning PC window, independently of which Ability class is used.

## Common mistakes

Loading `libentry.so` as if it contained the app; omitting the exported OH entry point; losing superclass lifecycle calls; mixing RustAbility and NativeAbility APIs; declaring 2in1 support as proof of PC behavior.

## Check it works

Inspect the HAP for the correct Rust library and ABI. On the target PC, cold launch, invoke a Rust command from the frontend, resize, change focus, background/foreground and recreate the window stage; inspect logs for duplicate initialization or stale handles.

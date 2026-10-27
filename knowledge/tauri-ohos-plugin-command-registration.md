---
{
  "schema_version": 1,
  "id": "tauri-ohos-plugin-command-registration",
  "title": "Connect an OH Tauri plugin command through registration and capabilities",
  "topic_key": "integration.tauri-ohos-plugin-command-registration",
  "area": "integration",
  "scope": "framework-specific",
  "frameworks": [
    "tauri"
  ],
  "summary": "Connect the Rust plugin handler, application registration, generated command permissions, window capability and TypeScript invoke name as one command contract.",
  "when_to_use": "A custom OH Tauri plugin compiles but its frontend command is unknown, denied or never reaches managed Rust state.",
  "keywords": [
    "Tauri",
    "plugin",
    "invoke",
    "capabilities",
    "permissions",
    "generate_handler",
    "guest-js",
    "unknown command",
    "command denied"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Command-only Tauri v2 plugins on a compatible OH runtime. Native PC services require a separate OH implementation and permission/lifecycle checks.",
  "related": [
    "tauri-ohos-ability-host",
    "rust-ohos-target-cfg",
    "native-registration-and-handler-coverage"
  ]
}
---

# Connect an OH Tauri plugin command through registration and capabilities

## When to use

A custom OH Tauri plugin compiles but its frontend command is unknown, denied or never reaches managed Rust state.

## Recommended approach

Connect the Rust plugin handler, application registration, generated command permissions, window capability and TypeScript invoke name as one command contract.

## Implementation steps

1. Define the Rust plugin with `tauri::plugin::Builder`, register its commands using `generate_handler!`, and initialize managed state during setup if handlers require it.
2. List exposed commands in `tauri_plugin::Builder::new(COMMANDS)` in the plugin build script so the corresponding command permissions exist.
3. Define the plugin default permission set, then add its namespaced permission to the application capability matching the actual WebView/window label.
4. Add the Rust dependency and call `.plugin(plugin_crate::init())` on the application builder. Compilation alone does not perform this registration.
5. Keep the TypeScript API invoking `plugin:<plugin-name>|<command-name>` with argument keys matching the handler. Test a command with an observable input/result rather than only a platform-name constant.
6. Use `#[cfg(target_env = "ohos")]` for OH-specific native branches. A Rust-only command need not add ArkTS scaffolding; add a real N-API/ArkTS adapter when the feature requires OH APIs.

## Conditions and alternatives

Tauri command authorization and OH operating-system permissions are distinct. A plugin capability does not grant file picker, clipboard, background or system-service privileges. Returning a string naming OH is a compile-time branch check, not evidence that a native service works.

## Common mistakes

Registering a dependency without registering the plugin; mismatching plugin and command names; missing generated permissions or a matching capability; assuming Android/iOS native plugin code serves OH; treating an echo test as native-service validation.

## Check it works

From the actual PC WebView, invoke a command with a distinctive argument and assert its returned result. Check an ungranted command is denied, and verify the native feature separately when one exists.

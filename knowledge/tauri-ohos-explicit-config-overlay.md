---
{
  "schema_version": 1,
  "id": "tauri-ohos-explicit-config-overlay",
  "title": "Pass the OH config explicitly when the experimental CLI selects Android",
  "topic_key": "build.tauri-ohos-explicit-config-overlay",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "tauri"
  ],
  "summary": "Inspect the OH command configuration target and pass the OH overlay with `--config` when the selected CLI incorrectly requests Android configuration.",
  "when_to_use": "An experimental OH build ignores `tauri.ohos.conf.json` or unexpectedly inherits Android build settings despite documenting OH overlay support.",
  "keywords": [
    "Tauri",
    "tauri.ohos.conf.json",
    "--config",
    "Target::Android",
    "configuration ignored",
    "platform overlay"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "Only CLI revisions whose OH build calls the config loader with Target::Android. Corrected forks may load the OH overlay automatically. Check the effective PC bundle inputs and native manifest separately.",
  "related": [
    "tauri-ohos-dependency-cohort"
  ]
}
---

# Pass the OH config explicitly when the experimental CLI selects Android

## When to use

An experimental OH build ignores `tauri.ohos.conf.json` or unexpectedly inherits Android build settings despite documenting OH overlay support.

## Recommended approach

Inspect the OH command configuration target and pass the OH overlay with `--config` when the selected CLI incorrectly requests Android configuration.

## Implementation steps

1. Inspect the selected CLI OH build path and the target passed to the configuration loader; the existence of an OH platform enum or help text does not prove the command uses it.
2. If the command requests `Target::Android`, inspect `tauri.android.conf.json` for settings that could be merged unexpectedly.
3. As a source-derived workaround, run `cargo tauri ohos build --config /absolute/path/to/tauri.ohos.conf.json`; explicit merge arguments are applied after the automatic platform overlay in the reviewed loader.
4. Include every conflicting setting intentionally. A JSON merge does not remove unrelated inherited Android settings unless the chosen merge patch explicitly handles them.
5. Prefer a corrected CLI revision that requests the OH target, after checking compatibility with the application runtime. Keep native `AppScope/app.json5` metadata under explicit review as well.

## Conditions and alternatives

This workaround follows the inspected config merge path and has not been executed. A different CLI fork may already correct platform selection; verify its code or effective configuration before applying the workaround.

## Common mistakes

Trusting the command help over its selected target; assuming an explicit overlay clears every inherited key; globally renaming Android configuration files; assuming Rust-side app metadata automatically updates the OH bundle.

## Check it works

Use a harmless distinguishing build setting to inspect the effective merged config and output. Verify the generated OH assets and bundle metadata, then check the resulting PC app separately. This entry does not claim a successful build.

---
{
  "schema_version": 1,
  "id": "tauri-ohos-production-custom-protocol",
  "title": "Build embedded Tauri OH assets in production protocol mode",
  "topic_key": "host.tauri-ohos-production-custom-protocol",
  "area": "host",
  "scope": "framework-specific",
  "frameworks": [
    "tauri"
  ],
  "summary": "Enable the selected Tauri fork's custom-protocol feature for packaged assets and check the effective frontend distribution path; release optimization alone does not select production asset loading.",
  "when_to_use": "A packaged Tauri OH webview reports Failed to request tauri://localhost or attempts to reach a development server after the HAP is installed offline.",
  "applicability": "Experimental Tauri OH forks where build.rs derives dev as the inverse of custom-protocol. Apply only after checking the pinned fork and effective config; the source reports a PC run, not curator validation.",
  "keywords": [
    "custom-protocol",
    "cfg dev",
    "frontendDist",
    "devUrl",
    "tauri://localhost",
    "offline",
    "white screen"
  ],
  "related": [
    "tauri-ohos-ability-white-screen",
    "tauri-ohos-explicit-config-overlay",
    "tauri-ohos-dependency-cohort"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Build embedded Tauri OH assets in production protocol mode

## When to use

A packaged Tauri OH webview reports Failed to request tauri://localhost or attempts to reach a development server after the HAP is installed offline.

## Recommended approach

Enable the selected Tauri fork's custom-protocol feature for packaged assets and check the effective frontend distribution path; release optimization alone does not select production asset loading.

## Implementation steps

1. Inspect the pinned framework build.rs and the application's feature graph. In the reviewed generation, `dev = !has_feature("custom-protocol")`.
2. Enable `tauri/custom-protocol` through a production feature or dependency feature when packaging embedded frontend assets. Do not assume `--release` enables it.
3. Verify `frontendDist` resolves to the prepared static output and the correct OH config overlay is active. Confirm the custom-scheme request serves packaged assets rather than forwarding to devUrl.
4. Keep development-server builds intentionally separate and preserve the matching Ability/runtime dependency cohort.

## Conditions and alternatives

The source exposes a static global Tauri frontend; this does not imply every bundler or newer CLI needs a manual feature flag. A white screen can also come from Ability mismatch, missing IPC or frontend exceptions.

## Common mistakes

Changing the URL alone does not create embedded assets. Mixing an old CLI with a new build/runtime can change feature/config semantics.

## Check it works

Install the packaged build with the dev server stopped and network unavailable, load nested assets and invoke a Rust command. Check that no development-host requests occur.

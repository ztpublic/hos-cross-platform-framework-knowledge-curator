---
{
  "schema_version": 1,
  "id": "tauri-ohos-ability-white-screen",
  "title": "Check the resolved Ability HAR when a Tauri OH page stays white",
  "topic_key": "web.tauri-ohos-ability-white-screen",
  "area": "web",
  "scope": "framework-specific",
  "frameworks": [
    "tauri"
  ],
  "summary": "Align the resolved Ability HAR, generated host API and Rust adapter generation; reconcile both the manifest and lockfile before investigating the frontend.",
  "when_to_use": "An initialized Tauri OH application packages and launches but its webview remains white, especially with an older generated Ability dependency.",
  "keywords": [
    "Tauri",
    "white screen",
    "blank page",
    "白屏",
    "@ohos-rs/ability",
    "oh-package-lock.json5",
    "0.2.1",
    "0.4.0-beta.0"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "A reported HarmonyOS 6.0 PC failure in the RustAbility generation. The historical version remedy is not a universal fix for NativeAbility or newer backends.",
  "related": [
    "tauri-ohos-ability-host",
    "tauri-ohos-dependency-cohort"
  ]
}
---

# Check the resolved Ability HAR when a Tauri OH page stays white

## When to use

An initialized Tauri OH application packages and launches but its webview remains white, especially with an older generated Ability dependency.

## Recommended approach

Align the resolved Ability HAR, generated host API and Rust adapter generation; reconcile both the manifest and lockfile before investigating the frontend.

## Implementation steps

1. Capture the actual Rust runtime revisions, generated Ability class and resolved `@ohos-rs/ability` HAR version. A manifest edit alone does not establish what was installed.
2. For the affected RustAbility integration, the source-reported remedy changes `@ohos-rs/ability` from `0.2.1` to `0.4.0-beta.0`. Apply it only with a compatible Rust/backend set.
3. Re-resolve the host dependencies with the package manager and inspect `oh-package-lock.json5`. A reviewed template declares the newer HAR while its bundled lockfile still names the old one; verify the effective resolution instead of assuming either file wins.
4. Regenerate or reconcile the host class, initialization and lifecycle configuration for the selected package generation, then rebuild and sign the application as a separate execution task.
5. If the compatible HAR is actually installed and the page remains white, inspect native module loading, first navigation, packaged assets and frontend IPC separately.

## Conditions and alternatives

The reported PC success is historical external evidence. Source review confirms the manifest/lockfile mismatch and the test project dependency, but no package-manager resolution or device behavior was reproduced here. Do not prescribe the old beta to every new project.

## Common mistakes

Blindly selecting the latest HAR; changing only the manifest; treating a closed issue as current runtime validation; assuming every white page has this cause.

## Check it works

Confirm the resolved HAR and host API, launch on the affected PC OS/API, observe first-page rendering, run an IPC round trip and repeat after a cold restart. Record the selected dependency set alongside the device evidence.

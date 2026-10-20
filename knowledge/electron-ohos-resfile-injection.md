---
{
  "schema_version": 1,
  "id": "electron-ohos-resfile-injection",
  "title": "Inject an Electron build into the HAP resource layer",
  "topic_key": "build.electron-ohos-resfile-injection",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Treat the desktop build output and the HAP resfile tree as two materialized copies: declare a copy rule from the packaged app's Resources/app and extra resources, rebuild the frontend first, and verify the packaged content.",
  "when_to_use": "An Electron app's HAP builds but shows a stale page or blank window, the desktop bundle is not inside the HAP, or changing the frontend does not change the device build.",
  "keywords": [
    "Electron",
    "HAP",
    "resfile",
    "app.asar",
    "extraResources",
    "resource injection",
    "frontend build",
    "packaging",
    "stale bundle",
    "electron-builder resources"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH 2in1/tablet Electron HAPs whose app code is packaged under resfile. The source layout assumes an electron-builder-style packaged app directory.",
  "related": [
    "electron-ohos-builder-packaging",
    "electron-ohos-runtime-and-jit",
    "electron-ohos-platform-capability-gates"
  ]
}
---

# Inject an Electron build into the HAP resource layer

## When to use

An Electron app's HAP builds but shows a stale page or blank window, the desktop bundle is not inside the HAP, or changing the frontend does not change the device build.

## Recommended approach

Treat the desktop build output and the HAP resfile tree as two materialized copies. Define a declarative copy rule from the packaged app's `Resources/app` (and extra resources) into `resfile/resources/app`, rebuild the frontend before injecting, and verify the packaged app content rather than trusting the build.

## Implementation steps

1. Build the desktop app bundle first; the packaged `Resources/app` directory is the source of truth for the HAP's `resfile/resources/app`.
2. Add a declarative injection rule that maps the packaged app directory and its extra resources into the HAP's `resfile` tree, with filters for docs and readme files.
3. Add a separate injection mode for local development that copies the `public` build into the app's `public` subtree, and make the script order explicit.
4. Rebuild the frontend before injecting when frontend code changed; an injection script that only builds the Electron main process will ship the previous frontend.
5. Keep the engine native libraries and the app resources separate: engine `.so` files go under the HAP's `libs/<abi>`, app code under `resfile`; injecting into `resfile` must not overwrite the engine.
6. Put app icons in `AppScope/resources/...`, not in the injected resfile tree; the next injection would overwrite them.
7. Verify the HAP content: the packaged app directory exists, contains the expected main entry and assets, and its timestamps or hashes match the fresh build.

## Conditions and alternatives

The copy rule assumes the desktop builder produces a known `Resources/app` layout. A different packager needs an equivalent source path. If the app can load its bundle from an archive, injecting a single archive is simpler, but the HAP resource layout must still be verified.

## Common mistakes

- Running only the inject script after a frontend change, so the HAP keeps the old page.
- Copying `public` over the packaged app instead of merging the intended subtree.
- Placing icons or engine libraries in `resfile` and losing them on the next injection.
- Assuming a successful HAP build means the resources are current.
- Injecting a desktop build for the wrong architecture or format.

## Check it works

Change a visible frontend string, rebuild the frontend, inject, rebuild the HAP, install, and confirm the new string appears. Inspect the HAP for the app directory, its main entry and assets, and confirm the engine libraries and icons are still present.

---
{
  "schema_version": 1,
  "id": "electron-ohos-resfile-flat-node-modules",
  "title": "Ship a symlink-free node_modules in the OH Electron resfile payload",
  "topic_key": "packaging.electron-ohos-resfile-flat-node-modules",
  "area": "packaging",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Use a flat npm install or dereference pnpm links in resfile/resources/app, then check that no symlinks remain.",
  "when_to_use": "The build passes, but require fails on the device with ENOENT, EACCES or Cannot find module for third-party packages in a pnpm workspace app.",
  "applicability": "Electron OH with loose resfile node_modules installed by pnpm. Reported by a knowledge collection for SDK v37.2.1 on an anonymized app; its evidence cites an unsigned HAP alongside a device-load claim, so treat the runtime cause as reported, not established.",
  "keywords": [
    "pnpm",
    "symlink",
    "node_modules",
    "resfile",
    "Cannot find module",
    "npm install --omit=dev",
    "cp -rL",
    "workspace:",
    "EACCES"
  ],
  "related": [
    "electron-ohos-resfile-injection",
    "electron-ohos-platform-binding-loader-shim",
    "electron-ohos-asar-store-precheck"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run"
}
---

# Ship a symlink-free node_modules in the OH Electron resfile payload

## When to use

The build passes, but require fails on the device with ENOENT, EACCES or Cannot find module for third-party packages in a pnpm workspace app.

## Recommended approach

Use a flat npm install or dereference pnpm links in resfile/resources/app, then check that no symlinks remain.

## Implementation steps

1. Inspect the staged payload with find node_modules -maxdepth 1 -type l and search deeper for links.
2. Remove or inline workspace:* dependencies from the staged package.json.
3. Run npm install --omit=dev --ignore-scripts in the staged app directory, or copy with cp -rL to dereference.
4. Add native addons built for OH separately, as --ignore-scripts skips their builds.
5. Fail the build if any symlink remains in the payload.

## Conditions and alternatives

An asar made from a dereferenced tree is another way to package it. Check for size growth from dereferencing, and for duplicate instances of packages that expect a single copy.

## Common mistakes

Verifying only on the build host, where links still resolve. Forgetting that deploy tools can leave workspace references.

## Check it works

Unpack the HAP and count symlinks, then run on a device, exercising lazy requires as well as startup ones.

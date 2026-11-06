---
{
  "schema_version": 1,
  "id": "electron-ohos-exec-path-metadata",
  "title": "Correct a stale electron_exec_path_ohos value in an older OH template",
  "topic_key": "packaging.electron-ohos-exec-path-metadata",
  "area": "packaging",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "If an older OH host template hard-codes electron_exec_path_ohos, set it to the installed resfile path for your bundle, or move to a template that omits it.",
  "when_to_use": "The app starts with a white screen and CDP reports about:blank, and module.json5 has metadata naming /data/app/electron.org/electron_1.0/bin/electron/electron.",
  "applicability": "Older ohos_hap host templates. Documented in a skill collection only, and a newer reviewed template omits this metadata. Inspect your template first; it is one of several causes of a white screen.",
  "keywords": [
    "electron_exec_path_ohos",
    "about:blank",
    "white screen",
    "module.json5 metadata",
    "resfile",
    "ohos_hap template"
  ],
  "related": [
    "electron-ohos-resfile-injection",
    "electron-ohos-runtime-and-jit",
    "electron-ohos-builder-packaging"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run"
}
---

# Correct a stale electron_exec_path_ohos value in an older OH template

## When to use

The app starts with a white screen and CDP reports about:blank, and module.json5 has metadata naming /data/app/electron.org/electron_1.0/bin/electron/electron.

## Recommended approach

If an older OH host template hard-codes electron_exec_path_ohos, set it to the installed resfile path for your bundle, or move to a template that omits it.

## Implementation steps

1. Check whether module.json5 has the electron_exec_path_ohos metadata at all. If it does not, this does not apply.
2. Find the real location with hdc shell find on a device with the app installed.
3. Set the value to the matching path, documented as /data/app/el1/bundle/public/<bundleName>/electron/resources/resfile/electron.
4. Prefer moving to a current host template if you can.

## Conditions and alternatives

Rule out a missing payload and boot failure first; other entries cover those. The path layout is documentation-reported and may vary by template.

## Common mistakes

Copying a template's path with another bundle name in it. Assuming all about:blank windows have this cause.

## Check it works

Install, check the resolved path on the device, and confirm the page loads after a cold start.

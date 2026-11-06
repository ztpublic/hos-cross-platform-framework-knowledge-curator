---
{
  "schema_version": 1,
  "id": "electron-ohos-execpath-appspawn-fork",
  "title": "Check child_process.fork on OH Electron, where execPath can be appspawn",
  "topic_key": "process.electron-ohos-execpath-appspawn-fork",
  "area": "process",
  "scope": "framework-specific",
  "frameworks": [
    "electron"
  ],
  "summary": "Test fork on your host before relying on it, since one reviewed host reports process.execPath as /system/bin/appspawn and fork fails with EACCES. No verified fix is known.",
  "when_to_use": "child_process.fork fails with spawn /system/bin/appspawn EACCES, breaking process pools, plugin hosts or cluster workers.",
  "applicability": "Observed on one Electron 37.2.0 OH port. Another source reports fork working on Electron 34 with ALLOW_WRITABLE_CODE_MEMORY, so this is host-scoped and the two reports are not reconciled. That permission concerns JIT memory, whereas this failure occurs at spawn.",
  "keywords": [
    "child_process.fork",
    "execPath",
    "appspawn",
    "EACCES",
    "cluster",
    "plugin host",
    "no-cluster",
    "utilityProcess"
  ],
  "related": [
    "electron-ohos-process-isolation",
    "hap-executable-declaration",
    "jit-executable-memory-authorization"
  ],
  "status": "unresolved",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Check child_process.fork on OH Electron, where execPath can be appspawn

## When to use

child_process.fork fails with spawn /system/bin/appspawn EACCES, breaking process pools, plugin hosts or cluster workers.

## Recommended approach

Test fork on your host before relying on it, since one reviewed host reports process.execPath as /system/bin/appspawn and fork fails with EACCES. No verified fix is known.

## Implementation steps

1. Log process.execPath at startup and run a small fork probe on the target device and runtime.
2. If fork fails, run the workload in-process as a stopgap, such as a --no-cluster mode, accepting the loss of isolation.
3. Evaluate alternatives on your runtime: a different process API, or a signed executable passed as execPath. The last is an untested curator suggestion, not an observed fix.
4. Report which features degrade when a child process host is unavailable, rather than failing silently.

## Conditions and alternatives

Unresolved: the reviewed port did not fix this and left its plugin host unadapted. Do not treat the permission from the other report as a fix without testing it.

## Common mistakes

Inferring that fork works from a run on another runtime. Assuming the JIT permission addresses a spawn EACCES.

## Check it works

Run the probe on each target host and record execPath, the runtime version, permissions declared and the result.

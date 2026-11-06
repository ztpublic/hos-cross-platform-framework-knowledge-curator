---
{
  "schema_version": 1,
  "id": "ohos-embedded-interpreter-in-process",
  "title": "Embed a language runtime in-process when OH will not exec it",
  "topic_key": "process.ohos-embedded-interpreter-in-process",
  "area": "process",
  "scope": "common",
  "frameworks": [],
  "summary": "If an interpreter executable cannot be launched, load its shared library and drive its embedding API on one dedicated large-stack thread.",
  "when_to_use": "A bundled language console cannot start because the interpreter binary copied from the HAP or app data is denied execution.",
  "applicability": "Interpreters offering an embedding API, reviewed with R in a Qt Widgets port on OH. Packages that run helper executables stay limited, as source comments note noexec app data. Replaces process isolation with in-process risk.",
  "keywords": [
    "embedded interpreter",
    "Rf_initialize_R",
    "setup_Rmainloop",
    "R_ToplevelExec",
    "ptr_R_WriteConsole",
    "noexec",
    "dlopen RTLD_GLOBAL",
    "setStackSize",
    "execve denied"
  ],
  "related": [
    "hap-executable-declaration",
    "native-ohos-worker-stack-budget",
    "ohos-exec-helper-permission-section"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Embed a language runtime in-process when OH will not exec it

## When to use

A bundled language console cannot start because the interpreter binary copied from the HAP or app data is denied execution.

## Recommended approach

If an interpreter executable cannot be launched, load its shared library and drive its embedding API on one dedicated large-stack thread.

## Implementation steps

1. Set the runtime's environment, R_HOME and related, before loading anything. Reduce default packages where startup needs it, as with R_DEFAULT_PACKAGES=NULL.
2. Load with dlopen and RTLD_GLOBAL so dependent modules resolve symbols, and find entry points with dlsym.
3. Start one dedicated thread with a large stack (16 MiB in the reviewed code, via QThread::setStackSize), because interpreter stack limits are tied to the thread.
4. Run initialization and every later evaluation on that thread, and wrap each evaluation in the runtime's top-level guard, R_ToplevelExec for R.
5. Install output callbacks after initialization, since it overwrites them, and forward to the UI by message.

## Conditions and alternatives

A signed helper executable, as covered by the executable entry, keeps isolation where it is permitted. A crash here takes down the whole app, and native package loading may still be limited.

## Common mistakes

Evaluating from the UI thread. Installing callbacks before init. Letting an interpreter error unwind through C++ frames without the guard.

## Check it works

Run an error, deep recursion and a long computation with the UI responsive, then a package that loads a native library, and stop a running evaluation.

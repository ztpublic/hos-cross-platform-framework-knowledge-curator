---
{
  "schema_version": 1,
  "id": "qt-pyqt-ohos-embedded-cpython-launcher",
  "title": "Start a PyQt app on OH from an embedded CPython launcher",
  "topic_key": "host.qt-pyqt-ohos-embedded-cpython-launcher",
  "area": "host",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Export main from a launcher library that the QPA loads, set the Python environment from inside the sandbox, embed CPython and run the app script, returning from main instead of exiting.",
  "when_to_use": "A PyQt app has no entry point on OH, dies silently at startup, or logs a crash when it exits normally.",
  "applicability": "PyQt5 on CPython 3.12 with a Qt 5 OH QPA, in two reviewed ports. Runtime libraries are not committed in those repositories, so source and build inputs were reviewed but the assembled runtime was not. The two ports disagree on whether to link Qt into the launcher.",
  "keywords": [
    "PyQt",
    "CPython",
    "embedded Python",
    "dlsym main",
    "Py_InitializeEx",
    "PYTHONHOME",
    "PYTHONPATH",
    "faulthandler",
    "sys.excepthook",
    "SystemExit",
    "find_sym2"
  ],
  "related": [
    "qt-ohos-shared-entrypoint",
    "qt-pyqt-ohos-subwindow-adoption",
    "python-ohos-extension-suffix-match"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Start a PyQt app on OH from an embedded CPython launcher

## When to use

A PyQt app has no entry point on OH, dies silently at startup, or logs a crash when it exits normally.

## Recommended approach

Export main from a launcher library that the QPA loads, set the Python environment from inside the sandbox, embed CPython and run the app script, returning from main instead of exiting.

## Implementation steps

1. Build a shared library exporting main with default visibility, and name it as the QPA's application library so dlsym("main") finds it.
2. Before initializing, set PYTHONHOME and PYTHONPATH covering stdlib, lib-dynload and site-packages, deriving the location with dladdr rather than a fixed path. Point HOME, XDG_* and TMPDIR at writable sandbox directories.
3. Call Py_SetProgramName and Py_InitializeEx(0), which returns failure instead of exiting, then run the script from resfile.
4. Redirect fds 1 and 2 to a sandbox log and enable faulthandler. Install sys.excepthook, because PyQt5 can abort on an exception escaping a slot.
5. Handle SystemExit in the launch script and return from main.
6. Decide about linking Qt into the launcher by testing: one port avoids it because of a reported musl find_sym2 conflict when sip.abi3.so is opened, and the other links Qt to get its packaging closure.

## Conditions and alternatives

This hosts a Python app without porting it to ArkTS, at the cost of bundling a runtime. See the extension-suffix entry for import failures.

## Common mistakes

Setting only PYTHONHOME. Calling sys.exit under PyRun. Losing stderr with no redirected log. Hard-coding the module path.

## Check it works

Launch cold on a clean install, raise an exception in a slot, exit normally and by close, and review the log for each.

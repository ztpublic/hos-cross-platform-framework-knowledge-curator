---
{
  "schema_version": 1,
  "id": "python-ohos-extension-suffix-match",
  "title": "Match Python extension module names to the embedded OH runtime",
  "topic_key": "packaging.python-ohos-extension-suffix-match",
  "area": "packaging",
  "scope": "common",
  "frameworks": [],
  "summary": "Check importlib.machinery.EXTENSION_SUFFIXES in the embedded runtime, then rename or rebuild modules whose platform suffix does not match.",
  "when_to_use": "from PyQt5 import sip raises ModuleNotFoundError although a sip.cpython-312-aarch64-linux-ohos.so file is present.",
  "applicability": "Embedded CPython on OH with wheels from another build. Reported by a port's build summary; the packaged site-packages was not in the repository. Stable-ABI .abi3.so modules are reported as unaffected.",
  "keywords": [
    "ModuleNotFoundError",
    "EXT_SUFFIX",
    "EXTENSION_SUFFIXES",
    "cpython-312",
    "abi3",
    "sip",
    "wheel",
    "extension module"
  ],
  "related": [
    "qt-pyqt-ohos-embedded-cpython-launcher",
    "ffi-library-and-symbol-availability",
    "native-dependency-closure"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run"
}
---

# Match Python extension module names to the embedded OH runtime

## When to use

from PyQt5 import sip raises ModuleNotFoundError although a sip.cpython-312-aarch64-linux-ohos.so file is present.

## Recommended approach

Check importlib.machinery.EXTENSION_SUFFIXES in the embedded runtime, then rename or rebuild modules whose platform suffix does not match.

## Implementation steps

1. Print importlib.machinery.EXTENSION_SUFFIXES inside the target runtime, not on the build host.
2. List packaged .so module names and compare them with those suffixes.
3. Prefer rebuilding against the embedded runtime, so the ABI matches as well as the name.
4. Where the ABI is known to match, rename in packaging and record why.
5. Add a build-time check that imports each native module, or at least matches the suffix list.

## Conditions and alternatives

A rename fixes discovery only. It does not make a module built for a different ABI safe.

## Common mistakes

Debugging the import path when the file name is the cause. Renaming modules built for an incompatible interpreter.

## Check it works

Import each extension on a device from the embedded interpreter and exercise a function from it.

---
{
  "schema_version": 1,
  "id": "hnp-tool-private-prefix-environment",
  "title": "Point a relocated tool at its private HNP install prefix",
  "topic_key": "process.hnp-tool-private-prefix-environment",
  "area": "process",
  "scope": "common",
  "frameworks": [],
  "summary": "Tools built with one install prefix cannot find helpers, configuration or CA files after private HNP installation, so set their locating variables in every process before the first spawn.",
  "when_to_use": "A prebuilt tool in a private HNP starts but misses its helpers, configuration or CA file, for example git failing on sub-commands or TLS verification.",
  "applicability": "OH apps that package command-line tools as private HNPs, installed under a /data/app/<name>.org/<name>_<version> path per the reviewed code. Git variables are the worked example; the approach applies to any relocatable tool with environment overrides.",
  "keywords": [
    "HNP",
    "private install",
    "GIT_EXEC_PATH",
    "LOCAL_GIT_DIRECTORY",
    "GIT_CONFIG_NOSYSTEM",
    "GIT_CONFIG_GLOBAL",
    "GIT_SSL_CAINFO",
    "prefix",
    "hnp.json"
  ],
  "related": [
    "electron-ohos-private-hnp",
    "native-curl-explicit-ca-bundle",
    "hap-executable-declaration"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Point a relocated tool at its private HNP install prefix

## When to use

A prebuilt tool in a private HNP starts but misses its helpers, configuration or CA file, for example git failing on sub-commands or TLS verification.

## Recommended approach

Tools built with one install prefix cannot find helpers, configuration or CA files after private HNP installation, so set their locating variables in every process before the first spawn.

## Implementation steps

1. Find the actual install directory on a device and compare it with the prefix compiled into the tool.
2. Derive paths from one constant kept in sync with hnp.json name and version, or read them at build time.
3. Set the tool's overrides. For git: LOCAL_GIT_DIRECTORY, GIT_EXEC_PATH, GIT_CONFIG_NOSYSTEM=1 and GIT_CONFIG_GLOBAL pointing at an existing file you own.
4. Set the CA path (GIT_SSL_CAINFO) from a bundled PEM, and prepend the HNP bin directories to PATH.
5. Apply this in each process that spawns tools: renderers do not inherit changes made in main after they start.

## Conditions and alternatives

Building the tool with a matching or relative prefix removes most of this, if the toolchain permits it. The compiled-in global config location in the reviewed git build was unreadable, which is why the explicit global file is needed.

## Common mistakes

Letting version strings drift between hnp.json and code. Setting the environment in main only. Pointing GIT_CONFIG_GLOBAL at a file that does not exist.

## Check it works

From each process, run version, a sub-command that needs helpers, a config read and an HTTPS clone. Repeat after a version change.

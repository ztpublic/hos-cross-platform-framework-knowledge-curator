---
{
  "schema_version": 1,
  "id": "hap-minimum-api-compatibility",
  "title": "Resolve device API incompatibility without falsifying the minimum",
  "topic_key": "build.hap-minimum-api-compatibility",
  "area": "build",
  "scope": "common",
  "frameworks": [],
  "summary": "Compare the device API with the real minimum required by the application and its native dependencies before changing the manifest.",
  "when_to_use": "HAP installation reports an older device SDK than the required compatibleSdkVersion.",
  "keywords": [
    "9568297",
    "compatibleSdkVersion",
    "API",
    "安装失败"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Use the exact selected SDK schema; deviceTypes such as 2in1 and an installable minimum do not prove PC feature compatibility.",
  "related": []
}
---

# Resolve device API incompatibility without falsifying the minimum

## When to use

HAP installation reports an older device SDK than the required compatibleSdkVersion.

## Recommended approach

Compare the device API with the real minimum required by the application and its native dependencies before changing the manifest.

## Implementation steps

1. Record device product/API and the selected compile, target and compatible SDK settings.
2. Determine the minimum API used by startup and optional native operations, including adapter code.
3. Lower a declared minimum only when older targets have a supported implementation or gated alternative; otherwise retain the requirement or use a suitable device.
4. Rebuild and sign the selected product, then test installation and startup on the lowest supported target.

## Conditions and alternatives

Use the exact selected SDK schema; deviceTypes such as 2in1 and an installable minimum do not prove PC feature compatibility.

## Common mistakes

Copying a lower compatibleSdkVersion can make installation succeed while startup still calls missing APIs.

## Check it works

Cover minimum-supported and older-rejected devices, plus capability-gated optional calls.

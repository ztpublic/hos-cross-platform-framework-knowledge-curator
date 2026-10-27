---
{
  "schema_version": 1,
  "id": "kmp-ohos-prepared-compose-resources",
  "title": "Package generated Compose resource values for OH",
  "topic_key": "storage.kmp-ohos-prepared-compose-resources",
  "area": "storage",
  "scope": "framework-specific",
  "frameworks": [
    "kotlin-multiplatform"
  ],
  "summary": "Publish the prepared Compose resources produced by the matching build into the namespace expected by the OH resource reader.",
  "when_to_use": "A Compose OH application links and starts but throws Failed to open raw file when reading a generated string or other Compose resource.",
  "applicability": "CPF Compose 1.9.2-1.0.0 resource adapter with the reviewed generated .cvr layout. Source evidence originated from a phone-targeted host; PC packaging and resource behavior remain unverified.",
  "keywords": [
    "preparedResources",
    ".cvr",
    "Failed to open raw file",
    "prepareComposeResourcesTaskForCommonMain",
    "composeResources",
    "rawfile"
  ],
  "related": [
    "kmp-ohos-rawfile-resources",
    "kmp-ohos-dependency-and-renderer"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Package generated Compose resource values for OH

## When to use

A Compose OH application links and starts but throws Failed to open raw file when reading a generated string or other Compose resource.

## Recommended approach

Publish the prepared Compose resources produced by the matching build into the namespace expected by the OH resource reader.

## Implementation steps

1. Inspect the selected Compose adapter's generated resource path and namespace rather than copying raw XML on the assumption it is the runtime format.
2. Make the host-publishing Copy task depend on resource preparation and the matching Kotlin shared-library link.
3. For this layout, copy build/generated/compose/resourceGenerator/preparedResources/commonMain/composeResources into entry/src/main/resources/rawfile/composeResources/<generated-resource-package>.
4. Copy libkn.so and its generated header from the same variant/ABI alongside those resources. Avoid mixing old generated namespaces with new Kotlin code.
5. Inspect the built HAP inventory and test a localized generated string after a clean build; stale copied resources can hide missing task dependencies.

## Conditions and alternatives

Paths and task names are version-specific. ResourceManager-based raw image reads and Compose generated-value lookup have different contracts. A simulator or phone result does not establish PC resource lookup or DPI behavior.

## Common mistakes

Do not copy only src/commonMain/composeResources XML when the adapter expects generated .cvr values, omit resource preparation from the task graph, or assume successful native linking proves resources are packaged.

## Check it works

Use clean and incremental builds, change a localized value, and verify the HAP contains the updated generated resource. Exercise first launch and missing-resource error handling on the intended PC.

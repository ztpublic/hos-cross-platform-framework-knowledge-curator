---
{
  "schema_version": 1,
  "id": "native-surface-owner",
  "title": "Give native surfaces one lifetime owner",
  "topic_key": "window.native-surface-owner",
  "area": "window",
  "scope": "common",
  "frameworks": [],
  "summary": "Choose a coherent surface-management API and stop renderer use before its owning node or surface is destroyed.",
  "when_to_use": "A surface handle becomes invalid during resize/recreation, or mixing surface APIs causes creation failure.",
  "keywords": [
    "SurfaceHolder",
    "XComponent",
    "surface",
    "opaque"
  ],
  "status": "unresolved",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to native rendering adapters. The reported mixed-API failure does not establish a universal mutual-exclusion boundary; precise coexistence remains unresolved.",
  "related": []
}
---

# Give native surfaces one lifetime owner

## When to use

A surface handle becomes invalid during resize/recreation, or mixing surface APIs causes creation failure.

## Recommended approach

Choose a coherent surface-management API and stop renderer use before its owning node or surface is destroyed.

## Implementation steps

1. Map node mount, surface creation/change/destruction and renderer submission states explicitly.
2. Retain handles only for their valid generation and reject callbacks from older surfaces.
3. Before switching API families, inventory input, focus and accessibility dependencies as well as rendering.
4. Use a minimal native reproduction to establish compatibility between OH_NativeXComponent and Node API SurfaceHolder routes.

## Conditions and alternatives

Applies to native rendering adapters. The reported mixed-API failure does not establish a universal mutual-exclusion boundary; precise coexistence remains unresolved.

## Common mistakes

Do not register two unknown surface owners or remove input/accessibility dependencies merely to make surface creation succeed.

## Check it works

Cover mount/unmount, resize, hide/show, recreation, mouse/touch/key input and accessibility. Treat the coexistence hypothesis as unverified until measured.

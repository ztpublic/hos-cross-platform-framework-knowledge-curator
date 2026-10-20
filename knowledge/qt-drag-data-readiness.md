---
{
  "schema_version": 1,
  "id": "qt-drag-data-readiness",
  "title": "Read dragged content at the supported drop stage",
  "topic_key": "input.qt-drag-data-readiness",
  "area": "input",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Use early drag events to negotiate accepted types and defer content consumption to the supported drop completion.",
  "when_to_use": "A Qt OH drop target can see MIME types but reads empty data during drag enter or movement.",
  "keywords": [
    "dropEvent",
    "dragEnterEvent",
    "MIME",
    "拖放"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to the selected Qt OH asynchronous drag backend; it is not a universal timing rule for every platform plugin.",
  "related": [
    "document-picker-uri-access"
  ]
}
---

# Read dragged content at the supported drop stage

## When to use

A Qt OH drop target can see MIME types but reads empty data during drag enter or movement.

## Recommended approach

Use early drag events to negotiate accepted types and defer content consumption to the supported drop completion.

## Implementation steps

1. Inspect available MIME descriptors during enter/move and choose an accepted action.
2. Read actual content during drop where the backend guarantees readiness.
3. Apply URI/access handling when the drop contains files rather than copying path text blindly.
4. Reject unsupported content explicitly and preserve cancellation.

## Conditions and alternatives

Applies to the selected Qt OH asynchronous drag backend; it is not a universal timing rule for every platform plugin.

## Common mistakes

Early MIME visibility is not proof that native payload transfer has completed.

## Check it works

Test text/files, cancel, unsupported types and delayed transfer.

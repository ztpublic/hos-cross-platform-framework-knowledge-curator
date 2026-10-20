---
{
  "schema_version": 1,
  "id": "calendar-file-handoff-completion",
  "title": "Calendar import through a receiver has a user-owned completion step",
  "topic_key": "integration.calendar-file-handoff-completion",
  "area": "integration",
  "scope": "common",
  "frameworks": [],
  "summary": "Keep the file and read grant alive; report receiver opening separately from the user completing calendar import.",
  "when_to_use": "Opening a calendar file is incorrectly reported as completed event insertion.",
  "keywords": [
    "Want",
    "viewData",
    "calendar",
    "ICS",
    "M3U8",
    "external player",
    "日历",
    "URI grant",
    "external app cannot read file",
    "ICS import",
    "share private file",
    "外部应用打开"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "A remote media URL may need a playlist wrapper for a particular receiver, but that does not automatically transmit authentication headers. Direct calendar APIs are an alternative when the app needs structured access rather than user-mediated import.",
  "related": [
    "external-handoff-file-uri"
  ]
}
---

# Calendar import through a receiver has a user-owned completion step

## When to use

Opening a calendar file is incorrectly reported as completed event insertion.

## Recommended approach

Keep the file and read grant alive; report receiver opening separately from the user completing calendar import.

## Implementation steps

For an `.ics` handoff, finish writing the temporary content, derive the supported file URI and send the view action with `text/calendar` plus the required read grant. Keep the file/grant alive for the receiving operation. A successful `startAbility` establishes that the receiver was opened; report an opened/handed-off result and let the user confirm import there. Do not label it as a measured number of inserted calendar events. If direct insertion is required, it needs a separate supported calendar API, permission and completion contract.

## Conditions and alternatives

A remote media URL may need a playlist wrapper for a particular receiver, but that does not automatically transmit authentication headers. Direct calendar APIs are an alternative when the app needs structured access rather than user-mediated import.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Test valid/invalid ICS content, missing receiver, denied grant, cancellation and delayed receiver reading. Confirm UI reports handoff rather than an unmeasured inserted-event count.

---
{
  "schema_version": 1,
  "id": "external-handoff-file-uri",
  "topic_key": "integration.external-handoff-file-uri",
  "scope": "common",
  "frameworks": [],
  "title": "Open or share a generated file with another OH application",
  "area": "integration",
  "summary": "Prepare a receiver-compatible file, expose it through the appropriate URI/access mechanism, and distinguish handoff acceptance from completed consumption.",
  "when_to_use": "Another app cannot read a private export, a calendar import fails, or an external player rejects the value passed through a channel.",
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
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": [
    "calendar-file-handoff-completion",
    "external-deep-action-contract"
  ]
}
---

# Open or share a generated file with another OH application

## When to use

Another app cannot read a private export, a calendar import fails, or an external player rejects the value passed through a channel.

## Recommended approach

Prepare a receiver-compatible file, expose it through the appropriate URI/access mechanism, and distinguish handoff acceptance from completed consumption.

## Implementation steps

1. Identify the receiving action and content type: document view, calendar import, media playback or sharing.
2. Generate the complete file before launching the receiver. Preserve the filename extension/MIME contract it expects.
3. Convert the private location into a supported file URI and grant read access through the selected handoff API.
4. Use a receiver chooser or handle the absence of a matching application explicitly.
5. Retain the file long enough for the receiver to open it and define later cleanup. Report cancellation or launch failure separately from success.

## Conditions and alternatives

A remote media URL may need a playlist wrapper for a particular receiver, but that does not automatically transmit authentication headers. Direct calendar APIs are an alternative when the app needs structured access rather than user-mediated import.

See [Calendar import through a receiver has a user-owned completion step](calendar-file-handoff-completion.md) for this independent contract.

See [Deep actions require a receiver contract as well as a valid URI](external-deep-action-contract.md) for this independent contract.

## Common mistakes

A native channel reply can mean only that launch was attempted. Do not delete the backing file immediately or assume a private filesystem string is readable by another app.

Do not turn a “no matching ability” exception into successful chooser presentation based on its message text. Report the native failure unless a documented handoff contract supplies evidence that the chooser actually opened.

## Check it works

Test a present and absent receiver, URI grant denial, cancellation, content validity, delayed opening and cleanup timing.

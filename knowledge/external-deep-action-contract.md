---
{
  "schema_version": 1,
  "id": "external-deep-action-contract",
  "title": "Deep actions require a receiver contract as well as a valid URI",
  "topic_key": "integration.external-deep-action-contract",
  "area": "integration",
  "scope": "common",
  "frameworks": [],
  "summary": "Validate receiver routing and encoded parameters; distinguish accepted launch from completed navigation.",
  "when_to_use": "A valid deep-action URI opens no receiver or opens the wrong screen.",
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

# Deep actions require a receiver contract as well as a valid URI

## When to use

A valid deep-action URI opens no receiver or opens the wrong screen.

## Recommended approach

Validate receiver routing and encoded parameters; distinguish accepted launch from completed navigation.

## Implementation steps

If the handoff carries a deep action rather than file bytes, define the receiver's scheme, route, query encoding and supported values. Encode query components rather than concatenating raw selected text; iterate Unicode code points when the receiver accepts a single supplementary-plane character. An OH bridge can submit a `Want` with `ohos.want.action.viewData` and the URI, but the target application must actually implement that protocol on the target product.

Do not infer OH availability from an Android package identifier or Android visibility declaration. Distinguish unsupported/missing receiver, launch rejection and accepted handoff where the API permits it. Acceptance cannot establish that an older receiver navigated to the requested detail page; test both cold and warm receiver behavior. A boolean bridge that collapses every failure needs a more precise result before the UI can accurately claim the receiver is not installed.

## Conditions and alternatives

A remote media URL may need a playlist wrapper for a particular receiver, but that does not automatically transmit authentication headers. Direct calendar APIs are an alternative when the app needs structured access rather than user-mediated import.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Test cold/warm receiver, encoded special characters and supplementary Unicode, unsupported routes, absent receiver and launch rejection. Confirm accepted launch is distinct from destination navigation.

---
{
  "id": "external-handoff-file-uri",
  "title": "Open or share a generated file with another OH application",
  "topic_key": "integration.external-handoff-file-uri",
  "area": "integration",
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
  "summary": "Prepare a receiver-compatible file, expose it through the appropriate URI/access mechanism, and distinguish handoff acceptance from completed consumption.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "Another app cannot read a private export, a calendar import fails, or an external player rejects the value passed through a channel."
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

### Calendar import through a receiver has a user-owned completion step

For an `.ics` handoff, finish writing the temporary content, derive the supported file URI and send the view action with `text/calendar` plus the required read grant. Keep the file/grant alive for the receiving operation. A successful `startAbility` establishes that the receiver was opened; report an opened/handed-off result and let the user confirm import there. Do not label it as a measured number of inserted calendar events. If direct insertion is required, it needs a separate supported calendar API, permission and completion contract.

### Deep actions require a receiver contract as well as a valid URI

If the handoff carries a deep action rather than file bytes, define the receiver's scheme, route, query encoding and supported values. Encode query components rather than concatenating raw selected text; iterate Unicode code points when the receiver accepts a single supplementary-plane character. An OH bridge can submit a `Want` with `ohos.want.action.viewData` and the URI, but the target application must actually implement that protocol on the target product.

Do not infer OH availability from an Android package identifier or Android visibility declaration. Distinguish unsupported/missing receiver, launch rejection and accepted handoff where the API permits it. Acceptance cannot establish that an older receiver navigated to the requested detail page; test both cold and warm receiver behavior. A boolean bridge that collapses every failure needs a more precise result before the UI can accurately claim the receiver is not installed.

## Common mistakes

A native channel reply can mean only that launch was attempted. Do not delete the backing file immediately or assume a private filesystem string is readable by another app.

Do not turn a “no matching ability” exception into successful chooser presentation based on its message text. Report the native failure unless a documented handoff contract supplies evidence that the chooser actually opened.

## Check it works

Test a present and absent receiver, URI grant denial, cancellation, content validity, delayed opening and cleanup timing.

---
{
  "schema_version": 1,
  "id": "utf8-command-payload-transport",
  "title": "Preserve UTF-8 payload bytes across shell and HTTP boundaries",
  "topic_key": "tooling.utf8-command-payload-transport",
  "area": "tooling",
  "scope": "common",
  "frameworks": [],
  "summary": "Inspect encoded request bytes and pass a UTF-8 payload file or structured request body instead of relying on ambiguous shell argument encoding.",
  "when_to_use": "Non-ASCII text sent by a command-line HTTP client arrives garbled.",
  "keywords": [
    "UTF-8",
    "curl",
    "中文乱码",
    "encoding"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Common host-tooling issue that can occur during any framework adaptation; not a HarmonyOS runtime text-rendering defect.",
  "related": []
}
---

# Preserve UTF-8 payload bytes across shell and HTTP boundaries

## When to use

Non-ASCII text sent by a command-line HTTP client arrives garbled.

## Recommended approach

Inspect encoded request bytes and pass a UTF-8 payload file or structured request body instead of relying on ambiguous shell argument encoding.

## Implementation steps

1. Confirm the intended text and encoding before the shell boundary.
2. Write the payload with explicit UTF-8 and send its bytes using the client's file-body option or a structured API.
3. Use the appropriate content type/charset for the receiving protocol and inspect the actual request/response encoding.
4. Keep diagnostic logging free of credentials and avoid interpreting source text as shell code.

## Conditions and alternatives

Common host-tooling issue that can occur during any framework adaptation; not a HarmonyOS runtime text-rendering defect.

## Common mistakes

Changing application fonts or sprinkling quoting escapes does not repair altered request bytes.

## Check it works

Round-trip Chinese text, emoji, quotes and newlines and compare the received decoded text with the original.

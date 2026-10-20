---
{
  "schema_version": 1,
  "id": "bounded-upload-staging",
  "title": "Scale uploads and previews without collecting the entire file",
  "topic_key": "networking.bounded-upload-staging",
  "area": "networking",
  "scope": "common",
  "frameworks": [],
  "summary": "Stream an owned staging file with backpressure and enforce separate transfer and preview limits.",
  "when_to_use": "Attachment uploads or previews exhaust memory or keep temporary files alive after cancellation.",
  "keywords": [
    "dartssh2",
    "SFTP",
    "resume",
    "checksum",
    "backpressure",
    "timeout",
    "cancellation",
    "upload resume",
    "eager upload",
    "remote prefix",
    "上传续传",
    "断点续传",
    "下载卡住",
    "校验"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "The transfer algorithm is shared application code, while user-mediated destination selection and background lifetime require an OH-capable implementation. A source application's Android-only save channel is not an OH backend. Provide the OH adapter using the [file-export contract](file-export-contract.md), and use a supported background task or resumable foreground operation as required.",
  "related": [
    "file-export-contract",
    "resumable-file-download-contract",
    "sandbox-import-as-access-fallback"
  ]
}
---

# Scale uploads and previews without collecting the entire file

## When to use

Attachment uploads or previews exhaust memory or keep temporary files alive after cancellation.

## Recommended approach

Stream an owned staging file with backpressure and enforce separate transfer and preview limits.

## Implementation steps

For a local attachment upload, carry an owned staging path and byte count rather than a whole-file channel payload. Stream the file into the SSH sink with backpressure, check the expected size before transfer and again as bytes are consumed, and report progress at a bounded cadence. A size check catches growth/truncation, not a same-length mutation; use a digest/version when content identity matters. Keep image-preview limits separate from the transfer limit so permitting a large file does not permit an equally large in-memory preview.

Define cleanup ownership for cancelled selection, removed attachments, failed uploads and successful consumption. A platform-specific picker that copies into a cache file is still a separate native adapter; an Android implementation does not establish an OH picker or export implementation. Use bounded reads on the chosen OH backend or an [explicit import copy](sandbox-import-as-access-fallback.md).

Budget deadlines by stage. Hashing a large remote file can take much longer than a single block read, so give verification its own cancellable deadline. Remove an inappropriate whole-transfer timeout only when cancellation, stalled-I/O detection and connection closure still settle the operation. A larger configured file limit is a product choice, not an OS maximum or proof that large transfers succeed.

## Conditions and alternatives

The transfer algorithm is shared application code, while user-mediated destination selection and background lifetime require an OH-capable implementation. A source application's Android-only save channel is not an OH backend. Provide the OH adapter using the [file-export contract](file-export-contract.md), and use a supported background task or resumable foreground operation as required.

A digest computed by a remote shell requires its documented command/tool availability and safe argument handling; prefer a protocol-supplied strong version/digest when available. Size alone cannot detect a same-length change. If restart persistence is needed, store a validated checkpoint and identity; an in-memory reconnect loop does not resume after process death.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Measure memory for a large attachment and preview; test growth/truncation, same-size mutation, cancelled selection, stalled transfer and cleanup after failure/success.

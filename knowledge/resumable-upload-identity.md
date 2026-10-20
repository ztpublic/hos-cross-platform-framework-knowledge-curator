---
{
  "schema_version": 1,
  "id": "resumable-upload-identity",
  "title": "Give resumable uploads a stable target and an independent owner",
  "topic_key": "networking.resumable-upload-identity",
  "area": "networking",
  "scope": "common",
  "frameworks": [],
  "summary": "Retain a transfer-owned remote target and immutable local content, validate identity on resume and separate navigation from transfer lifetime.",
  "when_to_use": "An upload retry creates another remote object or accepts an unrelated staged prefix.",
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
    "resumable-file-download-contract"
  ]
}
---

# Give resumable uploads a stable target and an independent owner

## When to use

An upload retry creates another remote object or accepts an unrelated staged prefix.

## Recommended approach

Retain a transfer-owned remote target and immutable local content, validate identity on resume and separate navigation from transfer lifetime.

## Implementation steps

Create one transfer-owned remote staging target and reuse it across retries. Reconnect with the same authenticated host/account policy, query the staged length, reject an offset beyond the local source, and stream the remaining range with acknowledged writes. Keep offset accounting separate from bytes merely queued locally. Verify completion before publishing the remote reference.

Remote length alone does not prove that the staged prefix matches the selected content. Retain an immutable local staging copy and a validated transfer identity; verify a prefix/content digest or restart when that identity cannot be established. A same-length remote replacement or local mutation must not be accepted as a completed transfer. Define remote staging cleanup and expiry explicitly: retaining partial bytes for resume replaces the old delete-on-disconnect policy.

Uploads started during attachment selection need an owner that exists before the final send action. Track transfer generation, initiating task and destination host; reject late progress/completion after removal, task switch or host change. Disable submission of pending/failed attachments or await their actual completion. Cancel the underlying I/O and wait for its use of the local staging file to finish before deleting that file. Retain retryable state only as long as the chosen lifecycle promises; an in-memory target does not survive process death.

Choose that owner's lifetime deliberately. For uploads that should survive navigation, keep drafts/transfers in an application-owned task store and let pages subscribe without cancelling on unmount. Discarding a draft or switching its destination still needs explicit cancellation and cleanup. Model offline attachments as waiting rather than successfully uploaded; resume them after connectivity returns using the same transfer identity. Preserve a failed submission's text and attachments without overwriting edits entered while it was in flight. Navigation persistence and process-restart persistence are separate promises.

## Conditions and alternatives

The transfer algorithm is shared application code, while user-mediated destination selection and background lifetime require an OH-capable implementation. A source application's Android-only save channel is not an OH backend. Provide the OH adapter using the [file-export contract](file-export-contract.md), and use a supported background task or resumable foreground operation as required.

A digest computed by a remote shell requires its documented command/tool availability and safe argument handling; prefer a protocol-supplied strong version/digest when available. Size alone cannot detect a same-length change. If restart persistence is needed, store a validated checkpoint and identity; an in-memory reconnect loop does not resume after process death.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Disconnect after acknowledged writes and retry to the same target. Reject mismatched prefixes and changed local content; test draft removal, navigation, host switch and late completion.

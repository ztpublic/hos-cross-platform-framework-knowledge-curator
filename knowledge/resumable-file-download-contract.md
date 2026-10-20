---
{
  "schema_version": 1,
  "id": "resumable-file-download-contract",
  "topic_key": "networking.resumable-file-download-contract",
  "scope": "common",
  "frameworks": [],
  "title": "Resume bounded file transfers without publishing incomplete data",
  "area": "networking",
  "summary": "Use bounded ordered I/O, verified content identity and owned cancellation/deadlines; publish only verified bytes through supported destination adapters.",
  "when_to_use": "A mobile SSH/SFTP transfer stalls or reconnects and needs safe bounded resume plus an accurate final save result.",
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
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": [
    "bounded-upload-staging",
    "file-export-contract",
    "resumable-upload-identity"
  ]
}
---

# Resume bounded file transfers without publishing incomplete data

## When to use

An SSH/SFTP download stalls on mobile network changes, restarts a large file from zero, or reports saved before destination writing finishes.

## Recommended approach

Stream bounded reads into a temporary file and resume only against verified remote identity. Own connection cancellation, stage-specific deadlines and the final OH save/export adapter separately.

## Implementation steps

1. Establish a dedicated transfer connection when the selected SSH package cannot reliably release a borrowed SFTP channel. Keep host-key verification and authentication policy on every reconnect.
2. Capture remote size and a trustworthy content digest/version before writing. Reject an unavailable identity or an excessive size rather than resuming by filename alone.
3. Pipeline a bounded number of offset reads and use one ordered writer. Advance the resume offset only after bytes have been written; verify each returned block length and leave the receive side able to process replies while disk writes run.
4. Give connect, channel-open, identify and read operations explicit deadlines. Race pending operations against connection closure and user cancellation. Dispose late-opened resources after a timed-out or cancelled attempt.
5. Retry only classified transient transport failures under a finite budget/backoff. Re-identify the remote object before resuming; changed identity requires a fresh transfer. Treat disk-write, authorization, host-key and checksum failures as their own outcomes rather than blind retries.
6. Verify final remote identity and local digest, flush/close the temporary output and only then call the destination adapter. Report transferring, verifying and saving as distinct stages; user cancellation is not a generic failure or success.
7. Await destination stream/descriptor completion before returning saved. Delete incomplete temporary data in finally, preserving the original error if cleanup also fails.

## Conditions and alternatives

The transfer algorithm is shared application code, while user-mediated destination selection and background lifetime require an OH-capable implementation. A source application's Android-only save channel is not an OH backend. Provide the OH adapter using the [file-export contract](file-export-contract.md), and use a supported background task or resumable foreground operation as required.

A digest computed by a remote shell requires its documented command/tool availability and safe argument handling; prefer a protocol-supplied strong version/digest when available. Size alone cannot detect a same-length change. If restart persistence is needed, store a validated checkpoint and identity; an in-memory reconnect loop does not resume after process death.

See [Scale uploads and previews without collecting the entire file](bounded-upload-staging.md) for this independent contract.

See [Give resumable uploads a stable target and an independent owner](resumable-upload-identity.md) for this independent contract.

## Common mistakes

Unbounded read buffering can deadlock or exhaust memory. A timeout on a Future does not cancel underlying I/O. Pausing receive processing while waiting for disk writes can prevent pending channel replies from arriving. Saving a partial file or retrying from an offset against changed contents silently corrupts the result.

## Check it works

Test mid-read disconnect, a pending read when SSH closes, channel setup timeout, cancellation, reconnect, a same-sized changed remote file, truncation, checksum mismatch, low storage and destination failure. Confirm memory remains bounded, control traffic remains responsive and no incomplete file is reported as saved.

For uploads, test disconnect after acknowledged writes, manual retry to the same target, a wrong or oversized remote prefix, stalled writes, source mutation and removal during transfer. Verify task/host changes cannot attach an old completion to a new destination and that both local and remote staging follow their retention policy.

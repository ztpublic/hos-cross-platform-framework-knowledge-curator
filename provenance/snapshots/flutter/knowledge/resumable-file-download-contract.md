---
{
  "schema_version": 2,
  "id": "resumable-file-download-contract",
  "title": "Resume bounded Dart file transfers without publishing incomplete data",
  "topic_key": "networking.resumable-file-download-contract",
  "area": "networking",
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
  "summary": "Use bounded ordered I/O, verified content identity and owned cancellation/deadlines; publish only verified bytes through supported destination adapters.",
  "when_to_use": "A mobile SSH/SFTP transfer stalls or reconnects and needs safe bounded resume plus an accurate final save result.",
  "status": "scoped"
}
---

# Resume bounded Dart file transfers without publishing incomplete data

## When to use

A Dart SSH/SFTP download stalls on mobile network changes, restarts a large file from zero, or reports saved before destination writing finishes.

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

The transfer algorithm is shared Dart, while user-mediated destination selection and background lifetime require an OH-capable implementation. A source application's Android-only save channel is not an OH backend. Provide the OH adapter using the [file-export contract](file-export-contract.md), and use a supported background task or resumable foreground operation as required.

A digest computed by a remote shell requires its documented command/tool availability and safe argument handling; prefer a protocol-supplied strong version/digest when available. Size alone cannot detect a same-length change. If restart persistence is needed, store a validated checkpoint and identity; an in-memory reconnect loop does not resume after process death.

### Scale uploads and previews without collecting the entire file

For a local attachment upload, carry an owned staging path and byte count rather than a whole-file channel payload. Stream the file into the SSH sink with backpressure, check the expected size before transfer and again as bytes are consumed, and report progress at a bounded cadence. A size check catches growth/truncation, not a same-length mutation; use a digest/version when content identity matters. Keep image-preview limits separate from the transfer limit so permitting a large file does not permit an equally large in-memory preview.

Define cleanup ownership for cancelled selection, removed attachments, failed uploads and successful consumption. A platform-specific picker that copies into a cache file is still a separate native adapter; an Android implementation does not establish an OH picker or export implementation. Use bounded reads on the chosen OH backend or an [explicit import copy](sandbox-import-as-access-fallback.md).

Budget deadlines by stage. Hashing a large remote file can take much longer than a single block read, so give verification its own cancellable deadline. Remove an inappropriate whole-transfer timeout only when cancellation, stalled-I/O detection and connection closure still settle the operation. A larger configured file limit is a product choice, not an OS maximum or proof that large transfers succeed.

### Give resumable uploads a stable target and an independent owner

Create one transfer-owned remote staging target and reuse it across retries. Reconnect with the same authenticated host/account policy, query the staged length, reject an offset beyond the local source, and stream the remaining range with acknowledged writes. Keep offset accounting separate from bytes merely queued locally. Verify completion before publishing the remote reference.

Remote length alone does not prove that the staged prefix matches the selected content. Retain an immutable local staging copy and a validated transfer identity; verify a prefix/content digest or restart when that identity cannot be established. A same-length remote replacement or local mutation must not be accepted as a completed transfer. Define remote staging cleanup and expiry explicitly: retaining partial bytes for resume replaces the old delete-on-disconnect policy.

Uploads started during attachment selection need an owner that exists before the final send action. Track transfer generation, initiating task and destination host; reject late progress/completion after removal, task switch or host change. Disable submission of pending/failed attachments or await their actual completion. Cancel the underlying I/O and wait for its use of the local staging file to finish before deleting that file. Retain retryable state only as long as the chosen lifecycle promises; an in-memory target does not survive process death.

Choose that owner's lifetime deliberately. For uploads that should survive navigation, keep drafts/transfers in an application-owned task store and let pages subscribe without cancelling on unmount. Discarding a draft or switching its destination still needs explicit cancellation and cleanup. Model offline attachments as waiting rather than successfully uploaded; resume them after connectivity returns using the same transfer identity. Preserve a failed submission's text and attachments without overwriting edits entered while it was in flight. Navigation persistence and process-restart persistence are separate promises.

## Common mistakes

Unbounded read buffering can deadlock or exhaust memory. A timeout on a Future does not cancel underlying I/O. Pausing receive processing while waiting for disk writes can prevent pending channel replies from arriving. Saving a partial file or retrying from an offset against changed contents silently corrupts the result.

## Check it works

Test mid-read disconnect, a pending read when SSH closes, channel setup timeout, cancellation, reconnect, a same-sized changed remote file, truncation, checksum mismatch, low storage and destination failure. Confirm memory remains bounded, control traffic remains responsive and no incomplete file is reported as saved.

For uploads, test disconnect after acknowledged writes, manual retry to the same target, a wrong or oversized remote prefix, stalled writes, source mutation and removal during transfer. Verify task/host changes cannot attach an old completion to a new destination and that both local and remote staging follow their retention policy.

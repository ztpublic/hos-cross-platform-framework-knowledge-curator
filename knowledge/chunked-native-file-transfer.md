---
{
  "schema_version": 1,
  "id": "chunked-native-file-transfer",
  "topic_key": "integration.chunked-native-file-transfer",
  "scope": "common",
  "frameworks": [],
  "title": "Transfer large bridge payloads through owned staging files",
  "area": "integration",
  "summary": "Send bounded acknowledged chunks to a session-owned staging file, validate ordering and completion, and publish only after a successful commit.",
  "when_to_use": "Large JavaScript/native payloads exceed message limits or an interrupted transfer leaves partial or incorrectly acknowledged files.",
  "keywords": [
    "ArkWeb",
    "JavaScript bridge",
    "chunk",
    "backpressure",
    "atomic rename",
    "short write",
    "Uint8Array",
    "大文件",
    "分块传输",
    "临时文件"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": [
    "file-export-contract",
    "record-protocol-byte-budgets",
    "typed-channel-payload-extraction"
  ]
}
---

# Transfer large bridge payloads through owned staging files

## When to use

A JavaScript/ArkWeb bridge rejects a large file message, stalls during serialization, or leaves a partially written destination after interruption.

## Recommended approach

Use bounded acknowledged chunks and an owned native staging file. Validate transfer identity, ordering and total length, then publish only a complete file; keep external export as a subsequent operation.

## Implementation steps

1. Choose a chunk size using the encoded message limit and memory budget. JSON arrays expand binary bytes; count encoded UTF-8 bytes rather than only JavaScript string length. Prefer a supported binary transport when available.
2. Define a protocol with a transfer token, destination identity, chunk index, declared total bytes and terminal final/abort operations. Bind the token to the current bridge session and validate finite integral indexes, lengths and byte values.
3. Reserve native transfer state before the first write. Resolve destinations under an authorized sandbox/export root, reject traversal and symlink escapes, and serialize transfers targeting the same destination.
4. Write into a unique temporary file beside the eventual destination. Check the actual write count and handle short writes; increment the committed offset only after the bytes are accepted. Acknowledge each chunk before sending the next or use a strictly bounded in-flight window.
5. Reject changed destinations/totals, missing or reordered chunks and overflow. On final, verify the complete length and any required digest, flush/close, and perform the supported atomic replacement before replying success.
6. Retain transfer ownership until replacement succeeds. A rename failure must still leave enough state to clean up the staging file. Abort, timeout, native-view destruction and process-restart recovery must remove abandoned temporary files without deleting a valid destination.
7. After commit, invoke the user-mediated export/save operation and report its selected/cancelled/error outcome separately. Use operation-appropriate deadlines; an outstanding picker is not a fast RPC.

## Conditions and alternatives

Chunking transport does not bound total memory if JavaScript first materializes the entire byte array. Stream the producer or enforce a file-size limit. Storage limits, destination replacement and durability guarantees depend on the selected filesystem/API; verify them independently. Transfer tokens are correlation identifiers, not authorization.

## Common mistakes

See [Bound record pages by encoded bytes and explicit progress](record-protocol-byte-budgets.md).

Incrementing by requested rather than written bytes can acknowledge a corrupt file. Removing pending state before rename loses cleanup ownership. Concurrent tokens can still overwrite the same path. Sending huge unbounded JSON payloads or treating an intermediate acknowledgment as export success defeats the protocol.

## Check it works

Test multiple chunks, zero bytes, a short final chunk, non-ASCII names, short writes, reordered/duplicate chunks, changed totals, rename failure, timeout, abort and native-view teardown. Confirm an old destination survives every pre-commit failure and the imported/exported bytes match exactly.

See [typed channel payloads](typed-channel-payload-extraction.md) and [file export](file-export-contract.md).

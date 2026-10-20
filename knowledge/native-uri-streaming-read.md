---
{
  "schema_version": 1,
  "id": "native-uri-streaming-read",
  "title": "Read large URI selections through bounded native handles",
  "topic_key": "storage.native-uri-streaming-read",
  "area": "storage",
  "scope": "common",
  "frameworks": [],
  "summary": "Return selection metadata first and read through owned bounded native descriptors with serialized read/close operations.",
  "when_to_use": "Large selected files allocate their entire content before the importer can stream them.",
  "keywords": [
    "file_selector",
    "XFile",
    "openFile",
    "readAsBytes",
    "openRead",
    "URI streaming",
    "readChunk",
    "descriptor",
    "file_picker",
    "CSV",
    "XLSX",
    "picker path missing",
    "selected file unreadable",
    "文件导入",
    "选择文件读不到",
    "空文件",
    "大文件内存"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Requires an OH native file adapter that supports authorized descriptors and bounded reads. This protocol is not a standard method set guaranteed by every picker plugin.",
  "related": [
    "persistent-file-access-grants",
    "picker-result-readable-content"
  ]
}
---

# Read large URI selections through bounded native handles

## When to use

Large selected files allocate their entire content before the importer can stream them.

## Recommended approach

Return selection metadata first and read through owned bounded native descriptors with serialized read/close operations.

## Implementation steps

When selection itself allocates the whole file, change the backend as well as the caller: return name, URI, MIME type and length metadata, then open an authorized native descriptor only when reading starts. A streaming adapter can expose a ranged read operation through an `open → readChunk → close` channel protocol. These operations are an adapter design, not standard methods supplied by every `file_selector` package.

Validate nonnegative ranges and integral chunk limits on both sides, bound native allocation before returning bytes, and request the next chunk only when the consumer is ready. An explicit cap such as one MiB is a tuning choice. Check actual bytes read, accept a valid empty file, and report premature EOF or an oversized chunk as failure. Treat the selected length as a snapshot: define a retry/failure policy if content changes rather than publishing a truncated import as complete.

Close the token in the caller's `finally` and on stream cancellation. Native ownership must also cover engine/ability detach: reject new reads, drain a read already in flight, then close all descriptors. Serialize operations sharing a descriptor so a close cannot race a read. Preserve the primary read failure if cleanup also fails. Reopening a URI later still depends on its access grant; see [persistent access](persistent-file-access-grants.md).

Verify that `openRead` performs bounded reads in the selected backend. Splitting a stream after an eager whole-file allocation does not bound memory. `readAsBytes` and `readAsString` still collect the entire content; keep a size limit or stream into a supported parser or staging file.

## Conditions and alternatives

Requires an OH native file adapter that supports authorized descriptors and bounded reads. This protocol is not a standard method set guaranteed by every picker plugin.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Measure memory during selection and ranged reads; test empty content, premature EOF, oversized chunks, changed content, invalid tokens, cancellation and detach during a pending read. Verify descriptors return to baseline.

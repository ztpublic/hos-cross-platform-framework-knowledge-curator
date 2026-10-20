---
{
  "schema_version": 2,
  "id": "picker-result-readable-content",
  "title": "Read selected files after changing the OH picker backend",
  "topic_key": "storage.picker-result-readable-content",
  "area": "storage",
  "keywords": ["file_selector", "XFile", "openFile", "readAsBytes", "openRead", "URI streaming", "readChunk", "descriptor", "file_picker", "CSV", "XLSX", "picker path missing", "selected file unreadable", "文件导入", "选择文件读不到", "空文件", "大文件内存"],
  "summary": "Adapt the importer to the selected-file object's content API; keep cancellation, empty content and read failure distinct instead of assuming every result is a local path.",
  "when_to_use": "A file picker returns a selection, but the importer cannot read it after switching plugins, or its previous path/bytes fields no longer exist.",
  "status": "scoped"
}
---

# Read selected files after changing the OH picker backend

## When to use

A file picker returns a selection, but the importer cannot read it after switching plugins, or its previous path/bytes fields no longer exist.

## Recommended approach

Adapt the importer to the selected-file object's content API; keep cancellation, empty content and read failure distinct instead of assuming every result is a local path.

## Implementation steps

1. Select an OH-capable backend and verify its exact selection and reading APIs. With a compatible `file_selector` API, describe accepted formats using `XTypeGroup`, await `openFile`, and handle its nullable `XFile` result. A null selection ends the operation without invoking the parser.
2. Pass the selected object to the importer. Use its `name` for format selection and its supported content API, such as `readAsBytes`, to obtain bytes. Treat its `path` as an ordinary filesystem location only when the backend guarantees that interpretation and access.
3. Keep an optional local-file streaming route for a verified readable file. If the plugin offers only an in-memory read, apply an appropriate size limit; a chunked read followed by collecting every chunk still uses memory proportional to the whole file.
4. Send the resulting bytes to the format-specific decoder: text decoding with an explicit encoding policy for CSV/TSV, a binary workbook parser for XLSX. Filename filters help selection but do not validate the content.
5. Return distinct cancelled, loaded and failed outcomes. Loaded content may have zero bytes; the parser can then report an empty or invalid document according to its own contract. Preserve a read exception as a read error instead of returning an empty string.
6. After asynchronous selection or reading, check the UI owner's lifetime before updating it. Close direct file handles in `finally` and reset busy/progress state on completion, cancellation and failure.

## Conditions and alternatives

This approach applies when the selected object supplies a usable content API on the chosen backend. It does not create file access permissions or make an inaccessible URI readable. If only a native authorized handle is available, add a narrow native read adapter. If a downstream library requires an ordinary path, use [an app-owned import copy](sandbox-import-as-access-fallback.md) instead.

The concrete API shape described here is the `file_selector` 1.0 API with the `cross_file` 0.3 API, where `name` and `path` are properties and `readAsBytes` is asynchronous. Check the selected OH fork and resolved versions; newer API generations can change those members.

Keep [plugin source and API selection](plugin-graph-source-and-api.md) separate from content decoding. `file_picker` and `file_selector` return different objects; changing the dependency alone does not adapt the importer.

### Read large URI selections through bounded native handles

When selection itself allocates the whole file, change the backend as well as the caller: return name, URI, MIME type and length metadata, then open an authorized native descriptor only when reading starts. A custom `XFile` adapter can implement `openRead(start, end)` through an `open → readChunk → close` channel protocol. These operations are an adapter design, not standard methods supplied by every `file_selector` package.

Validate nonnegative ranges and integral chunk limits on both sides, bound native allocation before returning bytes, and request the next chunk only when the consumer is ready. An explicit cap such as one MiB is a tuning choice. Check actual bytes read, accept a valid empty file, and report premature EOF or an oversized chunk as failure. Treat the selected length as a snapshot: define a retry/failure policy if content changes rather than publishing a truncated import as complete.

Close the token in Dart `finally` and on stream cancellation. Native ownership must also cover engine/ability detach: reject new reads, drain a read already in flight, then close all descriptors. Serialize operations sharing a descriptor so a close cannot race a read. Preserve the primary read failure if cleanup also fails. Reopening a URI later still depends on its access grant; see [persistent access](persistent-file-access-grants.md).

Verify that `openRead` performs bounded reads in the selected backend. Splitting a stream after an eager whole-file allocation does not bound memory. `readAsBytes` and `readAsString` still collect the entire content; keep a size limit or stream into a supported parser or staging file.

## Common mistakes

Do not stringify a selected object into a path or translate all read failures to empty content. Do not assume a valid extension establishes a valid workbook. Distinguish cancellation while selecting from cancellation during a long read, and avoid exposing partially decoded data as a completed import.

## Check it works

Select a local file and, where supported, content without a directly usable local path. Verify CSV encoding and XLSX parsing, empty and malformed content, cancelled selection, denied or expired access, large input, and leaving the screen during reading. A successful selection must either load the intended bytes or report a specific read failure.

For a streaming adapter, also test ranged reads, premature EOF, invalid tokens, cancellation and detach during a native read. Measure memory during selection and consumption, and confirm descriptor counts return to their baseline after success and failure.

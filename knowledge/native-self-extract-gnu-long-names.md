---
{
  "schema_version": 1,
  "id": "native-self-extract-gnu-long-names",
  "title": "Handle GNU tar long names when self-extracting a runtime payload",
  "topic_key": "packaging.native-self-extract-gnu-long-names",
  "area": "packaging",
  "scope": "common",
  "frameworks": [],
  "summary": "A first-run extractor for a payload written by GNU tar must apply typeflag L long-name entries to the following entry and skip K payloads, or it will truncate paths at 100 bytes.",
  "when_to_use": "First-run payload extraction aborts with EISDIR after creating a directory with a name cut at 100 characters, and the app then fails to find a deep dependency and shows a blank window.",
  "applicability": "Apps that ship a tar payload and unpack it with a small custom parser into the OH sandbox because no system tar is relied on. The tar format detail is general; it matters on OH because such extractors are written by hand for this purpose.",
  "keywords": [
    "tar",
    "GNU long name",
    "typeflag L",
    "././@LongLink",
    "EISDIR",
    "100 byte",
    "payload extraction",
    "first run",
    "node_modules"
  ],
  "related": [
    "native-dependency-closure",
    "startup-work-prerequisites",
    "sandbox-paths-and-app-identity"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Handle GNU tar long names when self-extracting a runtime payload

## When to use

First-run payload extraction aborts with EISDIR after creating a directory with a name cut at 100 characters, and the app then fails to find a deep dependency and shows a blank window.

## Recommended approach

A first-run extractor for a payload written by GNU tar must apply typeflag L long-name entries to the following entry and skip K payloads, or it will truncate paths at 100 bytes.

## Implementation steps

1. Read the 512-byte ustar header and keep the typeflag rather than classifying everything except files and directories as skippable.
2. For typeflag L, read its data as the real name of the next entry, removing the trailing NUL. Apply it after the same path normalization used for ordinary names.
3. For typeflag K, consume the data. Handle or explicitly reject pax x/g headers and links, depending on what your archive producer can emit.
4. Validate the resulting path stays inside the destination before writing.
5. Fail the whole extraction visibly and leave no ready marker when any entry fails, so the next launch retries instead of booting a partial tree.

## Conditions and alternatives

Using a bundled, tested tar library avoids the whole class. Alternatively have the build produce pax or restrict path length, but node_modules trees regularly exceed 100 bytes.

## Common mistakes

Counting a partial extract as success. Testing only with short paths. Treating the L payload as a file named ././@LongLink.

## Check it works

Build a payload containing paths over 100 and over 255 bytes, extract in the sandbox, compare the file count and digests with the source tree, and kill the app mid-extraction to confirm the next launch recovers.

---
{
  "schema_version": 2,
  "id": "offline-data-rescue-startup-isolation",
  "title": "Recover application data when normal Flutter startup fails",
  "topic_key": "storage.offline-data-rescue-startup-isolation",
  "area": "storage",
  "keywords": ["data rescue", "recovery ability", "offline maintenance", "startup failure", "corrupt bootstrap", "storage lease", "publication journal", "数据救援", "启动失败", "恢复导入", "损坏配置"],
  "summary": "Provide a minimal recovery entry before normal runtime initialization, obtain exclusive storage access, and restore through verified staging with a resumable publication journal.",
  "when_to_use": "A corrupt bootstrap file, database or plugin prevents the normal application from opening the screen needed to export or recover its data.",
  "status": "scoped"
}
---

# Recover application data when normal Flutter startup fails

## When to use

A corrupt bootstrap file, database or plugin prevents the normal application from opening the screen needed to export or recover its data.

## Recommended approach

Create a minimal recovery entry that does not construct the normal runtime or parse business data. Keep ordinary startup and maintenance under a shared storage-ownership contract, then verify imports in staging before publishing them through an explicit recovery state machine.

## Implementation steps

1. Select recovery mode before opening databases, preferences, normal loggers or plugins. On OH, a dedicated `FlutterAbility` can register only the required file-selection plugin and a maintenance channel. Export the matching native recovery ABI without requiring an application runtime handle. A separate ability is not automatically a separate process or storage owner.
2. Resolve application-owned storage from the platform context. Expose damaged bootstrap bytes and diagnostics without requiring successful decoding; let the user confirm the intended roots. Keep recovery staging and its journal outside the roots that will be replaced.
3. Acquire an exclusive maintenance lease before reading a consistent backup or modifying live data. Every normal writer, including background and CLI processes, must participate in the complementary lease. Report the live-owner conflict and require it to stop; a file lock cannot constrain unrelated code that ignores it.
4. Run blocking archive/file work on a native worker, returning completion on the owning channel thread. Preserve structured errors and distinguish a saved archive from a merely selected destination. See the [export contract](file-export-contract.md).
5. Validate archive format/layout, authenticated metadata, path membership, sizes and content digests before publication. Stream bounded chunks. Reject traversal, duplicate destinations and unsupported file types. Include required identities, configuration and supported secret material; do not silently claim a complete recovery when a required backend cannot export its keys.
6. Stage complete files on the destination volume and retain the original roots. Persist a journal before changing live paths; record each retention/publication step durably. Several directory renames are a sequence, not one atomic transaction. Block normal startup while publication is pending, and resume only through an explicit recovery operation that revalidates payloads and destinations.
7. Separate abandoned pre-publication staging from an interrupted publication. Clean up only owned uncommitted staging; retain displaced originals and failure diagnostics until recovery is verified. Preserve the journal on failure instead of opening a fresh empty database.

## Conditions and alternatives

Define archive compatibility by storage layout and schema, not only operating-system names. Native files and browser-origin stores can require different archive formats. Authentication, encryption, secret portability and device-bound keys need their actual backend contracts; a password prompt alone proves none of them.

If an independent recovery UI cannot start, an authorized external backup/maintenance tool is an alternative. It still needs the same root selection, exclusive access and interrupted-publication rules.

## Common mistakes

A recovery route inside the failing startup path is unavailable when needed. An exported ability should open a controlled maintenance UI, not treat incoming launch fields as authority to import or delete. Path validation and atomic rename do not establish resilience to every filesystem race, disk failure or device loss.

## Check it works

Inject invalid bootstrap bytes, database-open failure and normal-plugin failure. Open recovery, contend with a live writer, export and restore known data, reject malformed/truncated archives, and interrupt each publication phase. Verify normal startup stays blocked until explicit completion and that retained originals, identities and required credentials remain recoverable.

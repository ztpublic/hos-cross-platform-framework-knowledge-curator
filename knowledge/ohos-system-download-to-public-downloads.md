---
{
  "schema_version": 1,
  "id": "ohos-system-download-to-public-downloads",
  "title": "Move a system-agent download into the user Download folder",
  "topic_key": "file.ohos-system-download-to-public-downloads",
  "area": "file",
  "scope": "common",
  "frameworks": [],
  "summary": "Download with request.agent to a temporary file, get the target with DocumentViewPicker in DOWNLOAD mode, then move the file on completion and remove the task.",
  "when_to_use": "An app cannot write a download straight into the user's Download folder, or pausing shows a stale running state.",
  "applicability": "OH request.agent with DocumentPickerMode.DOWNLOAD. Reviewed in a Flutter plugin, but the APIs are ArkTS-level. A notification cannot be turned off according to a source comment. PC use involves large files and a visible Download folder, not exercised by this curation.",
  "keywords": [
    "request.agent",
    "DocumentPickerMode.DOWNLOAD",
    "Download folder",
    "moveFileSync",
    "saveas",
    "download task",
    "pause resume",
    "fileUri"
  ],
  "related": [
    "resumable-file-download-contract",
    "file-export-contract",
    "continuous-background-task-contract"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Move a system-agent download into the user Download folder

## When to use

An app cannot write a download straight into the user's Download folder, or pausing shows a stale running state.

## Recommended approach

Download with request.agent to a temporary file, get the target with DocumentViewPicker in DOWNLOAD mode, then move the file on completion and remove the task.

## Implementation steps

1. Create a task with request.agent.create, Action.DOWNLOAD and saveas set to a file in tempDir, with metered and gauge as your product requires.
2. Call DocumentViewPicker.save with pickerMode DOWNLOAD, and convert the result with new fileUri.FileUri(uri).path. Create the subfolder if it is missing.
3. Avoid overwriting by choosing a unique name when the target exists, for example adding (n).
4. On the completed event, call fs.moveFileSync from temp to target, then request.agent.remove. Handle a move failure explicitly.
5. Keep your stored state authoritative: a late progress event can report running after a pause, so check the stored paused state. Guard progress against an unknown total size.

## Conditions and alternatives

Use this for user-visible files. App-private data can stay in the sandbox with a plain task. Temp and target can be on different storage, so check that the move works for large files.

## Common mistakes

Treating the picker result URI as a path. Removing the task before moving. Dividing by an unknown size. Reporting failure of a move as a download failure with no retained file.

## Check it works

Download with pause, resume and cancel, a duplicate name, a large file, app restart in mid-task and low disk space, then open the file from the Files app.

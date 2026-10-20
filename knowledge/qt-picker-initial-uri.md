---
{
  "schema_version": 1,
  "id": "qt-picker-initial-uri",
  "title": "Pass only supported initial locations to the Qt OH document picker",
  "topic_key": "storage.qt-picker-initial-uri",
  "area": "storage",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Leave the initial location unspecified unless the adapter can supply a supported user-document URI.",
  "when_to_use": "The system picker reports it cannot open the initial application-private directory.",
  "keywords": [
    "defaultFilePathUri",
    "QFileDialog",
    "initial directory"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Applies to the Qt OH backend exposing DocumentViewPicker initialization; separate from authorization of the final selected file.",
  "related": [
    "document-picker-uri-access"
  ]
}
---

# Pass only supported initial locations to the Qt OH document picker

## When to use

The system picker reports it cannot open the initial application-private directory.

## Recommended approach

Leave the initial location unspecified unless the adapter can supply a supported user-document URI.

## Implementation steps

1. Inspect the caller's initial directory and the QPA value mapped to defaultFilePathUri.
2. Do not convert an arbitrary private sandbox path into a presumed selectable document URI.
3. Omit the native field for an empty/default initial location and validate nonempty user-document inputs.
4. Deploy both caller and QPA changes when both contribute to the faulty value.

## Conditions and alternatives

Applies to the Qt OH backend exposing DocumentViewPicker initialization; separate from authorization of the final selected file.

## Common mistakes

Requesting another permission does not repair an invalid initial URI.

## Check it works

Test no initial location, valid user-document location, private path and cancellation.

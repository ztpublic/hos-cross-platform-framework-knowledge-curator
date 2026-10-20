---
{
  "schema_version": 1,
  "id": "embedding-patches-and-dependency-order",
  "topic_key": "compatibility.embedding-patches-and-dependency-order",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "title": "Apply a narrowly scoped native compatibility patch reproducibly",
  "area": "compatibility",
  "summary": "Prefer a compatible release; when a patch is necessary, pin its input, apply it at the correct build stage and verify both matching and behavior.",
  "when_to_use": "An OH plugin/embedding API mismatch requires a source patch, but the affected source is downloaded or generated during the build.",
  "keywords": [
    "HAR patch",
    "oh_modules",
    "SDK API",
    "Hvigor",
    "getPageOffset",
    "补丁",
    "generated source",
    "patch oh_modules",
    "HAR compatibility patch",
    "generated dependency",
    "补丁不生效"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Flutter OH with the selected embedding/plugin versions; PC behavior requires the stated acceptance checks.",
  "related": []
}
---

# Apply a narrowly scoped native compatibility patch reproducibly

## When to use

An OH plugin/embedding API mismatch requires a source patch, but the affected source is downloaded or generated during the build.

## Recommended approach

Prefer a compatible release; when a patch is necessary, pin its input, apply it at the correct build stage and verify both matching and behavior.

## Implementation steps

1. Locate the owning dependency and classify the mismatch: missing API, changed signature, wrong build integration or packaging omission.
2. Check whether a supported version removes the need for a patch. Align the package/toolchain first when feasible.
3. If patching, record the expected input version and exact transformation in the build process. Keep it limited to the diagnosed incompatibility.
4. Apply the patch only after its source is materialized and before compilation. Verify expected matches and fail if the input differs unexpectedly.
5. Inspect the resulting build inputs; confirm the compiled/packaged copy is the one patched.
6. Reevaluate the patch whenever the SDK or dependency changes.
7. For keyboard/avoid-area crashes involving native window objects, preserve the native wrapper's mutation contract. Either copy the required rectangle scalars into an ordinary stable structure before updating them, or update supported scalar fields in place. Replacing a nested rectangle wholesale with an object literal can violate native object identity; do not infer runtime validity from an ArkTS type cast.

## Conditions and alternatives

Older direct native-module wiring and newer source-injection systems can require different integration points. Do not combine recipes from unrelated build generations blindly.

### Remove retired workarounds after checking the original fault

Separate a compiler/cache mismatch from an embedding/plugin runtime defect before patching core semantics. Replacing a native resolver result with a constant can mask the wrong input rather than fix it. If startup fails, preserve the first error and avoid unbounded recovery restarts while diagnosing; a broad recovery disablement is a diagnostic policy, not a universal shipping fix.

After selecting a compatible SDK/backend, remove obsolete patches, source overlays, generated-registration hooks and their extra resolution paths as one coherent change. Recheck the capability those patches supplied instead of assuming removal proves equivalent behavior. Moving database message decoding from a worker to the main thread can avoid one startup error but restore large-batch UI stalls; evaluate both contracts before adopting it.

## Common mistakes

Replacing a call with a constant can change behavior even if compilation succeeds. A text patch matching nothing is not success, and an unrecorded cache edit is not reproducible.

## Check it works

Build from clean dependency inputs, verify patch application and packaging, then exercise the specific affected keyboard/window/WebView/native-asset behavior.

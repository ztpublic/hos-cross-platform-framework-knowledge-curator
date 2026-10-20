---
{
  "schema_version": 1,
  "id": "plugin-graph-source-and-api",
  "topic_key": "plugins.plugin-graph-source-and-api",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "title": "Select an OH plugin implementation that matches your Dart API",
  "area": "plugins",
  "summary": "Resolve the app-facing package, platform interface and OH backend as one compatible dependency graph, and adapt callers to the selected API generation.",
  "when_to_use": "A package resolves but its API differs from the examples, its OH implementation is missing, or a fork introduces dependency conflicts.",
  "keywords": [
    "dependency_overrides",
    "pubspec",
    "GitCode",
    "Gitee",
    "插件",
    "monorepo",
    "version",
    "lockfile",
    "plugin incompatible",
    "dependency conflict",
    "OH backend",
    "插件适配",
    "找不到插件"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Flutter OH with the selected embedding/plugin versions; PC behavior requires the stated acceptance checks.",
  "related": [
    "native-registration-and-handler-coverage",
    "picker-result-readable-content"
  ]
}
---

# Select an OH plugin implementation that matches your Dart API

## When to use

A package resolves but its API differs from the examples, its OH implementation is missing, or a fork introduces dependency conflicts.

## Recommended approach

Resolve the app-facing package, platform interface and OH backend as one compatible dependency graph, and adapt callers to the selected API generation.

## Implementation steps

1. Identify the exact methods your feature uses, including cancellation, streams and optional flags.
2. Inspect each selected package's declared name, source revision, monorepo subdirectory, Dart/Flutter constraints and backend dependency.
3. Confirm whether the app-facing package endorses the OH backend or the app must select it explicitly. Review transitive Git branches and local-path dependencies as well as direct entries.
4. Pin reproducible sources and retain the application's dependency lock. Use root overrides for a specific source/version conflict, not as proof of API compatibility.
5. Adjust callers where the fork exposes a different API shape: scalar versus list results, instance versus static methods, renamed options or event semantics.
6. Trace one call end to end before integrating the whole feature.

For file selection, an API change can replace `PlatformFile` with `XFile` as well as the picker method. Adapt content reading, filters and nullable cancellation results together; see [reading selected content](picker-result-readable-content.md). For notifications, verify that the application's platform factory selects the new backend as well as resolving its package.

## Conditions and alternatives

A plugin that already has a compatible OH release may require no override. A fork selected by package name also changes the shared code used on your other targets.

### Use explicit platform implementations without assuming registration

A federated OH implementation can be an ordinary pinned dependency alongside the shared interface and other platform packages when its metadata and versions match. Keep one root lock/resolution path and verify the generated OH registrant, native package integration and actual supported API; a package appearing in the root dependency list does not establish that it was selected or registered. If a temporary override graph or source overlay is retired, verify that public shared callers still reach the intended backend.

At the call boundary, adapt signature and lifetime changes as well as names: callback versus stream events, asynchronous disposal, binary payloads and readable picker content can change between an upstream package and its OH fork. Review these contracts after synchronization even when Dart version constraints are unchanged.

### Replace a vendored generator or fork deliberately

For an OH-capable Pigeon generator, verify that the selected monorepo package exposes `arkTSOut` and `ArkTSOptions` before retiring a local copy. Retain the resolved Git revision and regenerate all target bindings from one interface definition. Compare codecs, nullable arguments, error envelopes and channel suffixes together; a newer version string alone does not establish compatibility. See [host setup and handler coverage](native-registration-and-handler-coverage.md).

Check the effective Dart/Flutter constraints in the resolved graph, including dev tools. Updating code generators can raise the required Dart version even when the application's broad SDK declaration stays unchanged. Returning an OH plugin fork to a hosted package also requires checking its current OH implementation metadata, registration and exact feature; a lockfile source change does not prove preserved drag-and-drop or other native behavior.

Dependency resolution can succeed while package code calls an API missing from the selected transitive version. For errors such as an undefined archive-decoder method, compare the actual caller API with the resolved archive library and any cap imposed by another dependency. Use a narrow, temporary compatible version range or upgrade the complete SDK/package graph; do not force a newer transitive library past a known consumer constraint. Record the condition that permits removing the workaround.

For specialized media features, align the player API, video controller and OH native-library packages to the same compatible implementation. A custom HDR constructor argument in Dart is not evidence that a stock native backend recognizes it.

## Common mistakes

A repository URL without the package subdirectory may select no package. Familiar package names do not imply identical interfaces. Do not force an override just to silence the resolver.

## Check it works

Inspect the resolved source and run the exact feature, not only dependency resolution. Check cancellation/errors and the same public API on your other supported platforms.

# Atomic authoring and classification

Keep the established consumer shape: **When to use; Recommended approach; Implementation steps; Conditions and alternatives; Common mistakes; Check it works**. Checks are proposed acceptance cases unless corresponding execution evidence exists in provenance.

A useful atom answers one question: what change resolves this failure under these conditions? Multiple steps needed for one lifetime or transaction contract belong together. Split topics when they can be applied, fail, or be verified independently. For example database-engine selection, typed-preference migration and cross-process store access are different entries. Do not split every numbered step into a shallow fragment.

Keep concrete API names, error strings, ordering, units, configuration and ownership. Remove project names, history narration, source paths, commits, dates and license metadata from consumer guidance and the index. Neutral configuration paths and API examples may remain when required to implement the solution. Put source identity and attribution in provenance.

## Generalization test

1. Remove the source framework's wrapper: does the cause and remedy still hold for another consumer of the same platform/native API?
2. If yes, write a common entry with the OS/runtime preconditions. If a concrete adapter remains useful, retain a framework-specific linked entry.
3. If the behavior depends on QPA, Flutter's messenger, React Native's bridge, an Electron runtime port or Kotlin/Native target tooling, keep it specific even when another framework has a superficially similar symptom.
4. A generalization from one framework is an inference, recorded in provenance. It is not independent corroboration or a successful port of another framework.

Examples: URI/path/grant separation is common; configuring QFileDialog's initial URI is Qt-specific. Resolving symbols and ELF dependencies is common; Flutter federated registration is Flutter-specific. Rust target ABI details are common native-layer conditions, not evidence of Kotlin Multiplatform or RN support.

## PC and evidence

Preserve phone/tablet origins and unknown product support. In `applicability`, state the relevant PC boundary and version/backend requirements. In `pc_validation`, use `not-run` for an import even if the source reports device success; keep that claim in provenance. A future actual PC validation must identify device class, OS/API, build inputs, exercised behavior and evidence artifacts.

Keep experimental/proposed fixes scoped. Prefer complete dependency builds or disabling an unused feature over empty ABI stubs. Do not retain a source's unconditional workaround when its own evidence only shows a local experiment. Record the correction and rationale; preserve the original snapshot.

Check both semantic duplicates and scope drift. A common lesson can reference adapters without requiring them. Don't append a second unrelated fix to a long Conditions section: create a new atom and link it. Broad API catalogs, setup policies, end-to-end migration plans and version status dashboards are research sources rather than atomic consumer entries.

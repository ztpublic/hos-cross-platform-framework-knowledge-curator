# AI tooling opportunities for HarmonyOS cross-platform migration

Date: 2026-10-21, Asia/Shanghai.

The strongest direction is a **feature-preserving migration workbench**: an agent harness that understands what an application does, identifies the boundary preventing that behavior on HarmonyOS, helps implement the missing integration, and demonstrates the resulting behavior. Knowledge retrieval and accessibility-based crash exploration become components of this workbench.

The important unit of progress is a working feature. A package resolving, a HAP building, an application launching, and a feature behaving correctly are separate milestones. The product should make those distinctions useful to the author while doing the integration work between them.

## What this recommendation reviewed

- All **187 catalog entries**, including their problem/solution metadata and acceptance-check sections, across Flutter, Qt, Electron, React Native and Kotlin Multiplatform. Selected integration, dependency, storage, lifecycle and rendering entries were read in greater detail, with their provenance inspected where needed.
- The **20 direct project registrations**, their source-to-entry associations, and all **60 imported Flutter project records**. The two imported knowledge collections are additional evidence inputs, not two additional applications. Imported Qt material includes further project history that is not an independently enumerated project sample here.
- The existing [feature-loss investigation](../2026-10-21-feature-loss/REPORT.md), its **33 feature cases**, source excerpts and review coverage. Its selected review covered 39 source/document files and three historical diffs across 21 projects. This proposal reuses that saved evidence; it does not claim to have repeated the original source investigation.
- Current primary documentation for existing DevEco capabilities and selected native services. Some Huawei pages expose substantive search extracts while direct opens return a JavaScript shell or timeout; those limits apply to the external checks below.

The database integrity check passed: **88 common mechanisms and 99 framework-specific mechanisms** (32 Flutter, 43 Qt, 13 Electron, 5 React Native, 6 KMP). There are 138 imported-guidance entries and 49 source-reviewed entries. All 187 still record `pc_validation=not-run`. The proposals below are product judgments derived from that evidence, not new device results or estimates of ecosystem-wide frequency.

[coverage.json](coverage.json) records every catalog entry, every registered project, all selected feature-loss findings, input hashes and proposed opportunity associations. Associations are editorial routing suggestions; their counts do not measure impact, development effort or independent incidents.

## What the projects actually spend effort solving

| Problem family | Concrete evidence in the studied material | Product implication |
| --- | --- | --- |
| Silent feature reduction | NipaPlay's gamepad replacement exposes empty streams; Mi Workbench disables BLE scanning; ETOHOS substitutes an application-local clipboard; ConnSSH rejects batch downloads; KTouch deliberately cuts large KDE flows. | Detect lost behavior and distinguish intentional scope from missing integration. A successful build can conceal substantial loss. |
| An incompatible combination of otherwise plausible parts | Flutter package/platform-interface/backend graphs; compiler/engine/HAR cache coherence; QtCore/QPA/ArkTS template mismatches; Electron Node ABI; OH-adapted KMP artifacts. | Solve compatibility across the whole integration, including other supported targets. Individual package recommendations are insufficient. |
| Native code and resources that do not reach the installed application | Versioned SONAMEs omitted from Flutter packaging; stale Electron frontend injection; RNOH HAR extraction and offline bundles; Qt QML/plugin/resource deployment. | Inspect the actual artifact and connect it to the source that produced it. |
| Framework-to-system wiring | Flutter plugin selection and messenger registration; RNOH C++/ArkTS registration; KMP C exports and host mounting; Qt application initialization; cold/warm Want delivery. | Generate and verify the entire route from shared code to system behavior. |
| Cross-runtime ownership and timing | NAPI handles crossing threads; native event loops using destroyed handles; asynchronous player teardown; callbacks settling the wrong RTC or OAuth attempt. | Provide causal debugging and lifetime analysis across language and runtime boundaries. |
| Different desktop/window/rendering contracts | Qt subwindow input and surfaces; high-DPI counted twice; Kuikly resize ordering; Flutter 2-in-1 classification; GL translation state; HDR/PiP surface changes. | Treat PC input, rendering and window transitions as first-class migration requirements. |
| Storage and external interoperability | URI versus path versus grant; persistent folder authorization; database relocation/WAL; credential-backend guarantees; receiver handoff versus completed save/import. | Verify durable data and observable outputs across app boundaries. |
| Process-independent and system-owned behavior | Background media/transfers; reminders after exit; native cards; media commands; distributed discovery versus synchronization. | Exercise the system interaction over time and after application lifecycle changes. |
| Real policy or distribution uncertainty | Reported PTY denial, standalone executable/HNP eligibility, JIT authorization and reminder entitlement. | Establish the exact target conditions, run scoped probes where possible, and suggest honest architectural alternatives. |
| Ongoing divergence | Vendored packages, embedding patches, custom native assets, SDK forks and upstream changes that restore or overwrite prior choices. | Maintain a port continuously and retire obsolete workarounds. |

The feature-loss investigation classifies 19 of its 33 selected cases as dependency/backend gaps, four as deliberate scope, six as restorations/counterexamples, one as a runtime workaround, one as a reported restriction, and two as unresolved. These are selected feature cases, not a prevalence study. They support pursuing adapter recovery and better diagnosis without assuming that every disabled feature is recoverable unchanged.

## Feature portfolio

### M1 — Migration feature audit and capability investigator

**Author experience:** “Tell me what this application will lose on HarmonyOS, why, and what choices I have.”

Build a feature inventory from routes, menus, services, dependency usage, platform branches, existing tests and a working reference app. Trace each feature through shared code, framework adapter, native library, system API and product eligibility. Surface unreachable OH branches, empty implementations, success-shaped stubs and disabled UI.

The output should be a living capability ledger. Each feature has a current state, an evidence link, a blocking layer and a proposed next action: connect an existing backend, implement an adapter, replace a dependency, investigate policy, redesign the interaction, or retain an explicit product cut. Missing evidence remains unknown.

An ambitious extension is **alternative architecture exploration**. For scripting, image processing or terminal services, compare embedded native code, a separately packaged runtime, a WebAssembly backend, and an external service where relevant. Make effects on offline operation, latency, functionality, distribution and maintenance explicit. A pipe-based shell must not be presented as preserving PTY semantics; an image-derived palette does not reproduce Android Material You.

**Grounding:** feature-loss cases F01–F33, especially gamepad, BLE, clipboard, reminders, batch exports and historical restorations; [capability initialization](../../../knowledge/capability-gated-backend-initialization.md), [privilege gates](../../../knowledge/privileged-capability-gating.md), [SDK mismatch](../../../knowledge/sdk-version-versus-os-limitation.md).

**Feasibility:** valuable in bounded form now; complete automatic discovery of every product feature remains a research problem. Feature importance and intentional cuts need author input.

### M2 — Compatibility solver for the complete project

**Author experience:** “Find a combination of SDKs, runtimes, packages and native artifacts that supports these features across my existing targets.”

Resolve a compatible integration rather than swapping dependencies one at a time. Consider the actual methods used, platform-interface versions, generated bindings, framework forks, compiler/runtime identities, native ABI, renderer mode and packaging support. Explain an incompatible constraint set and offer alternatives that retain the requested behavior.

The solver should identify when an OH override changes Android/iOS/desktop code, when a package exists but lacks a required method, and when versions match in name while artifacts disagree. It should propose narrowly scoped compatibility patches and explicit conditions for removing them.

**Grounding:** [Flutter plugin graph](../../../knowledge/plugin-graph-source-and-api.md), [Flutter cache coherence](../../../knowledge/flutter-engine-cache-coherence.md), [Qt integration coherence](../../../knowledge/qt-qpa-runtime-template-coherence.md), [Electron addon ABI](../../../knowledge/electron-ohos-native-addon-abi.md), [KMP dependencies/renderers](../../../knowledge/kmp-ohos-dependency-and-renderer.md), [KMP target isolation](../../../knowledge/kmp-ohos-compose-target.md).

**Feasibility:** bounded compatibility diagnosis is practical. A universal solver needs observed compatibility evidence that package manifests alone do not provide.

### M3 — Native dependency porting foundry

**Author experience:** “This missing native dependency blocks three features. Help produce a usable OH build and prove the subset we need.”

Inventory native dependencies and their feature consumers, then port the required functionality: toolchain configuration, host-versus-target generators, musl boundaries, Rust target configuration, C ABI differences, symbol exports, PIC, SONAME closure and runtime loading. Generate focused compile/load/behavior probes and reduce failures to small reproducible examples.

The ambitious outcome is a reusable catalog of OH native artifacts with exact source/build inputs, ABI, dependency closure, license information and operation-level results. An application could reuse a validated FFmpeg capability or image-processing subset rather than rediscovering the same port. A compiler fix that simply removes the required codec or scripting functionality should register as a feature loss.

**Grounding:** Pure Live recording/hardware-decoder gaps; DSH's libvips substitution; KLayout scripting exclusions; [native dependency closure](../../../knowledge/native-dependency-closure.md), [musl boundaries](../../../knowledge/musl-portable-libc-boundaries.md), [Rust integration](../../../knowledge/rust-bridge-target-and-packaging.md), [target C ABI](../../../knowledge/ffi-c-char-target-abi.md).

**Feasibility:** substantial automation is plausible for libraries and known patterns. Arbitrary native-library ports, GPU backends and restricted execution paths remain open-ended engineering work.

### M4 — Native adapter factory and capability transplantation

**Author experience:** “Implement the missing system integration while preserving my shared application's interface.”

Infer the contract from existing Android/iOS/desktop implementations and actual callers: operations, events, errors, cancellation, permissions, persistence and lifetime. Generate the OH backend and its framework-specific registration, host declarations and shared-code selection. Check the complete call route instead of producing only generated bindings.

Candidate integrations include file selection/export, clipboard, TTS, scanning, BLE, media sessions, reminder scheduling, cards, deep links and status-bar behavior. System capability availability is a starting point; device protocol, API version, entitlement and framework semantics still determine the implementation.

The most ambitious extension is **capability transplantation**: find a working integration in another studied port, extract its underlying platform contract, and adapt that contract to the receiving framework. An Electron status-bar integration could inform a Flutter backend; a Qt work-folder grant could inform another native host. Transfer semantics and evidence, with license/provenance tracking, rather than assuming source code is interchangeable.

Over time, shared platform mechanisms could become reusable native service components with thin framework adapters. Keep framework-specific surface ownership, event loops and UI lifetimes explicit; some integrations will not share one implementation cleanly.

**Grounding:** BeeCount reminder factory, Kelivo TTS, Mi Workbench BLE and Pure Live scanning/login; [Flutter handler coverage](../../../knowledge/native-registration-and-handler-coverage.md), [RNOH host](../../../knowledge/rnoh-native-host-and-codegen.md), [KMP host](../../../knowledge/kmp-ohos-compose-host.md), [Qt system media bridge](../../../knowledge/qt6-ohos-avsession-bridge.md).

**Feasibility:** a strong opportunity for bounded service contracts. Universal semantic translation and reliable automatic transplantation are research directions.

### M5 — Build, package and deployment doctor

**Author experience:** “Explain why this build succeeded but the installed app is stale, incomplete or unable to start.”

Connect source inputs, generated outputs, packaged resources, native libraries, signing identity, installed application and runtime diagnostics. Detect skipped native builds, stale Electron frontend copies, missing versioned libraries, duplicate producers, wrong architecture, absent offline assets, incompatible templates and the wrong active signing product.

Return a specific failure chain and a targeted repair, with artifact inspection and runtime confirmation kept separate. Use a recognizable source change or artifact digest to establish what actually reached the application. Include a reproducible local/CI environment diagnosis and distinguish dependency-network failures from source incompatibility.

**Grounding:** [native rebuild evidence](../../../knowledge/hvigor-native-rebuild-evidence.md), [Electron resource injection](../../../knowledge/electron-ohos-resfile-injection.md), [Flutter versioned libraries](../../../knowledge/flutter-ohos-versioned-so-packaging.md), [single producer](../../../knowledge/native-packaging-single-producer.md), [signing identity](../../../knowledge/hap-signing-identity.md), [RNOH offline assets](../../../knowledge/rnoh-offline-bundle-and-fonts.md).

**Feasibility:** among the most concrete opportunities. The distinctive value is explaining cross-framework artifact lineage and repairing the integration, beyond invoking build/install commands.

### M6 — Cross-language causal debugger and lifetime investigator

**Author experience:** “Show where this operation stopped, which runtime owns it, and why the crash or hang follows.”

Follow an operation through Dart/JS/Kotlin/Qt, ArkTS, NAPI and C/C++/Rust, using the runtimes actually present. Correlate request identity, engine/messenger identity, thread, native handle, callback and teardown. Join application logs, native stacks, fault logs, source and packaged symbol identities into a causal explanation.

The ambitious version proposes instrumentation, reduces a failing sequence, and suggests a repair for wrong-thread access, stale callbacks, missing registration, double completion, shared-buffer lifetime, use-after-free or shutdown waits. It should distinguish “public operation completed” from “native resources released.”

**Grounding:** [NAPI ownership](../../../knowledge/napi-thread-and-handle-lifetime.md), [native event-loop teardown](../../../knowledge/native-event-loop-handle-teardown.md), [shared-buffer ownership](../../../knowledge/native-ffi-shared-buffer-ownership.md), [Qt JS gateway](../../../knowledge/qt-js-thread-gateway.md), [RTC correlation](../../../knowledge/rtc-join-and-participant-correlation.md), [Electron main-process debugging](../../../knowledge/electron-ohos-main-inspect.md).

**Feasibility:** log/trace correlation and targeted instrumentation are practical; dependable automatic root-cause diagnosis across arbitrary runtimes is ambitious.

### M7 — Reference-app behavior twin and feature-parity oracle

**Author experience:** “Learn this workflow from the working app, then tell me whether the HarmonyOS port preserves the outcome.”

Observe or replay workflows on a functioning Android/iOS/desktop version and express their meaningful outcomes. Run equivalent workflows on OH despite different widgets, system dialogs and navigation. Compare resulting files, records, protocol messages, external handoffs, session state and visible behavior. Use accessibility and screenshots as available observations alongside those outputs.

Examples: an export must be readable with correct bytes; clipboard data must reach another application; a reminder must arrive after process exit; cancellation must leave the old destination intact; a paused video must remain paused after surface reconstruction.

The oracle should separate functional parity, deliberate native adaptation, accepted degradation and untested behavior. Match semantic outcomes and app-specific tolerances rather than requiring pixel identity. For black-box applications, confidently verifying every behavior may be impossible; low-observability claims need additional probes or human review.

This is the most direct expansion of the existing QA tool. It converts exploration into migration acceptance and can supply concrete repair objectives to the coding agent.

**Grounding:** the 33 feature-loss cases; [export completion](../../../knowledge/file-export-contract.md), [cross-app clipboard](../../../knowledge/qt-ohos-external-clipboard-paste.md), [observed playback state](../../../knowledge/native-playback-state-observation.md), [external handoff](../../../knowledge/external-handoff-file-uri.md), [reminder lifetime](../../../knowledge/system-reminder-identity-and-lifetime.md).

**Feasibility:** strong for author-selected workflows with observable outputs. General automatic oracle construction is a major research direction.

### M8 — Lifecycle and system-interaction chaos lab

**Author experience:** “Find the transition sequences that break this migrated feature, and give me the smallest reproducer.”

Systematically vary foreground/background, freeze/resume, process death, slow runtime readiness, cold/warm Wants, permission revocation, callback delay, network loss, disk errors, cancellation and native-owner recreation. Generate combinations from the feature contract and minimize failures into replayable scenarios.

Test behavior owned outside the main UI: media keys, scheduled reminders, cards, duplicate launches, file opening, secondary abilities and external applications. Where the platform does not permit forcing a condition, report the missing coverage rather than simulating it as equivalent.

**Grounding:** [cold-start delivery](../../../knowledge/incoming-wants-before-runtime-ready.md), [background tasks](../../../knowledge/continuous-background-task-contract.md), [OAuth correlation](../../../knowledge/oauth-browser-callback-correlation.md), [continuation readiness](../../../knowledge/continuation-cached-state-and-readiness.md), [window state](../../../knowledge/window-observed-state.md), [resumable transfer](../../../knowledge/resumable-file-download-contract.md).

**Feasibility:** many scenarios are automatable; hardware, policy and process-control limitations constrain the coverage matrix.

### M9 — Desktop experience, rendering and performance adaptation lab

**Author experience:** “Make this port behave correctly as a PC application across real windows, displays and input devices.”

Exercise resize/maximize/minimize, multi-window ownership, modal dialogs, focus, keyboard shortcuts, mouse/trackpad, text input, density changes, large text, theme and native accessibility. Compare logical layout, physical surfaces and input coordinates to isolate scaling and hit-testing faults.

For media and 3D, combine visible output with native measurements: surface creation, decoder selection, HDR signaling, frame timing, memory, GL state and resource release. Detect a stable but blank viewport, software decoding mistaken for hardware decoding, or an interactive control that the accessibility bridge never exposes.

An ambitious feature is automatic architectural diagnosis: identify whether a visual defect originates in application layout, embedding, QPA, compositor, translation layer or native media output, then propose the smallest useful experiment. Actual HDR and physical trackpad behavior require suitable hardware; injected events and screenshots have narrower evidentiary value.

**Grounding:** OhShark subwindows/input/DPI; FreeCAD GL translation and batching; VLC video/subtitle/AVSession; Kuikly resize; [font metrics](../../../knowledge/qt-ohos-default-font-metrics.md), [HDR/surface contract](../../../knowledge/hdr-output-surface-and-capability.md), [accessibility activation](../../../knowledge/qt-accessibility-bridge-activation.md).

**Feasibility:** broad diagnostic automation is plausible; full graphics correctness and hardware qualification remain demanding.

### M10 — Data continuity and interoperability auditor

**Author experience:** “Verify that this port preserves user data and can exchange real content with the system and other apps.”

Check database engines, schema/types, old versus new storage roots, bundle identity, WAL-safe migration, preferences revisions, shared native extensions and credential guarantees. Exercise populated upgrades, corruption, interruption, concurrent writers, backup/recovery and failures that could otherwise become a fresh empty database.

Audit file workflows end to end: URI identity, operation-specific grants, persistent access, removable storage, large/empty/Unicode content, staging, exact output bytes and delayed receiver access. For network-backed content, preserve authentication, certificate validation, proxy policy, cancellation and resume identity.

The ambitious outcome is an evidence-backed data migration rehearsal for a realistic synthetic corpus, with detected semantic differences and an app-specific recovery route. Device-bound credentials and non-exportable keys may require reauthentication; a tool cannot guarantee portable secrets by copying files.

**Grounding:** [database replacement](../../../knowledge/database-backend-and-data-contract.md), [URI/grant contract](../../../knowledge/document-picker-uri-access.md), [work-folder grants](../../../knowledge/qt-ohos-work-folder-grant.md), [secure storage](../../../knowledge/secure-storage-backend-selection.md), [recovery isolation](../../../knowledge/offline-data-rescue-startup-isolation.md), [transport policy](../../../knowledge/network-adapter-policy-propagation.md).

**Feasibility:** highly useful when the author can supply schemas, migration rules and synthetic fixtures; generic semantic guarantees are out of reach.

### M11 — Continuous upstream synchronization agent

**Author experience:** “Keep my HarmonyOS port current as the original app, framework and OS evolve.”

Track adaptation intent alongside source patches. On upstream changes, identify affected contracts and feature consumers, refresh bindings and adapters, rebase narrowly scoped patches, and check the original platforms as well as OH. Explain semantic regressions even when Git merges cleanly.

Look for workaround retirement: a native implementation becomes available, a newer SDK restores a missing package API, or an earlier runtime fault no longer reproduces. Detect obsolete capability comments and avoid carrying historical cuts forward forever. Changes can arrive as reviewable patches or draft PRs under the author's chosen workflow.

The ambitious version optimizes for **smaller sustained divergence**: reusable framework fixes and upstream-compatible abstractions that reduce the maintenance burden for many apps.

**Grounding:** [embedding patch lifecycle](../../../knowledge/embedding-patches-and-dependency-order.md), [platform vendoring](../../../knowledge/flutter-ohos-target-platform-vendoring.md), [shared compilation](../../../knowledge/shared-source-platform-detection.md), database-executor regression risks, and source restorations in Bugaoshan OCR, PiliPlus charts and FreeCAD UI/rendering.

**Feasibility:** reviewable update proposals are practical; unattended correctness across changing external services and devices is a substantially higher bar.

### M12 — Executable migration knowledge and regression benchmark

**Author experience:** “Reuse an adaptation with its preconditions, checks and evidence, then know when that advice stops applying.”

Extend the existing knowledge database with optional executable assets: minimal probes, contract tests, artifact inspectors, failure reproductions and narrowly applicable transformations. Bind every result to actual SDK/runtime/artifact/device conditions, and retain provenance/license boundaries.

The knowledge base becomes an experiment memory. It can retrieve a relevant mechanism, propose the discriminating check, consume the result and explain the next step. A source-reviewed recipe stays distinct from an independently reproduced runtime result.

Use historical migration problems to construct a benchmark with defective and repaired revisions, behavior assertions and held-out checks. Score preserved/restored features, correct failure classification, regressions on other targets, data correctness and maintenance divergence. Track build/launch success as intermediate outcomes. Separate infrastructure/policy unavailability from product failures.

**Grounding:** the database's existing JSON metadata, acceptance sections, source hashes, import mappings and evidence tiers; all other opportunities can contribute validated experiments.

**Feasibility:** contract-specific assets are practical. Reliable benchmark construction requires real reproductions; existing source reports alone are insufficient test cases.

## Product judgment

If choosing one flagship, choose **M7, the behavior twin**, paired with **M1, the capability ledger**. It gives authors a concrete answer to the most consequential question: which user workflows survived this migration? It also makes the existing QA system more useful without confining the product to crash detection.

If choosing the most ambitious code-producing capability, choose **M4, the adapter factory**, supported by M2/M3/M5. It directly addresses recurring missing integrations and can eventually reuse platform contracts across frameworks.

If choosing an enduring maintenance product, choose **M11, continuous synchronization**, grounded in the same feature contracts and runtime checks. Getting a port working once and keeping it viable are different developer needs.

| Opportunity | Judgment of value for this workspace | Main uncertainty |
| --- | --- | --- |
| M1 feature/capability audit | Broad foundation; makes existing evidence actionable | Completeness of feature discovery |
| M2 compatibility solver | Broad recurring migration work | Incomplete compatibility facts |
| M3 native dependency foundry | Can unblock entire feature clusters | Open-ended native engineering |
| M4 adapter factory/transplantation | Strongest ambitious creation feature | Semantic equivalence and host integration |
| M5 artifact/deployment doctor | Concrete and widely useful supporting tool | Toolchain and packaging variation |
| M6 cross-runtime debugger | Potentially substantial time savings on difficult faults | Instrumentation/observability across runtimes |
| M7 behavior twin | Strongest flagship and extension of existing QA | Reliable behavior oracles |
| M8 lifecycle chaos | Finds failures ordinary exploration misses | Controllable system states |
| M9 desktop/rendering lab | Particularly relevant to HarmonyOS PC | Hardware and graphics observability |
| M10 data/interop auditor | High-consequence correctness work | Application-specific data semantics |
| M11 upstream synchronization | Durable value after initial migration | Semantic drift despite clean merges |
| M12 executable knowledge/benchmark | Compounds existing assets and measures progress | Cost of obtaining trustworthy reproductions |

These are qualitative priorities, not measured market demand or ROI. No implementation schedule is implied.

## How the harness should feel

An author should be able to ask: “Migrate this app and preserve these ten workflows,” “Restore scanning on this target,” “Explain why the HAP contains old UI,” or “Update upstream while retaining our OH database and window behavior.”

Each task should maintain a compact record: desired behavior, observed failure, suspected boundary, relevant knowledge, proposed change, execution evidence and remaining uncertainty. The harness can alternate between source inspection, a small diagnostic experiment, code generation and behavior verification, while retaining the reasoning needed for review and later updates.

The model contributes semantic understanding, hypothesis selection, adaptation and failure explanation. Compilers, loaders, protocol assertions, byte comparisons and runtime observations establish the corresponding facts. This division makes the tool useful even when model confidence is misplaced.

Existing platform tools should supply execution and diagnostic capabilities. Huawei already advertises an AI Agent spanning generation, repair, build and verification, along with cross-language debugging and profiling in [DevEco Studio](https://developer.huawei.com/consumer/cn/deveco-studio); [AppAnalyzer](https://developer.huawei.com/consumer/cn/doc/doccenter-deveco-studio/ide-app-analyzer-scenes) already supports scenario checks and performance diagnostics. [ArkXtest](https://github.com/openharmony/testfwk_arkxtest) supplies OpenHarmony test infrastructure. The proposed differentiation is feature preservation across frameworks, deeper integration diagnosis, portable adaptation contracts and continuous maintenance. This review does not establish that any individual proposed capability is absent from all existing tools.

Native service availability should be established per target. Primary examples checked here include [PC status-bar API changes](https://developer.huawei.com/consumer/en/doc/harmonyos-releases/js-apidiff-desktopextensionkit-6003) and [BLE connection/reconnection guidance](https://developer.huawei.com/consumer/cn/doc/doccenter-industry-solutions/btreconnectdemo-0000002757080811). These show integration opportunities, with device/API/product conditions; they do not validate a particular Flutter plugin, peripheral protocol or full desktop API equivalent.

## Scope of the work performed

This work reviewed local catalog/project evidence, inspected selected consumer/provenance records, checked current primary documentation, and wrote this recommendation plus its coverage record. It did not update source-project revisions, change consumer knowledge, advance source checkpoints, build/sign/install applications or reproduce device behavior. The existing QA tool is taken as described by the author; its implementation was not audited here.

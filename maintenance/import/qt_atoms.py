# Reviewed Qt/native atoms, consumed by import_knowledge.py; no source commands executed.
C='ohos-common-kb-public/'
G='semantic/qt-harmonyos-golden-rules.md'

def common(id,title,area,when,summary,steps,conditions,mistakes,checks,paths,keywords=(),related=(),status='scoped'):
    qt(id,title,area,'common',when,summary,steps,conditions,mistakes,checks,paths,keywords,related,status)

def qatom(id,title,area,when,summary,steps,conditions,mistakes,checks,paths,keywords=(),related=(),status='scoped'):
    qt(id,title,area,'qt',when,summary,steps,conditions,mistakes,checks,paths,keywords,related,status)

common('document-picker-uri-access','Keep document URI identity separate from path and access grants','storage',
'A selected document cannot be read or written, or a remembered selection stops working after restart.',
'Preserve the original URI and its operation-specific grant; convert to a path only when the selected native backend requires and permits it.',
['Return a typed selection containing URI, name and known access mode/lifetime. Distinguish cancellation before attempting I/O.', 'Use the target SDK fileuri/fileShare or equivalent supported APIs to resolve access. Converting URI text does not grant read, write or persistent access.', 'Pass an authorized handle or verified path to native code. Test read, create, overwrite, rename, delete and directory-child access independently.', 'Persist and reactivate grants only where supported; otherwise offer reauthorization or an explicit private import copy.'],
'Relevant to all framework adapters using HarmonyOS document selection, including PC external volumes. File and directory authority, API level and access lifetime can differ.',
'Do not strip a URI prefix into a path, infer recursive access, or treat permission denial as file deletion.',
'Select a file and a directory; test cancellation, separate read/write access, restart, revocation and removal of the underlying volume.',
[C+'semantic/document-picker-uri-access.md',C+'problems/document-picker-uri-access-failure.md'],['DocumentViewPicker','file picker','URI','permission','文件选择','授权'],['persistent-file-access-grants','sandbox-import-as-access-fallback'])

common('native-code-load-location','Load native libraries from supported code locations','native',
'dlopen rejects a library copied into cache or another writable data directory.',
'Package fixed native code through the supported HAP distribution path and diagnose loader policy separately from ABI and dependency failures.',
['Capture the resolved path and native loader error inside the application process.', 'Inspect ELF architecture, required symbols and NEEDED dependencies before attributing the error to permission policy.', 'Place fixed libraries in the target ABI packaging inputs and rebuild/sign through the application distribution workflow.', 'If dynamic code delivery is a real requirement, use a supported platform delivery mechanism and validate its signing and lifecycle contract.'],
'Applies when the target product enforces code-location/integrity policy. Ordinary writable storage is not automatically an executable-code location.',
'Changing file mode or moving a library into private cache does not establish loader permission. Do not replace missing dependencies with empty ABI stubs.',
'Confirm the library loads from the installed package in a clean app and that each required symbol/dependency resolves; separately observe the rejected-path case.',
[C+'problems/dlopen-rejects-writable-path.md','problems/runtime-fail-dlopen-writable-path.md'],['dlopen','可写路径','loader','QLibrary','QPluginLoader'],['ffi-library-and-symbol-availability'])

common('bundle-data-write-location','Copy mutable seed data out of installed resources','storage',
'Updating a bundled configuration, database or model fails with read-only or permission errors.',
'Read immutable resources in place; initialize mutable copies in platform-provided application data storage before editing.',
['Classify the artifact as resource, persistent data, cache or executable code.', 'Resolve the application data directory through its runtime context, create an owned destination, and copy seed data only when initialization or migration requires it.', 'Route subsequent writes to the mutable copy and preserve existing data during upgrades.', 'Keep initialization failure distinct from an empty default store and retain recoverable originals during migration.'],
'Applies to ordinary packaged apps on HarmonyOS PC. Application and shell namespaces may expose different path prefixes.',
'Do not chmod installed resources, write next to argv[0], or assume reinstall preserves private data.',
'Exercise first install, populated-data upgrade, failed copy, interrupted initialization and restart; confirm writes target the runtime-resolved directory.',
[C+'problems/runtime-write-to-readonly-bundle-path.md',C+'semantic/application-sandbox-paths.md','problems/runtime-crash-sandbox-readonly-path.md'],['readonly','bundle','配置','database','sandbox'],['sandbox-paths-and-app-identity'])

common('application-path-namespace','Diagnose paths in the process that actually uses them','storage',
'A path visible through hdc shell is absent or inaccessible inside the application.',
'Resolve paths from the owning application context and inspect visibility in the actual process namespace.',
['Record which subject performs the operation: application, child process, shell or system service.', 'Resolve app bundle and data locations through that subject\'s platform API rather than importing shell absolute paths.', 'Pass resolved roots explicitly to native services and verify any child-process shared mount arrangement.', 'Separate not-found, denied access, loader rejection and missing native dependencies in diagnostics.'],
'Applies to sandboxed PC and other device classes; specific mount prefixes are implementation details, not portable constants.',
'A successful shell ls does not prove that application I/O or execution is permitted.',
'Test the same intended resource from the app and shell, then verify child-process access only where that route is used.',
[C+'semantic/application-sandbox-paths.md'],['namespace','hdc','mount','路径'],['sandbox-paths-and-app-identity','terminal-guest-working-directory'])

common('hap-executable-declaration','Diagnose EACCES when launching a packaged native executable','native',
'An ELF executable exists in the HAP but execve returns EACCES or chmod returns EPERM.',
'Validate executable packaging, platform signing authorization, process-visible path and ELF dependencies as separate requirements.',
['Check the target SDK module schema for executableBinaryPaths and extraction requirements; declare only the binaries actually executed.', 'Rebuild and sign with the correct application identity and authorized profile, then inspect the installed execution attributes.', 'Resolve the binary from the application namespace and verify architecture, interpreter and dynamic dependencies.', 'Test launch in a clean installation or isolated test identity without removing valuable application data. Use documented runtime embedding when subprocess execution is unsuitable.'],
'Executable packaging and permission policy depend on the SDK/product. A normal .so loaded by dlopen is not automatically an executableBinaryPaths entry.',
'Do not chmod the installed bundle or assume a generic signing command grants every permission. Do not equate shell execution with app execution.',
'Check declaration, signing and native loader diagnostics independently; run the intended entrypoint with resources and child-process cleanup.',
[C+'problems/executable-binary-eacces.md',C+'semantic/native-runtime-embedding.md'],['execve','EACCES','executableBinaryPaths','EPERM','执行权限'])

common('jit-executable-memory-authorization','Separate JIT memory authorization from ordinary allocation failure','native',
'A JIT runtime aborts while initializing executable memory or its code cache.',
'Confirm the allocation failure and verify both the declared capability and the actual signing/profile authorization.',
['Locate the failing executable-memory allocation in the native crash log; distinguish it from ordinary memory exhaustion.', 'Where required by the selected product, check ohos.permission.kernel.ALLOW_WRITABLE_CODE_MEMORY and the matching profile ACL.', 'Verify that the application type and certificate are eligible; a manifest permission name alone is insufficient.', 'If authorization is unavailable, evaluate a runtime-supported interpreter or non-JIT mode and measure its changed functionality/performance.'],
'Applies to runtimes requiring writable/executable memory under the target PC policy; importing this diagnosis does not establish an Electron or JVM port.',
'Do not treat all ENOMEM as a permission error, assume automatic signing grants system ACLs, or reuse unrelated profiles.',
'Inspect granted capability and crash logs, then test startup, code generation and shutdown using the intended release/debug signing path.',
[C+'problems/executable-memory-permission-denied.md',C+'semantic/privileged-system-capabilities.md'],['JIT','V8','HotSpot','CodeCache','ALLOW_WRITABLE_CODE_MEMORY','ACL'])

common('hap-minimum-api-compatibility','Resolve device API incompatibility without falsifying the minimum','build',
'HAP installation reports an older device SDK than the required compatibleSdkVersion.',
'Compare the device API with the real minimum required by the application and its native dependencies before changing the manifest.',
['Record device product/API and the selected compile, target and compatible SDK settings.', 'Determine the minimum API used by startup and optional native operations, including adapter code.', 'Lower a declared minimum only when older targets have a supported implementation or gated alternative; otherwise retain the requirement or use a suitable device.', 'Rebuild and sign the selected product, then test installation and startup on the lowest supported target.'],
'Use the exact selected SDK schema; deviceTypes such as 2in1 and an installable minimum do not prove PC feature compatibility.',
'Copying a lower compatibleSdkVersion can make installation succeed while startup still calls missing APIs.',
'Cover minimum-supported and older-rejected devices, plus capability-gated optional calls.',
[C+'problems/hap-device-sdk-version-mismatch.md','skills/qt-community-to-official/fix-build-error/build-fix-compatiblesdkversion-mismatch.md'],['9568297','compatibleSdkVersion','API','安装失败'])

common('hap-signing-identity','Keep product signing and application identity consistent','build',
'A HAP is unsigned, rejected, or launches under an unexpected bundle identity.',
'Align application/module identity with the selected product\'s signingConfig and authorized profile.',
['Use a stable bundleName accepted by the selected SDK schema and carry it into generated configuration and launch targets.', 'Check that the active product references its intended signingConfig; defining the config alone does not select it.', 'Verify profile identity, device eligibility and permission authorization without publishing credential material.', 'Inspect the actual output artifact, then separately verify installation, launch and required features.'],
'Applies to HAP-based integrations across frameworks. Changing bundle identity may create a different storage owner and require an explicit data migration.',
'A signed artifact is not automatically authorized for every device or system permission; changing bundleName is not a storage migration.',
'Inspect selected product identity/signature and verify the installed app\'s identity and data location.',
[C+'semantic/hap-application-identity.md',C+'semantic/hap-native-project-structure.md'],['bundleName','signingConfig','签名','profile'],['sandbox-paths-and-app-identity'])

common('native-dependency-closure','Package the complete native dependency closure','native',
'A packaged native library links successfully but fails to load because a versioned SONAME or indirect dependency is absent.',
'Trace NEEDED entries and required exports recursively, then package supported ABI-compatible dependencies using the names the loader requests.',
['Inspect ELF machine, SONAME, NEEDED and unresolved symbols with the selected toolchain utilities.', 'Resolve every dependency to an allowed system API or a redistributable matching native artifact. System-wide existence does not establish app-namespace visibility.', 'Preserve required versioned names through packaging; verify actual HAP contents rather than assuming symlinks or copy globs survive.', 'Rebuild unavailable dependencies or disable the feature requiring them. For static linkage into shared libraries, verify PIC and licensing.', 'Test optional codecs and late-used symbols as well as startup.'],
'Applies to native libraries consumed by any framework; module directory conventions and library versions remain backend-specific.',
'Empty stubs can have incompatible signatures and silently remove functionality. Pulling a device system library is not a portable general packaging solution.',
'Load from a clean package and exercise each enabled feature; inspect the installed dependency chain and diagnostics.',
[C+'semantic/ohos-native-third-party-libraries.md','problems/runtime-ffmpeg-versioned-soname-transitive-deps.md'],['SONAME','NEEDED','FFmpeg','symbol not found','传递依赖'],['ffi-library-and-symbol-availability','native-code-load-location'])

common('ohos-native-toolchain-inputs','Keep native target inputs separate from host build tools','build',
'Cross compilation finds host libraries or produces a binary for the wrong platform.',
'Use a coherent OH target compiler/sysroot/runtime and resolve host generators separately from target libraries.',
['Select the SDK-provided OH toolchain and target ABI/API intentionally.', 'Record the actual compiler, linker and sysroot used by configure probes and final compile/link commands.', 'Build code generators for the host; build shipped libraries for the target. Check CMake package discoveries for accidental host paths.', 'Inspect resulting ELF architecture, RPATH, exported ABI and dependencies before integrating it into the framework package.'],
'The exact CMake/toolchain variable names belong to the selected SDK. Android or desktop Linux binaries are not interchangeable with OH artifacts.',
'Arm64 architecture alone does not prove the OS ABI or runtime compatibility.',
'Rebuild with clean declared inputs, inspect tool commands and final ELF metadata, then initialize the library in the target app.',
[C+'semantic/ohos-native-third-party-libraries.md','procedural/llvm-ohos-cross-compile.md'],['NDK','sysroot','Clang','cross compile','交叉编译'],['native-dependency-completeness'])

common('musl-portable-libc-boundaries','Replace glibc-private names at the native ABI boundary','native',
'OH native compilation rejects __pid_t, __uid_t or a glibc-only in6_addr member.',
'Use public POSIX types and explicit byte-layout handling instead of reproducing glibc internals.',
['Replace private typedef references with pid_t/uid_t and include their defining public headers explicitly.', 'For IPv6 data, use the portable byte representation and validate the input and endianness; preserve the intended wire/parser semantics.', 'Keep platform restrictions separate from compilation: an existing POSIX symbol can still fail under the application sandbox.', 'Limit compatibility changes to diagnosed symbols and test both original and OH targets.'],
'Applies to C/C++ native dependencies built against an OH musl-based SDK; exact extension availability must be checked in that SDK.',
'Do not add a reverse shim depending on the missing private type or blindly copy integer bytes without checking parser length and byte order.',
'Compile public-header probes and verify IPv6 byte sequences and formatting on the intended architectures.',
['problems/build-fail-glibc-internal-symbols-ohos-musl.md'],['musl','glibc','__pid_t','__uid_t','s6_addr32'])

common('sysroot-header-search-order','Diagnose C++ header failures from injected sysroot include order','build',
'C++ standard headers fail with nullptr_t or math overload diagnostics in one OH build configuration.',
'Compare the actual failing include search order with a minimal toolchain build before adding type or header shims.',
['Capture the full compile command and include trace for a minimal standard-library source.', 'Compare explicit -isystem sysroot/usr/include injection with the toolchain\'s normal --sysroot ordering.', 'If the injected path reproduces shadowing, correct the build rule that adds it and regenerate build files.', 'Retest full targets; if host compiler, SDK and build route changed together, record a working combination without claiming which one caused the fix.'],
'This is a diagnosis for a matching include-order failure, not a universal OH compiler flag recommendation. The original evidence included an isolated probe and a multi-variable workaround.',
'Hand-editing generated Ninja files is not a durable fix. Removing unrelated include paths or changing several toolchains at once does not isolate a cause.',
'Compare minimal probe and full build commands after regeneration; verify no undesired include injection returns.',
['problems/build-fail-qt6-libcpp-musl-isystem.md'],['nullptr_t','__promote','isystem','sysroot','libc++'])

common('native-packaging-single-producer','Give each packaged native library one build producer','build',
'Hvigor ProcessLibs reports duplicate .so files from native intermediates and the manual libs input.',
'Choose either integrated native build output or an explicit prebuilt input for each logical library.',
['Trace both conflicting files to their producers and compare ABI/version/hash.', 'Keep the intended producer and remove only the redundant generated/copy rule, preserving separately supplied runtime dependencies.', 'Check ABI filters against every shipped dependency rather than enabling unsupported architectures from a template.', 'Rebuild the package and inspect that each intended library appears once.'],
'Applies to projects using Hvigor native integration; exact packaging paths and dependency deployment are framework-specific.',
'Do not delete the only valid prebuilt library or resolve a duplicate by packaging two different versions under arbitrary names.',
'Reproduce the conflict, regenerate from clean derived output and verify one consistent library per ABI.',
['problems/build-fail-duplicate-so.md'],['00306049','Duplicated files','ProcessLibs','abiFilters'])

common('windows-build-environment','Keep Windows build subprocess paths in the expected shell format','build',
'A Windows native build loses backslashes in tool paths or Hvigor cannot spawn Java despite an installed JBR.',
'Inspect the shell and environment actually inherited by the failing subprocess and use a compatible native command environment.',
['For make-generated Windows commands, check whether an unintended sh consumes backslashes; use the generator\'s supported Windows shell or regenerate correctly quoted commands.', 'For packaging Java, verify executable lookup from the same Windows process environment that runs Node/Hvigor, including the PATH separator convention.', 'Retain a complete JBR installation; setting JAVA_HOME alone may not fix a tool that searches PATH.', 'Rerun the failing stage and inspect its selected executable before changing source code.'],
'Applies to Windows build hosts; it does not require all OH builds to use one shell. MinGW make quoting and Windows process PATH are distinct diagnostics.',
'Do not replace all backslashes in a generated Makefile or infer a general Node spawn limitation from one PATH conversion failure.',
'Validate compiler and Java lookup in the chosen host environment and complete native compilation plus packaging.',
['problems/build-fail-bash-path-escape.md','problems/build-fail-hvigor-spawn-java-enoent.md'],['spawn java ENOENT','Git Bash','PATH','mingw32-make'])

common('hvigor-native-rebuild-evidence','Verify that a requested native rebuild actually ran','build',
'Hvigor finishes quickly but packages stale native libraries or skips the intended native build.',
'Confirm toolchain inputs, task execution and artifact hashes before treating a successful packaging command as a native rebuild.',
['Inspect selected SDK/JBR and externalNativeOptions configuration in the actual build environment.', 'Compare native task logs and output hashes with the edited source or intended prebuilt input.', 'If stale derived state is demonstrated, regenerate only the relevant build output after preserving source, user data and signing configuration.', 'Verify the new artifact is included in the final package and launched.'],
'Applies to Hvigor-driven framework hosts; CLI and IDE success need equivalent inputs and output checks.',
'Do not copy only java.exe out of its runtime, blindly clear source directories, or conclude CLI packaging is universally broken from one failing setup.',
'Use a traceable native change, observe compilation and final packaged hash, then verify launch with the rebuilt artifact.',
['problems/build-hvigor-native-skip.md','problems/deploy-fail-hvigor-cli-hap-crash.md'],['BuildNativeWithNinja','stale','Hvigor','native skip'])

common('ohpm-network-diagnosis','Diagnose OHPM registry failures before changing dependencies','build',
'ohpm install times out or cannot reach its registry.',
'Check the configured client, registry and network/proxy path before altering lockfiles or dependency versions.',
['Record the actual OHPM executable/version and active registry.', 'Differentiate DNS, TLS, proxy and temporary endpoint failures using the same environment as the build.', 'Use an approved reachable registry or repair the network configuration without baking a temporary mirror into framework code.', 'Read back configuration and repeat dependency installation, preserving the intended lock.'],
'Applies to any generated or handwritten OH native host using OHPM.',
'Deleting lockfiles or repeated cache clearing does not fix a registry network failure.',
'Verify retrieval in the actual build environment and confirm the resolved dependency identities remain intentional.',
[C+'problems/ohpm-install-timeout.md'],['ohpm','registry','timeout','代理'])

common('system-metric-unavailability','Represent unavailable system metrics separately from zero','integration',
'A Linux-oriented system-information library returns empty CPU, disk or process statistics on OH.',
'Check each metric\'s real data source under app privileges and use an explicit unavailable result or supported native backend.',
['Map each metric to its procfs or native API source.', 'Inspect access and parsing in the application process, not a privileged shell.', 'Distinguish denial, unimplemented API, parser failure, empty collection and true zero.', 'Choose a product-supported API or visible feature degradation; review target cfg that selected a desktop Linux backend.'],
'Access depends on device/API/application privileges. A source report about one procfs file is not a permanent availability table.',
'Do not fabricate zero or assume one readable metric proves every metric is available.',
'Exercise each required metric with known workload changes and with denied/unavailable data sources.',
[C+'semantic/system-information-access-limits.md',C+'problems/procfs-system-information-unavailable.md'],['procfs','sysinfo','CPU','unavailable','系统信息'])

common('privileged-capability-gating','Check product and signing eligibility before promising a system capability','integration',
'An SDK exposes an API or permission, but an ordinary application cannot invoke the feature.',
'Gate the capability on API level, product scope, application type, actual grants and device support.',
['Inspect the target SDK declaration and product restrictions, including deprecated or system-only status.', 'Check whether the application can request the permission and whether its signed profile actually grants it.', 'Query the device capability and keep rejection distinct from an empty successful result.', 'Provide a supported chooser/settings/user-mediated alternative or mark the feature unavailable.'],
'For example, an API for programmatic hotspot management can be restricted to particular non-general products; verify the intended PC policy rather than promising access from an API name.',
'A stub, permission string or Linux backend does not grant privileged capability.',
'Test eligible and denied configurations and verify the UI follows the actual capability result.',
[C+'semantic/privileged-system-capabilities.md',C+'problems/programmatic-hotspot-unavailable.md'],['hotspot','system permission','ACL','热点','权限'],['capability-gated-backend-initialization'])

common('native-surface-owner','Give native surfaces one lifetime owner','window',
'A surface handle becomes invalid during resize/recreation, or mixing surface APIs causes creation failure.',
'Choose a coherent surface-management API and stop renderer use before its owning node or surface is destroyed.',
['Map node mount, surface creation/change/destruction and renderer submission states explicitly.', 'Retain handles only for their valid generation and reject callbacks from older surfaces.', 'Before switching API families, inventory input, focus and accessibility dependencies as well as rendering.', 'Use a minimal native reproduction to establish compatibility between OH_NativeXComponent and Node API SurfaceHolder routes.'],
'Applies to native rendering adapters. The reported mixed-API failure does not establish a universal mutual-exclusion boundary; precise coexistence remains unresolved.',
'Do not register two unknown surface owners or remove input/accessibility dependencies merely to make surface creation succeed.',
'Cover mount/unmount, resize, hide/show, recreation, mouse/touch/key input and accessibility. Treat the coexistence hypothesis as unverified until measured.',
[C+'semantic/arkui-window-xcomponent-model.md','problems/surfaceholder-nativexcomponent-incompatibility.md'],['SurfaceHolder','XComponent','surface','opaque'],status='unresolved')

common('window-observed-state','Separate requested visibility from focus and drawable surface state','window',
'A PC window is brought forward but the framework still thinks it is hidden or has no drawable surface.',
'Reconcile platform window status, focus, framework visibility and surface readiness independently.',
['Capture the requested transition and the current native window generation.', 'Observe actual window status/exposure/focus events rather than a single show/hide callback.', 'Submit rendering only when the current surface is valid and ready; update framework state from appropriate native observations.', 'Cancel pending transition work on destruction and report timeout as unknown/incomplete rather than completed visibility.'],
'Applies to asynchronous PC window adapters. Callback names and resynchronization hooks belong to the selected framework/backend.',
'A window object, successful command or focus event does not prove every visibility/render state is synchronized.',
'Test task switching, preload/restore, minimize/recover, close/reopen and surface recreation while comparing native and framework state.',
[C+'semantic/arkui-window-xcomponent-model.md',G],['focus','visibility','white screen','窗口','WMS'])

common('rust-ohos-target-cfg','Distinguish OH target environment from desktop Linux cfg','native',
'A Rust native dependency selects desktop integration for an OH target.',
'Inspect target_os, target_env and framework-injected cfg separately and gate by the operation\'s actual platform semantics.',
['Inspect rustc target cfg for the selected target, including target_env = "ohos" where applicable.', 'Review branches guarded only by Linux or desktop and identify the native operation they enable.', 'Exclude OH from desktop-only implementations or select an explicit OH adapter, without mechanically rewriting every Linux branch.', 'Build each supported target and confirm runtime backend selection.'],
'Common native-layer knowledge for applications embedding Rust; not evidence that every UI framework supplies an OH Rust bridge.',
'Framework desktop/mobile flags and compiler target attributes are not interchangeable.',
'Compile cfg probes and exercise the selected storage/system backend on the actual OH application.',
[C+'semantic/ohos-cross-compile-pitfalls.md'],['Rust','target_env','target_os','cfg','linux'])

common('ffi-c-char-target-abi','Use the target c_char type in native bindings','native',
'A native binding fails on OH because it hard-codes signed character pointers.',
'Model C ABI types with the toolchain\'s c_char definitions and regenerate or narrowly repair mismatched bindings.',
['Confirm the selected target\'s c_char signedness and pointer/function ABI.', 'Use std::os::raw::c_char or the matching libc/generated C type at the boundary.', 'Keep encoding and byte interpretation explicit when crossing to strings or arbitrary binary buffers.', 'Patch only the affected dependency boundary or disable an unused dependent feature, then test original and OH targets.'],
'Applies to native Rust/C interop where target signedness differs; c_char is not guaranteed signed or unsigned across all targets.',
'A pointer cast can hide a type error without proving the called ABI or string encoding is correct.',
'Cross-compile bindings and round-trip ASCII, non-ASCII and bytes above 127 with correct ownership.',
[C+'semantic/ohos-cross-compile-pitfalls.md'],['c_char','u8','i8','E0308','ABI'])

qatom('qt-linux-branch-selection','Keep OH out of unintended desktop Linux branches','architecture',
'Qt compilation or runtime enters a Linux desktop branch that assumes desktop services.',
'Handle Q_OS_OHOS deliberately before broad Linux guards and exclude OH from incompatible qmake unix branches.',
['Review Q_OS_LINUX/Q_OS_UNIX guards and the operations they select, because the OH Qt build may also define Linux.', 'Add an OH branch where behavior differs; keep genuinely portable POSIX logic shared.', 'For desktop-only qmake scopes, exclude ohos alongside other excluded targets.', 'Do not infer native backend availability from a PC layout or QML OS label.'],
'Applies to the selected Qt OH fork; check actual macros and QML platform reporting for that version.',
'Do not turn every Linux feature off or assume Qt.platform.os returns ohos.',
'Compile original and OH targets and exercise the affected platform branch.',[G,'semantic/qt-harmonyos-porting-workflow.md'],['Q_OS_OHOS','Q_OS_LINUX','qmake','unix'])

qatom('qt-qpa-runtime-template-coherence','Align Qt runtime, QPA and ArkTS startup templates','build',
'Startup crashes with missing modules/modulesFactories, deviceInfo or undefined QPA handlers after replacing one native artifact.',
'Use a compatible runtime/QPA/template set and inspect its actual startup contract before changing property names.',
['Identify exact QtCore, libqohos and ArkTS template artifacts and compare their exported/required symbols.', 'Inspect which setup object the installed QPA reads: modules or modulesFactories, and whether values are objects or factories.', 'Align the whole template and its module exports/import timing with that implementation, preserving app resources, configuration and signing inputs.', 'Check APP_LIBRARY_NAME against the packaged native artifact; rebuild/repackage all changed components and inspect final hashes.'],
'Qt branch/version labels alone are insufficient; different snapshots can expect opposite setup forms. Qt 5 and Qt 6 integration APIs differ.',
'Do not blindly rename modules to modulesFactories or replace one libqohos against an unrelated QtCore.',
'Test a clean launch, warm Want delivery and module-backed features; inspect native fault logs as well as process survival.',
['problems/runtime-crash-arkts-template-modules-mismatch.md','problems/runtime-crash-libqohos-modules-mismatch.md','problems/runtime-crash-libqohos-libqt5core-version-mismatch.md','problems/runtime-crash-ohos-deviceinfo-global-expression.md'],['modulesFactories','modules','deviceInfo','libqohos','NAPI'])

qatom('qt-cmake-package-discovery','Find target Qt packages without importing host libraries','build',
'CMake cannot find Qt or a native dependency outside the OH toolchain root.',
'Inspect root-path search policy and explicitly select the target package prefix before relaxing package lookup.',
['Confirm the package config and library belong to the OH target SDK.', 'Inspect CMAKE_FIND_ROOT_PATH_MODE_PACKAGE and the actual search trace.', 'Where the SDK layout requires it, use a scoped BOTH policy and explicit target prefixes before find_package.', 'Inspect every imported target path to ensure the relaxed search did not select host libraries.'],
'Applies to Qt/CMake OH integrations with externally installed target packages; a blanket BOTH rule is unsafe without path verification.',
'Finding a package is not proof it is built for OH.',
'Configure from clean inputs and inspect imported targets plus final ELF architecture.',
[G,'semantic/qt-harmonyos-third-party-libs.md'],['find_package','CMAKE_FIND_ROOT_PATH_MODE_PACKAGE','CMAKE_PREFIX_PATH'])

qatom('qt-plugin-deployment-layout','Package Qt plugins in the loader layout expected by the selected SDK','build',
'The app cannot load QPA, QSQLITE, image formats or the OH style despite packaging QtCore.',
'Deploy the selected Qt runtime and categorized plugins with their dependency closure and expected search layout.',
['Link/integrate the selected QOhosPlatformIntegrationPlugin target where that SDK exposes it and inspect libqohos in the HAP.', 'Determine the SDK\'s actual SQL/style/image plugin filenames and copy/integrate them into the loader\'s category directories when automatic deployment does not.', 'Inspect plugin dependencies, including DBus where the selected QPA requires it.', 'Diagnose QSqlDatabase driver availability separately from database-path writability; keep native runtime versions coherent.'],
'Qt 5 source examples use styles, sqldrivers and imageformats directories; exact names and deployment helpers must match the selected Qt generation.',
'Do not assume desktop plugin filenames, that linking QtSql deploys its driver, or that a missing QPA dependency is an application-code crash.',
'Inspect package contents and runtime plugin diagnostics; enumerate SQL drivers, open a writable database and exercise required image/style plugins.',
['problems/runtime-fail-qpa-plugin-not-found.md','problems/runtime-fail-libqtdbus-missing.md','problems/runtime-fail-sqlite-open-database.md',G],['QPA','QSQLITE','Driver not loaded','libqsqlite.so','sqldrivers','styles'],['native-dependency-closure','bundle-data-write-location'])

qatom('qt-ohosextras-version-boundary','Resolve QtOhosExtras against the actual Qt generation','build',
'QtOhosExtras package discovery, headers or enums fail after changing the Qt OH SDK.',
'Inspect installed headers and build metadata instead of carrying Qt 5 public Extras recipes into Qt 6.',
['For a Qt 5 package lacking generated CMake configs, use its supported qmake integration or an explicitly verified manual link contract.', 'Use installed header names; some distributions expose lowercase qohos headers without CamelCase forwarding headers.', 'Qualify scoped enums and locate the owning class/free function in actual headers.', 'For Qt 6, locate the equivalent integration API and its public/private status; version-pin private APIs if unavoidable.'],
'The imported examples concern particular Qt 5.12/5.15 and Qt 6 OH distributions; module organization is not universal.',
'Do not invent a CMake target/library suffix or copy a Qt 5 close enum into Qt 6.',
'Compile a minimal use against the installed headers, inspect the linked artifact and test the requested native capability.',
['problems/build-fail-cmake-ohosextras.md','problems/build-fail-closeeventrootcause-path.md','problems/build-fail-qtohosextras-camelcase-headers.md',G],['QtOhosExtras','qmake','CloseEventRootCause','CamelCase'])

qatom('qt-dialog-window-parenting','Establish native dialog ownership before creating its window','window',
'A Qt dialog appears as another task or loses close/wheel input while a sibling modal dialog is active.',
'Give dialogs a deliberate Qt/transient parent and, where supported, tag the native subwindow relationship before native creation.',
['Select the intended owning top-level window and assign the dialog parent where feasible.', 'Use the selected OH tagging API before show or winId creates the native window.', 'If a QPA synthetic parent exists only natively, inspect whether the matching Qt transient-parent chain is missing.', 'Prefer an app-level ownership repair; scope any QPA setTransientParent fix to the diagnosed fallback and avoid self-parenting.'],
'Qt parentage, native subwindow ownership and system-task identity are separate concepts. QPA behavior differs by version.',
'Tagging after winId or show can be too late. Disabling modal blocking globally can introduce incorrect input routing.',
'Test nested dialogs, modal siblings, close, scroll, task-switch entries and owner destruction.',
['problems/runtime-blocked-by-modal-window-no-parent.md',G,'semantic/qt-harmonyos-window-model.md'],['QDialog','transientParent','blockedByModalWindow','tagWindowOrWidgetAsSubWindowOf'])

qatom('qt-close-event-origin','Handle Qt close events according to their native origin','lifecycle',
'A save prompt stalls termination or an ordinary window close is handled like system shutdown.',
'Distinguish interactive window closing from ability termination and persist incrementally instead of relying on shutdown UI.',
['Inspect the selected Qt OH close-cause API and map its native origins.', 'In Qt 5 variants with Extras, use the actual scoped CloseEventRootCause values and the getCloseEventRootCause free function.', 'Allow interactive save/cancel only on the supported user-close path; use bounded noninteractive persistence for termination callbacks.', 'For Qt 6, inspect changed/private close APIs rather than copying Qt 5 enum names.'],
'The OS may terminate a process without a useful final callback. Forced termination cannot be promised as interceptable.',
'Do not show a blocking dialog in ability shutdown or assume all close signals mean the user clicked a window button.',
'Test window close, task removal, termination and restart after unsaved edits.',
[G,'semantic/qt-harmonyos-lifecycle.md'],['closeEvent','AbilityClose','WindowStageClose','OnPrepareToTerminate'],['foreground-recovery-and-save-lifetime'])

qatom('qt-resource-registration-linkage','Keep Qt resource initialization compatible with the exported entrypoint','build',
'Linking an OH shared-library entry shim reports an undefined qInitResources symbol.',
'Compare C/C++ linkage and resource inclusion; keep explicit initialization in a C++ helper when it is required.',
['Inspect generated resource object symbols and the reference emitted by the exported entry shim.', 'Ensure qrc inputs are compiled and linked, using the appropriate AUTORCC or Qt resource helper.', 'If explicit initialization is needed, call Q_INIT_RESOURCE from a C++-linkage helper outside the C-exported entry definition.', 'Omit redundant explicit initialization only after confirming resource objects and their registration actually survive the link.'],
'Applies to Qt apps loaded as native shared libraries. Static archives and linker retention can require explicit resource references.',
'Deleting Q_INIT_RESOURCE blindly can trade a link error for missing icons/QML/translations.',
'Inspect symbols and test access to representative resources in the packaged app.',
['problems/build-fail-qinitresource-extern-c-main.md',G],['Q_INIT_RESOURCE','qInitResources','AUTORCC','extern C'])

qatom('qt-native-entry-initialization','Preserve application initialization when adapting the native entrypoint','architecture',
'A minimal OH entry shim launches a window with missing domain objects and crashes immediately.',
'Preserve the original application initialization contract before constructing its UI.',
['Trace required configuration, services and model objects from the original entrypoint to the initial window.', 'Adapt only the platform entry/export boundary while retaining valid initialization order and error handling.', 'Do not replace unavailable dependencies with null objects unless the consumer explicitly supports that state.', 'Fail with a visible recoverable initialization result when a required service cannot be created.'],
'Applies to Qt entrypoint migration; a placeholder window is not the original application running successfully.',
'A non-null QApplication and a constructed main window do not establish a valid domain model.',
'Test first launch, invalid configuration and normal startup through the real application entrypoint.',
['problems/runtime-crash-null-pointer-entry-shim.md'],['entry shim','nullptr','startup','SIGSEGV'])

qatom('qt-native-file-dialog-contract','Verify the selected QFileDialog backend before replacing it','storage',
'A migration disables QFileDialog or assumes a returned path is a completed user-visible export.',
'Try the selected QPA\'s implemented dialog route and validate result access and visibility separately.',
['Confirm the Qt/QPA/template combination exposes the required open/save dialog behavior.', 'Preserve cancellation and validate whether the result is a usable authorized file path or requires adapter processing.', 'For save, complete the content write and confirm whether the destination is user-selected/public or app-private.', 'Introduce a narrow native adapter only for behavior the actual Qt backend does not provide.'],
'Historical Qt backend examples differ in public-document behavior. A functioning dialog is not proof of persistent URI grants or file-manager visibility.',
'Do not replace a working picker with hard-coded sandbox paths or present a private save as public export.',
'Test open/save/cancel, inaccessible destinations and reopening the saved file from the intended user workflow.',
['problems/ohos-qfiledialog-native-support.md'],['QFileDialog','save','文件选择'],['document-picker-uri-access','file-export-contract'])

qatom('qt-picker-initial-uri','Pass only supported initial locations to the Qt OH document picker','storage',
'The system picker reports it cannot open the initial application-private directory.',
'Leave the initial location unspecified unless the adapter can supply a supported user-document URI.',
['Inspect the caller\'s initial directory and the QPA value mapped to defaultFilePathUri.', 'Do not convert an arbitrary private sandbox path into a presumed selectable document URI.', 'Omit the native field for an empty/default initial location and validate nonempty user-document inputs.', 'Deploy both caller and QPA changes when both contribute to the faulty value.'],
'Applies to the Qt OH backend exposing DocumentViewPicker initialization; separate from authorization of the final selected file.',
'Requesting another permission does not repair an invalid initial URI.',
'Test no initial location, valid user-document location, private path and cancellation.',
['problems/runtime-fail-file-dialog-invalid-default-uri.md'],['defaultFilePathUri','QFileDialog','initial directory'],['document-picker-uri-access'])

qatom('qt-picker-suffix-filter-mapping','Translate Qt file filters without inventing native glob support','storage',
'An extensionless executable or other valid file disappears from an OH QFileDialog.',
'Translate only representable suffix filters and validate filename/content constraints after selection.',
['Inspect the Qt filename/glob filter sent through the QPA adapter.', 'Convert compatible *.extension patterns into the native suffix form required by the SDK.', 'Omit fileSuffixFilters when no supported suffix remains; do not pass executable names such as qmake* as suffixes.', 'Perform application-specific validation after a successful selection.'],
'Applies to DocumentViewPicker-backed Qt dialogs; suffix filtering is not an arbitrary filename-glob language.',
'A picker filter is not content validation or proof the selected executable is compatible.',
'Test extensionless names, multiple valid suffixes, unsupported patterns and cancellation.',
['problems/runtime-fail-file-dialog-invalid-default-uri.md'],['fileSuffixFilters','qmake','glob','QFileDialog'])

qatom('qt-drag-data-readiness','Read dragged content at the supported drop stage','input',
'A Qt OH drop target can see MIME types but reads empty data during drag enter or movement.',
'Use early drag events to negotiate accepted types and defer content consumption to the supported drop completion.',
['Inspect available MIME descriptors during enter/move and choose an accepted action.', 'Read actual content during drop where the backend guarantees readiness.', 'Apply URI/access handling when the drop contains files rather than copying path text blindly.', 'Reject unsupported content explicitly and preserve cancellation.'],
'Applies to the selected Qt OH asynchronous drag backend; it is not a universal timing rule for every platform plugin.',
'Early MIME visibility is not proof that native payload transfer has completed.',
'Test text/files, cancel, unsupported types and delayed transfer.',[G,'semantic/qt-harmonyos-code-patterns.md'],['dropEvent','dragEnterEvent','MIME','拖放'],['document-picker-uri-access'])

qatom('qt-qml-dark-palette','Fix indistinguishable QML controls by inspecting palette roles','ui',
'Qt QML controls render text but dark backgrounds merge into one black area.',
'Inspect effective palette roles and apply a deliberate app palette only when the native mapping is unsuitable.',
['Differentiate missing QML/resources from a palette problem by checking visible text and component geometry.', 'Inspect QPalette Base, Button, Window and text roles under light/dark modes.', 'Choose an explicit app palette with adequate contrast, or implement a supported follow-system update path.', 'Keep the fixed-theme tradeoff visible; do not claim a one-time override follows later OS theme changes.'],
'The source observation concerns a particular OH QPA palette. Check the installed backend and accessibility/theme policy before overriding.',
'Forcing dark colors unconditionally changes light-mode behavior and can hide system preferences.',
'Test light/dark changes, disabled controls, focus/hover and larger text on the target display.',
['problems/runtime-style-dark-palette-pure-black.md'],['QPalette','SystemPalette','Base','dark theme','纯黑'])

qatom('qt-opengl-texture-completeness','Check texture completeness before treating black RGBA output as a decoder failure','rendering',
'Qt3D RGBA textures appear black on a particular OH ZINK/OpenGL backend.',
'Inspect mipmap generation, sampling filters and texture completeness before changing image format or blending policy.',
['Reduce to a known RGBA image and inspect texture levels/filter configuration.', 'Compare a non-mipmapped compatible minification filter or fully supplied mip chain under the same backend.', 'Separate alpha-blending correctness from missing texture levels; avoid assuming RGB conversion fixes the cause.', 'Scope the workaround to the measured backend and retain other platforms\' intended rendering path.'],
'Applies to the diagnosed Qt3D/ZINK configuration; not a claim that OH generally cannot render PNG alpha.',
'QML platform names may report Linux; a guard for ohos alone may never execute. Blending and texture completeness are different fixes.',
'Test RGBA/RGB inputs, scaling/minification, alpha blending and release builds.',
['problems/render-black-rgba8-zink-mipmap.md'],['Qt3D','ZINK','RGBA8','mipmap','black texture'])

qatom('qt-application-thread-initialization','Initialize Qt objects on the thread that owns its event loop','lifecycle',
'QCoreApplication::notify asserts that the application main thread does not match the current thread.',
'Trace early Qt object initialization and keep Qt application construction aligned with the OH adapter\'s designated event-loop thread.',
['Identify the ArkTS/JS thread and Qt event-loop thread in the selected integration.', 'Inspect static/global initializers and callbacks that touch QThread or QObject before QApplication exists.', 'Move premature Qt initialization into the owning Qt startup phase, and marshal cross-thread operations through supported gateways.', 'Preserve the adapter\'s shutdown ordering rather than resetting private main-thread state to hide the assertion.'],
'Applies to Qt OH dual-thread integration; exact startup callbacks and private state differ by version.',
'A JS callback running on the UI thread is not automatically running on the Qt application thread.',
'Observe thread identities through cold startup, callback delivery and shutdown; exercise the previously failing path.',
['problems/ohos-mainthread-mismatch.md'],['theMainThread','QCoreApplication::notify','QThread','ArkTS'])

# Attach independent Qt evidence to already-shared lessons rather than duplicate them.
for ident,paths in {
 'sandbox-paths-and-app-identity':[C+'semantic/application-sandbox-paths.md'],
 'persistent-file-access-grants':[C+'semantic/document-picker-uri-access.md'],
 'incoming-wants-before-runtime-ready':[C+'semantic/stage-uiability-lifecycle.md'],
 'native-dependency-completeness':[C+'semantic/ohos-native-third-party-libraries.md'],
}.items():
    for path in paths:
        PROVS[ident]['sources'].append(source('qt',path));MAP.setdefault(('qt',path),[]).append(ident)
    PROVS[ident]['transformations'].append('Merged matching Qt-collection platform evidence without copying its orchestration policies.')

qatom('qt-accessibility-bridge-activation','Enable and verify the Qt native accessibility bridge','accessibility',
'An ArkUI inspector sees only an XComponent and automation cannot find Qt controls.',
'Confirm the QPA contains its accessibility bridge, activate the supported launch option and query through the native accessibility consumer.',
['Inspect the selected build configuration for QT_NO_ACCESSIBILITY and the actual QPA bridge; binary strings alone are only a diagnostic clue.', 'For versions exposing it, pass the Boolean Want parameter io.qt.experimental.enableA11ySupport before the QPA initializes.', 'Restart the intended process and query its native accessibility tree with a supported consumer such as uitest dumpLayout.', 'Check roles, names, bounds, focus and supported actions for actual Qt controls; resolve current node identities after recreation.'],
'Applies to Qt OH builds that implement this experimental launch option. A launch flag cannot enable bridge code omitted from the binary.',
'An ArkUI inspector tree, a compiled symbol or a running system screen reader does not prove the application bridge publishes controls.',
'Compare native control trees before/after activation and exercise focus/actions after resize and window recreation. Record accessibility separately from build/install.',
['semantic/qt-harmonyos-accessibility.md'],['QAccessible','enableA11ySupport','uitest','XComponent','无障碍'])

common('napi-thread-and-handle-lifetime','Keep Node-API values on their owning environment and handle lifetime','native',
'Native callbacks crash when a JS value crosses threads or is reused in a later asynchronous turn.',
'Marshal JS work to the owning environment and pass ordinary native values across threads; retain JS objects with the supported persistent-reference mechanism when needed.',
['Identify the environment/thread owning each napi_value or framework wrapper.', 'Convert returned data to an owned native representation before leaving the JS callback/thread.', 'For later callbacks, create values in the callback\'s current environment or retain/release a valid persistent reference according to the runtime API.', 'Invalidate pending work at environment destruction and finish release while its owner remains valid.'],
'Common native Node-API integration rule; concrete dispatchers and reference wrappers depend on the selected OH runtime/framework.',
'Capturing a transient wrapper by value does not make its underlying JS handle persistent or cross-thread safe.',
'Exercise delayed callbacks, garbage collection, worker delivery and environment teardown; verify values and references remain valid only for their declared lifetime.',
['semantic/qt-ohos-js-thread-gateway.md'],['Node-API','napi_value','napi_ref','GC','线程'])

qatom('qt-js-thread-gateway','Dispatch Qt native calls through the owning JS gateway','native',
'Qt event-loop code needs an ArkTS/system API but cannot safely access Node-API objects directly.',
'Use the selected integration\'s JS gateway and return copied native values instead of JS handles.',
['Prefer a public Qt API when it implements the needed operation.', 'For a pinned Qt 5 private integration, inspect QOhosJsThreadGateway in qcore_ohos_p.h and its asynchronous/result-returning variants.', 'Supply matching private headers, node-addon-api include inputs and required native Node-API linkage where the SDK does not transitively provide them.', 'Keep JS-facing work inside the gateway callback and deliver ordinary values back to Qt; dispatch reverse-direction UI work to the Qt owner.'],
'The gateway is a private API with no general version/ABI promise. Qt 6 may reorganize it.',
'Do not use the backend registration interface as an application facade or return transient QNapi values from another thread.',
'Build a minimal call, verify thread identities and error propagation, and test delayed completion plus shutdown.',
['semantic/qt-ohos-js-thread-gateway.md'],['QOhosJsThreadGateway','CorePrivate','ace_napi.z','QNapi'],['napi-thread-and-handle-lifetime'])

common('async-bridge-shutdown-wait','Avoid waiting for a stopped event loop during shutdown','lifecycle',
'Shutdown hangs waiting for an asynchronous callback after the native JS environment stops processing work.',
'Complete cross-runtime work before teardown and use bounded cancellable waits only where the supplying event loop can progress.',
['Identify the queue/event loop that will settle the pending result.', 'Prefer asynchronous completion; never block that same queue or a UI thread waiting for itself.', 'If adapting a synchronous API on a worker, use a bounded wait and shared completion ownership that survives timeout without publishing stale results.', 'During teardown invalidate pending requests, avoid new work depending on a destroyed runtime and release native resources through their own supported stop protocol.'],
'Applies to bridges where the environment may stop pumping callbacks during destruction. A timeout must not destroy state still used by a late callback.',
'A nested event loop or unbounded future wait is not a reliable shutdown protocol; callback rejection and native resource release are separate.',
'Test unresolved promises, environment teardown, late callback after timeout and repeated shutdown without blocking the UI.',
['semantic/qt-ohos-js-thread-gateway.md','episodic/postmortems/quit-deadlock-tsfn.md'],['TSFN','deadlock','shutdown','Promise','appfreeze'],['native-event-loop-handle-teardown'])

qatom('qt-tray-effective-capability','Verify each Qt tray operation instead of trusting availability flags','integration',
'A Qt tray availability check succeeds but geometry, menu state or notification callbacks do not behave as expected.',
'Trace the selected QPA\'s implemented status-bar operations and limit UI promises to observed capabilities.',
['Check required ArkTS status-bar/display/notification module registration for the same QPA template generation.', 'Verify add/remove/update and click delivery independently; some historical availability/geometry methods returned constants.', 'Design icons for the backend\'s monochrome-mask behavior where applicable.', 'Validate each menu feature and notification result rather than assuming desktop QMenu or messageClicked parity.'],
'Imported behavior concerns a particular Qt 5 OH status-bar implementation; check API gates and actual device behavior for the chosen version.',
'A true supportsMessages flag or constant geometry is not proof of notification authorization or real icon bounds.',
'Test show/hide, icon change, menu activation, disabled/checkable requirements, tooltip gate and notification interaction separately.',
['semantic/qt-harmonyos-system-tray.md'],['QSystemTrayIcon','StatusBarExtensionKit','geometry','messageClicked','托盘'])

qatom('qt-optional-module-requirements','Separate missing Qt modules from unnecessary example dependencies','build',
'Qt package discovery fails on UiTools or falls back to the wrong Qt major version.',
'Inspect the installed target modules and determine whether the missing module is required by shipping code or only optional examples.',
['List actual target package configs and compare them with the requested components.', 'Trace QUiLoader or other affected API usage before removing a requirement.', 'Disable optional examples/features that alone require the module; if shipping behavior needs it, obtain/build a compatible module through a verified supported route.', 'Report the unresolved capability when the needed module is unavailable rather than silently selecting an older Qt major.'],
'Module presence depends on the Qt distribution and build route. Historical host-build limitations are not proof that every future cross-build is impossible.',
'A module marked supported upstream may still be absent from an installed SDK; deleting its link target does not replace runtime UI loading.',
'Configure core and optional targets independently and execute any retained module-backed feature.',
['problems/build-missing-qt5uitools-ohos.md'],['UiTools','QUiLoader','Qt4','find_package'])

qatom('qt-icu-sdk-contract','Match Qt ICU probes to the SDK headers and libraries','build',
'Qt configure reports ICU unavailable or compilation fails after forcing it on.',
'Check the SDK\'s real ICU headers, exported APIs and library layout before adjusting configure detection.',
['Compare the failing probe with actual merged or separate ICU library names in the selected SDK.', 'Confirm required headers and symbols exist, not just a link stub.', 'Prefer a compatible SDK/module combination; scope any probe patch to the verified layout.', 'If disabling unavailable ICU functionality, specify affected collation/time-zone behavior and validate the fallback.'],
'Applies to Qt OH builds with ICU integration; a historical SDK-version recommendation must be rechecked before selecting a current toolchain.',
'Force-enabling ICU or suppressing an include does not supply missing converter/resource APIs.',
'Run configure, compile affected modules and test representative collation, locale and time-zone operations.',
['problems/build-fail-qt515-icu-detection.md'],['ICU','ures.h','QCollator','QTimeZone'])

common('input-driven-io-coalescing','Coalesce expensive I/O triggered by text editing','ui',
'Every keystroke starts parsing or file I/O, causing stalls, races or stale results.',
'Use an owned restartable debounce or an explicit submit action, then validate input and correlate results with the current edit generation.',
['Cancel/restart one pending debounce when the input changes; choose its interval from interaction needs.', 'Validate the path/input and bound parsing before launching expensive work.', 'Move blocking work off the UI path where supported and ignore completion from superseded edits.', 'Check empty/short parse results before indexing and release operation resources on owner destruction.'],
'Common UI mechanism extracted from a Qt textChanged failure; a debounce does not prove the underlying parser or native library is crash-safe.',
'Multiple delayed callbacks with only string equality can still repeat work when input changes away and back. A speculative symbol-resolution theory is not a confirmed cause.',
'Test rapid typing, same-value revisits, invalid/large files, empty parsing and navigation during work.',
['problems/runtime-crash-loadfile-textchanged-no-debounce.md'],['debounce','textChanged','I/O','防抖'])

common('clang-template-depth-limit','Scope Clang nesting-limit changes to the failing target','build',
'A template expansion such as magic_enum exceeds the compiler bracket-depth limit.',
'Confirm a finite intended expansion and adjust its range or target-local nesting limit instead of changing unrelated build settings.',
['Read the first expansion diagnostic and identify the actual template range.', 'Reduce an unnecessarily broad reflection range where supported.', 'If the expansion is required, apply a bounded -fbracket-depth value to the affected Clang target only.', 'Measure compilation cost and keep the setting conditional on that compiler and requirement.'],
'Applies to native C++ dependencies regardless of UI framework; an example limit is not an OH operating-system restriction.',
'Do not raise every compiler limit globally or infer the number of declared enum constants solely from the scan range.',
'Compile the affected instantiation and test enum conversion boundaries and invalid inputs.',
['problems/build-fail-magic-enum-bracket-depth.md'],['magic_enum','fbracket-depth','Clang','fold expression'])

common('multimedia-timebase-api-migration','Preserve time-base semantics when replacing a removed media API','media',
'A native media dependency no longer declares the function used to read a codec time base.',
'Identify the time domain consumed by the caller and choose a replacement with equivalent units and meaning.',
['Inspect the installed media headers and the intended meaning of the removed value at each call site.', 'Distinguish stream timestamps from codec timing; use AVStream.time_base only when the operation requires the stream time base.', 'Convert timestamp units explicitly where a change of domain is required.', 'Version-scope compatibility code and preserve original behavior on still-supported library generations.'],
'Applies to native FFmpeg consumers across frameworks. A source report of a removed function does not establish a universal replacement or current version boundary.',
'Do not assume stream and codec time bases are interchangeable merely because a replacement compiles.',
'Test representative audio/video timestamp conversion, variable frame rate, seeking and reported durations against known media.',
['problems/build-fail-ffmpeg9-removed-api.md'],['FFmpeg','av_stream_get_codec_timebase','AVStream','time_base'])

common('native-uuid-dependency-contract','Replace a missing UUID dependency without changing identifier semantics','native',
'Native compilation cannot find uuid/uuid.h or link libuuid in the selected OH SDK.',
'Determine the UUID operations actually required and use a compatible target implementation or a narrowly defined adapter.',
['List used generation, parsing, formatting and comparison functions; inspect return/error conventions.', 'Prefer a supported target library or the framework\'s UUID implementation when it preserves the needed format and semantics.', 'If providing a compatibility adapter, verify the 16-byte representation, textual format and generation guarantees instead of implementing names only.', 'Remove the unavailable link dependency only after all required consumers use the verified replacement.'],
'Common dependency mechanism. Qt QUuid can be one adapter; other frameworks need their own equivalent implementation. UUID generation is not automatically suitable for secret tokens.',
'Do not claim complete libuuid compatibility for a shim implementing only three functions or assume every SDK lacks the library permanently.',
'Test binary/text round trips, invalid input, formatting, generation behavior and all functions actually used by callers.',
['problems/build-fail-libuuid-missing-ohos-musl.md'],['libuuid','uuid_generate','QUuid','musl'])

common('utf8-command-payload-transport','Preserve UTF-8 payload bytes across shell and HTTP boundaries','tooling',
'Non-ASCII text sent by a command-line HTTP client arrives garbled.',
'Inspect encoded request bytes and pass a UTF-8 payload file or structured request body instead of relying on ambiguous shell argument encoding.',
['Confirm the intended text and encoding before the shell boundary.', 'Write the payload with explicit UTF-8 and send its bytes using the client\'s file-body option or a structured API.', 'Use the appropriate content type/charset for the receiving protocol and inspect the actual request/response encoding.', 'Keep diagnostic logging free of credentials and avoid interpreting source text as shell code.'],
'Common host-tooling issue that can occur during any framework adaptation; not a HarmonyOS runtime text-rendering defect.',
'Changing application fonts or sprinkling quoting escapes does not repair altered request bytes.',
'Round-trip Chinese text, emoji, quotes and newlines and compare the received decoded text with the original.',
['problems/runtime-curl-chinese-garbled.md'],['UTF-8','curl','中文乱码','encoding'])

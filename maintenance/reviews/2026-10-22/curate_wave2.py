"""Additional source-reviewed mechanisms from the second discovery wave."""
import hashlib
import json
import pathlib
import runpy

HERE=pathlib.Path(__file__).resolve().parent
g=runpy.run_path(str(HERE/'curate.py'))
DB=g['DB']; snapshot=g['snapshot']; atom=g['atom']; dump=g['dump']; sources=g['SOURCES']
sources.update({
 'flameshot-ohos':dict(bare='flameshot',url='https://gitcode.com/ohos-qt/flameshot',revision='6ce0dbe9354c02e65b8eb688a495c6a3c4bd34b9',ref='main',framework='qt',license='GPL-3.0-or-later'),
 'tex-ide-ohos':dict(bare='ohos_TeX_IDE',url='https://gitcode.com/OpenHarmonyPCDeveloper/ohos_TeX_IDE',revision='87adaaedca76c355ff835e1ad2308c820d2804f5',ref='main',framework='qt',license='Application GPL-3.0; reviewed MiKTeX watcher carries GPL-2.0-or-later.'),
 'hachimi-world-ohos':dict(repo='HachimiWorld/hachimi-world-client-ohos',url='https://github.com/HachimiWorld/hachimi-world-client-ohos',revision='7d7bfa45badf4c1a39810eb9708d1f0abda8d494',ref='ohos-dev',framework='kotlin-multiplatform',license='AGPL-3.0',local_path=''),
 'kuikly-mmkv':dict(repo='walkman707/KuiklyMMKV',url='https://github.com/walkman707/KuiklyMMKV',revision='e1d6192802707ad3ef12b499065f121cfc2fad5c',ref='main',framework='kotlin-multiplatform',license='Apache-2.0',local_path=''),
})
for name in ['github-wave4-review.json','github-wave5-review.json','github-wave6-review.json']:
 for row in json.loads((HERE/name).read_text()):g['downloads'][(row['repo'],row['path'])]=row['content'].encode()
for row in json.loads((HERE/'github-wave4-trees.json').read_text()):
 g['trees'][row['repo']]={f['path']:f.get('sha') for f in row['tree']['tree'] if f['type']=='blob'}

FW='HarmonyOS/entry/src/main/ets/qability/QAbility.ets'
SAVE='src/utils/screenshotsaver.cpp'
WATCH='miktex/Libraries/MiKTeX/Core/FileSystemWatcher/ohos/ohosFileSystemWatcher.cpp'
HBASE='composeApp/src/ohosMain/kotlin/world/hachimi/app/'
SESSION=HBASE+'player/OhosMediaSession.kt'

atom('qt-ohos-window-stage-recreation','Guard repeated QPA window-stage creation during scene recovery','lifecycle','qt',
 'After backgrounding a Qt OH app with no visible windows, reopening aborts with a missing io.qt.private.abilityInstanceId or pending auto-started instance error.',
 'Distinguish first stage creation from repeated creation on the same UIAbility and use the QPA recovery path only for the matching recovered instance.',
 'Qt 6.12 OH QPA with non-idempotent handleAbilityOnWindowStageCreate and a compatible handleAbilityOnWindowStageRestore. Relevant to PC tray-only/background windows; newer QPA implementations may differ.',
 ['onWindowStageCreate','scene recovery','io.qt.private.abilityInstanceId','handleAbilityOnWindowStageRestore','tray','background reopen'],
 ['foreground-recovery-and-save-lifetime','qt-qpa-runtime-template-coherence'],
 ['Log ability identity and window-stage create/destroy/restore ordering so a repeated callback can be distinguished from a fresh process or fresh ability.',
  'Keep the first-create marker on the ability instance. Forward the first successful creation to handleAbilityOnWindowStageCreate using the matching runtime/template.',
  'If the same ability receives another create callback after stage destruction, route it to the reviewed QPA restore handler instead of re-entering its one-time instance-registration path.',
  'Keep actual onWindowStageRestore forwarding and QPA lifecycle ownership coherent. A genuinely new ability must still take the initial create path.',
  'Serialize asynchronous permission/startup work with recovery. Mark initialization complete only for the stage that still exists; do not copy an early boolean that can hide a failed or abandoned first initialization.'],
 'The reviewed guard prevents a specific fatal re-entry. Its source still reports possible blank content after very long background sessions, so it does not establish full scene recovery. Inspect the selected QPA restore semantics before applying this version-specific route.',
 'Do not reset the ability marker merely because the stage was destroyed, share one marker across all abilities, or claim a non-crashing blank window is a successful recovery.',
 'Exercise minimize, closing all windows while retaining tray state, short/long background, relaunch and process death. Verify both absence of the fatal error and restored visible/interactable Qt content.',
 [('flameshot-ohos',FW),('flameshot-ohos','README.md')],['Crash guard only; the upstream long-background blank-window limitation remains explicit.'])

atom('qt-ohos-save-picker-name-and-format','Set the save-picker filename and default format separately','storage','qt',
 'A Qt OH save dialog ignores the suggested filename, produces files_3.ppm, or saves an image with a suffix that the system preview cannot open.',
 'Pass a directory as the initial location, preselect the basename explicitly, and put the intended MIME/suffix choice first for the selected OH picker adapter.',
 'QFileDialog backed by the reviewed Qt 6.12 OH DocumentViewPicker helper. Applies to PC save/export windows; supported initial locations and authorization remain independent requirements.',
 ['QFileDialog','selectFile','newFileNames','files_3.ppm','setMimeTypeFilters','setDefaultSuffix','PNG','HiPreview'],
 ['qt-picker-initial-uri','qt-picker-suffix-filter-mapping','file-export-contract'],
 ['Keep a stable visible foreground parent while opening the native save dialog; do not leave it attached only to an already-hidden capture window.',
  'Split a suggested full path into an allowed initial directory and basename. Construct QFileDialog with the directory and call selectFile(basename) so the adapter can fill newFileNames.',
  'Build filters from formats the application can actually encode. Move the configured MIME type to the front; use a supported PNG fallback when appropriate.',
  'Set the selected MIME filter and default suffix as well. The reviewed adapter applies its first native suffix choice even when a prefilled basename has another suffix.',
  'After acceptance, use the returned target and the selected format consistently. Return cancellation normally and verify file bytes as well as the filename.'],
 'Filter order is adapter-specific behavior, not a universal Qt rule. A correctly named file can still contain the wrong encoding or be unreadable by another app. Omit an unsupported initial location instead of assuming a private path is a valid document URI.',
 'Do not pass a complete filename as the initial directory, rely on supportedMimeTypes enumeration order, or treat setDefaultSuffix alone as proof of the effective native choice.',
 'Save PNG and JPEG with default and edited basenames, cancel, and invoke export after hiding the capture view. Inspect signatures and suffixes and open the results in the intended PC preview application.',
 [('flameshot-ohos',SAVE),('flameshot-ohos','README.md')])

atom('native-directory-polling-fallback','Use bounded directory snapshots when native watching is unavailable','storage',None,
 'A native OH dependency needs file-change notifications but its selected backend cannot use its normal inotify implementation.',
 'Use a cancellable polling worker that diffs directory snapshots, with explicit loss, error and shutdown semantics.',
 'Native OH filesystem consumers with app-authorized readable directories and no usable event backend. PC project/cache trees can be large; this is a latency/CPU tradeoff rather than equivalent inotify behavior.',
 ['FileSystemWatcher','inotify','poll','lstat','mtime','cancel pipe','directory watcher'],
 ['application-path-namespace','foreground-recovery-and-save-lifetime'],
 ['Register only needed directories and take an initial snapshot of filename plus relevant metadata. Define whether watching is shallow or recursive.',
  'On a worker, use poll on a cancellation pipe with a finite interval. On timeout scan and emit added, removed and changed records by comparing snapshots.',
  'Treat scan failure as an error/unknown state, not an empty directory. Preserve the previous valid snapshot on EACCES or transient I/O failure to avoid false mass-deletion events.',
  'Serialize directory registration, snapshot mutation and notification delivery, or use one owner thread with a command queue. Copying a directory list under a lock does not protect a separate snapshot map.',
  'On stop, signal cancellation, join the worker and close pipe descriptors only after work is quiescent. Reinitialize/drain cancellation state before restart as required.'],
 'The source tracks second-resolution mtime and size and scans one directory level. Same-size rewrites within the same timestamp interval and short-lived files can be missed. Renames may appear as remove/add; add higher-resolution metadata or stronger checks only where the feature requires them.',
 'Do not assert that inotify is absent on every OH product, translate opendir failure into deletion, or perform large recursive scans on the UI thread.',
 'Test add/remove/rename, same-size fast rewrites, permission loss/recovery, directory disappearance, concurrent registration and stop/restart. Measure latency and scan cost on a realistic PC workspace.',
 [('tex-ide-ohos',WATCH),('tex-ide-ohos','README.md')],
 ['Reviewed implementation returns an empty scan on opendir failure and mutates snapshots outside the directory-list lock; the entry explicitly hardens these source limitations rather than claiming they were fixed or tested.'])

atom('kmp-ohos-prepared-compose-resources','Package generated Compose resource values for OH','storage','kotlin-multiplatform',
 'A Compose OH application links and starts but throws Failed to open raw file when reading a generated string or other Compose resource.',
 'Publish the prepared Compose resources produced by the matching build into the namespace expected by the OH resource reader.',
 'CPF Compose 1.9.2-1.0.0 resource adapter with the reviewed generated .cvr layout. Source evidence originated from a phone-targeted host; PC packaging and resource behavior remain unverified.',
 ['preparedResources','.cvr','Failed to open raw file','prepareComposeResourcesTaskForCommonMain','composeResources','rawfile'],
 ['kmp-ohos-rawfile-resources','kmp-ohos-dependency-and-renderer'],
 ['Inspect the selected Compose adapter\'s generated resource path and namespace rather than copying raw XML on the assumption it is the runtime format.',
  'Make the host-publishing Copy task depend on resource preparation and the matching Kotlin shared-library link.',
  'For this layout, copy build/generated/compose/resourceGenerator/preparedResources/commonMain/composeResources into entry/src/main/resources/rawfile/composeResources/<generated-resource-package>.',
  'Copy libkn.so and its generated header from the same variant/ABI alongside those resources. Avoid mixing old generated namespaces with new Kotlin code.',
  'Inspect the built HAP inventory and test a localized generated string after a clean build; stale copied resources can hide missing task dependencies.'],
 'Paths and task names are version-specific. ResourceManager-based raw image reads and Compose generated-value lookup have different contracts. A simulator or phone result does not establish PC resource lookup or DPI behavior.',
 'Do not copy only src/commonMain/composeResources XML when the adapter expects generated .cvr values, omit resource preparation from the task graph, or assume successful native linking proves resources are packaged.',
 'Use clean and incremental builds, change a localized value, and verify the HAP contains the updated generated resource. Exercise first launch and missing-resource error handling on the intended PC.',
 [('hachimi-world-ohos','composeApp/build.ohos.gradle.kts'),('hachimi-world-ohos','OHOS_MIGRATION.md')])

atom('kmp-ohos-resource-environment-main-thread','Resolve Compose resource environments on the owning OH thread','native','kotlin-multiplatform',
 'Calling getString(StringResource) from a worker coroutine terminates the OH process with ecma_vm cannot run in multi-thread!.',
 'Dispatch resource lookup to the initialized OH main dispatcher when the selected Compose adapter queries ArkTS state through NAPI.',
 'CPF Compose resource adapter that reads the resource environment over NAPI for each lookup. Source host is phone-oriented; PC locale/theme/DPI changes and dispatcher ownership need validation.',
 ['ecma_vm cannot run in multi-thread!','getString','StringResource','Dispatchers.Main','withContext','NAPI','locale'],
 ['kmp-ohos-prepared-compose-resources','kmp-ohos-compose-host'],
 ['Confirm the failing lookup queries an ArkTS/NAPI environment; reading a native raw byte file does not by itself require the same UI-thread path.',
  'Initialize the KMP main handler from the owning ArkTS thread before using Dispatchers.Main.',
  'Use a suspend helper with withContext(Dispatchers.Main) { getString(resource, *formatArgs) } on OH. Resolve the string there and pass the immutable result back to background work.',
  'Use the helper in non-Composable service/model code as well as UI callers. Avoid blocking waits from the UI thread back to a worker waiting for that thread.',
  'Re-evaluate the workaround if the adapter changes to cache its platform environment. Any cache must still invalidate correctly for locale, theme and display changes.'],
 'This applies to an adapter whose environment lookup crosses NAPI. It is not a rule that all Kotlin resource IO belongs on the UI thread. The source helper keeps other platforms on their original lookup path.',
 'Do not move every image decode or file read onto the UI thread, use an uninitialized Main dispatcher, or hold a lock across a cross-thread lookup.',
 'Resolve localized strings from UI and worker callers, then change locale/theme/display scale and retry. Confirm no VM-thread abort, correct values and responsive PC windows.',
 [('hachimi-world-ohos','composeApp/src/commonMain/kotlin/world/hachimi/app/util/StringResources.kt'),('hachimi-world-ohos','composeApp/src/commonMain/kotlin/world/hachimi/app/model/GlobalStore.kt'),('hachimi-world-ohos','harmonyApp/entry/src/main/cpp/napi_init.cpp'),('hachimi-world-ohos','OHOS_MIGRATION.md')])

atom('native-avsession-metadata-asset-id','Include stable asset identity in native AVSession metadata','media',None,
 'OH_AVSession_SetAVMetadata returns AV_SESSION_ERR_SERVICE_EXCEPTION (6600101) although individual title, artist or cover setters appear successful.',
 'Set a nonempty assetId before generating native metadata and check publication separately from each builder operation.',
 'OH native AVSession/AVMetadata builder APIs as used by the reviewed OH Kotlin distribution. Relevant to PC media controls on products exposing those APIs; required fields and versions must be checked for the selected SDK.',
 ['OH_AVMetadataBuilder_SetAssetId','OH_AVSession_SetAVMetadata','6600101','AV_SESSION_ERR_SERVICE_EXCEPTION','assetId','cover art'],
 ['qt6-ohos-avsession-bridge','kmp-ohos-native-avsession-host'],
 ['Create the metadata builder and supply a stable track identity with OH_AVMetadataBuilder_SetAssetId before generating the metadata object.',
  'Add title, artist, duration and cover URI as supported, checking their return codes. A successful setter does not prove the session service accepts the complete record.',
  'Generate metadata and check both the generation result and OH_AVSession_SetAVMetadata result. Log codes without assuming every service exception is a cover-decoding problem.',
  'Retain builder-owned storage for the lifetime required by the selected API contract; replace and destroy metadata/builders in a defined order.',
  'Update identity when the track changes even if two tracks have identical titles. Keep unavailable session/cover capabilities visible to the application.'],
 'The missing-assetId failure is source-reported and the reviewed code sets it explicitly. This is a scoped diagnostic, not a claim that every 6600101 error has the same cause.',
 'Do not rely on title as the only identity, diagnose the cover first without inspecting required fields, or report media-control integration from builder success alone.',
 'Publish metadata with and without a valid identity under controlled conditions, switch between same-title tracks, and inspect the actual PC control card and native return codes.',
 [('hachimi-world-ohos',SESSION),('hachimi-world-ohos','OHOS_MIGRATION.md')])

atom('kmp-ohos-native-avsession-host','Connect KMP playback to an OH native media session','media','kotlin-multiplatform',
 'A Kotlin/Native OH player produces audio but the system media card has no reliable metadata, controls or background-playback ownership.',
 'Use the OH native AVSession bindings with an explicit callback owner, publish player state, and coordinate audio background tasks through the owning ArkTS ability.',
 'CPF Kotlin distribution exposing platform.AVSessionKit.OHAVSession and a compatible OH product/API. Source host is phone-oriented; PC minimize/background distinctions and media controls require separate validation.',
 ['StableRef','staticCFunction','OHAVSession','RegisterCommandCallback','RegisterSeekCallback','AUDIO_PLAYBACK','9800005','KEEP_BACKGROUND_RUNNING'],
 ['native-avsession-metadata-asset-id','continuous-background-task-contract','qt6-ohos-avsession-bridge'],
 ['Verify the actual Kotlin distribution includes the OH AVSession cinterop and link the target native libraries. Create/activate an audio session with the running bundle and declared ability identity.',
  'Register top-level staticCFunction callbacks with a retained StableRef as userData. Forward play/pause/next/previous/seek into the application playback service rather than creating a second player.',
  'Publish metadata with assetId, playback state and position using the native API\'s time units. Check every registration and publication result.',
  'When legitimate playback becomes active, notify the ArkTS host. Request an audio-playback continuous task only with the required permission, ability backgroundModes and eligible active session; stop it on pause/end as appropriate.',
  'Serialize playback and background-task transitions so overlapping promises cannot publish stale state. Apply a generation when the owner changes.',
  'On teardown cancel/join the publisher, stop background work, unregister callbacks, ensure callbacks are quiescent, destroy session and metadata, then dispose the StableRef. Callback thread assumptions require explicit verification.'],
 'The source uses a Main-dispatcher polling publisher and retained C callback state. It does not establish callback-thread safety or fully cancel that publisher in release(); the cleanup ordering above is defensive guidance, not a tested patch. PC minimization may not produce the same background transition as a phone.',
 'Do not dispose StableRef while callbacks can still execute, infer background eligibility from playing audio alone, or assume all multimedia APIs in an OH fork are available in every product SDK.',
 'On the intended PC, verify external play/pause/seek, same-title track changes, foreground/background transitions and teardown with commands in flight. Confirm session/task cleanup and no callbacks to released Kotlin state.',
 [('hachimi-world-ohos',SESSION),('hachimi-world-ohos','harmonyApp/entry/src/main/ets/entryability/EntryAbility.ets'),('hachimi-world-ohos','harmonyApp/entry/src/main/module.json5'),('hachimi-world-ohos','composeApp/build.ohos.gradle.kts')],
 ['No callback race, background-task regression or PC runtime test was run.'])

atom('native-c-wrapper-optional-string-lifetime','Audit optional string lifetimes before native storage calls','native',None,
 'An OH C wrapper passes optional encryption-key or root-directory pointers into a native storage API after the temporary std::string objects have left scope.',
 'Keep each optional std::string owner alive through the native call; treat the reviewed dangling-pointer path as unresolved until corrected and exercised.',
 'Native C++ storage wrappers called from KMP or another OH runtime. Relevant to PC native storage arguments; successful compilation does not validate optional-key/root-path behavior.',
 ['MMKV','mmkv_c_default_mmkv','mmkv_c_mmkv_with_id','std::string','crypt_key','root_dir','dangling pointer','use after scope'],
 ['kmp-napi-string-allocator-ownership','native-bridge-stale-handle-identity'],
 ['Trace pointer construction to the actual native call, including optional branches. In the reviewed pattern key_ptr/root_dir_ptr outlive the inner block that declares their std::string owners.',
  'As a candidate correction, declare the string owners in the enclosing call scope and select either their addresses or nullptr without letting them leave scope early.',
  'Verify whether the native storage API consumes or retains the pointed-to data. If it retains it, extend ownership to the documented retention lifetime instead of only to function return.',
  'Keep null and empty-string behavior distinct and review error-return handling before exposing the result as a usable storage instance.',
  'Keep this path unresolved until a corrected wrapper is compiled and exercised with non-null keys and explicit root directories.'],
 'This is a source-level lifetime observation, not a reproduced storage failure or a verified patch. The active target/build and cinterop exist, but they do not establish that encryption, custom-root handling or every wrapper method is safe.',
 'Do not point at block-local strings and call the native API after the block, assume an ordinary caller-owned const char* has transferred ownership, or report a no-key demo as validation of optional branches.',
 'Use a focused native lifetime checker where available, then exercise default storage, encrypted storage, custom roots and reopen with matching/mismatched keys on the intended PC. Preserve user data during verification.',
 [('kuikly-mmkv','ohosApp/mmkv_c_wrapper/src/main/cpp/mmkv_c_wrapper.cpp'),('kuikly-mmkv','mmkvKotlin/build.gradle.kts'),('kuikly-mmkv','mmkvKotlin/src/ohosArm64Main/kotlin/com/kuikly/thirdparty/kmp/lib/mmkv/MMKVImpl.kt'),('kuikly-mmkv','ohosApp/mmkv_c_wrapper/src/main/cpp/CMakeLists.txt')],
 ['No source application was modified or built; source-level dangling optional-argument pointers remain unresolved.'])
p=DB/'knowledge/native-c-wrapper-optional-string-lifetime.md'
raw,body=p.read_text().split('---\n',2)[1:];meta=json.loads(raw);meta['status']='unresolved';p.write_text('---\n'+json.dumps(meta,indent=2)+'\n---\n'+body)

embedded=json.loads((HERE/'flutter-qt-current-ref.json').read_text())
sources['flutter-embed-into-qt']=dict(bare='flutter_embed_into_qt-current',url=embedded['url'],revision=embedded['revision'],ref=embedded['ref'],framework='qt',frameworks=['qt','flutter'],license='Apache-2.0')
EBASE='flutter_embed_into_qt/embedded/'
atom('qt-flutter-ohos-embedded-node-host','Host a Flutter OH page inside a Qt widget','integration',['qt','flutter'],
 'A Qt OH application needs an embedded Flutter page with independent routing, input focus and messages rather than a separate application window.',
 'Use a compatible mixed ArkUI-node bridge, one coherent Flutter HAR dependency, and per-page engine identity across Qt, ArkTS and Dart.',
 'Qt for OpenHarmony QPA accepting the bridge\'s native Node wrapper and the matched Flutter OH embedding. PC focus, resize, multi-page isolation and destruction are unverified in this curation.',
 ['FlutterEmbeddedQWidget','QWindow::fromWinId','createWindowContainer','ComponentContent','FlutterManager','embedded.har','engineId','PACKAGE_FIND_FILE'],
 ['qt-qpa-runtime-template-coherence','rnoh-native-host-and-codegen','native-bridge-stale-handle-identity'],
 ['Resolve one compatible Flutter HAR for the Qt host and bridge, including the root override when both depend on it. Match Qt QPA and bridge ABI; do not package arbitrary duplicate Qt libraries.',
  'Include PACKAGE_FIND_FILE when provided by the OH cross-HAR build and link the bridge target into the Qt business library.',
  'Push the owning UIAbility and WindowStage into FlutterManager. Register the native bridge, message transfer and mixed-component factory on the owning UI thread before requesting an embedded page.',
  'Create the bridge\'s ArkUI ComponentContent containing FlutterPage. The reviewed Qt wrapper passes a bridge-specific Node through QWindow::fromWinId and QWidget::createWindowContainer; it is not an arbitrary desktop native handle.',
  'Use one unique engine/page ID in the FlutterWindow, plugin registry, Dart entrypoint arguments and message handler. Route messages through queued Qt invocation and an owning-thread NAPI/TSFN path.',
  'Forward focus/blur as window-stage activation events and page visibility to the FlutterWindow. Package the assets/AOT product required by the exact Flutter build mode.',
  'Remove the widget and request bridge destruction before releasing its wrapper. Verify actual engine, window-map, message-handler, native-node and TSFN cleanup; visual disappearance alone does not establish release.'],
 'The reviewed bridge provides source for mounting and communication, but its aboutToDisappear only logs. Runtime cleanup and late-message safety need independent validation. Its native Node wrapper is specific to the selected QPA; a matching SDK is a precondition, not a universal Qt embedding API.',
 'Do not mix Qt runtimes from the host and HAR, route every page through one unscoped channel, assume onPageHide destroys an engine, or copy setup commands as evidence that the integration runs.',
 'On the intended PC create two pages, exchange messages in both directions, resize and switch focus, destroy/recreate one page, and inspect engine/node/handler counts and late-message rejection. Verify correct debug/release resources separately.',
 [('flutter-embed-into-qt','README.md'),('flutter-embed-into-qt',EBASE+'src/main/cpp/src/FlutterEmbeddedQWidget.cpp'),('flutter-embed-into-qt',EBASE+'src/main/cpp/src/EmbeddedBridge.cpp'),('flutter-embed-into-qt',EBASE+'src/main/ets/flutter/EncapsulationXCompent.ets'),('flutter-embed-into-qt',EBASE+'src/main/ets/flutter/MyFlutterWindow.ets'),('flutter-embed-into-qt',EBASE+'oh-package.json5'),('flutter-embed-into-qt','example/qt-embed-demo/qtmodule/src/main/ets/entryability/EntryAbility.ets')],
 ['Gitee is archived; the current canonical GitCode master was reviewed. No source setup/build instructions were executed.','The source does not prove complete destruction, queue-failure cleanup or a safe multi-window global UIContext.'])
sources['rnoh-skia-ohos']=dict(repo='hime88/react-native-skia-ohos',url='https://github.com/hime88/react-native-skia-ohos',revision='a9c799e33238ee13ea25853f805d91fde7094939',ref='sig',framework='react-native',license='MIT for the reviewed React Native Skia adapter; vendored components retain their notices.',local_path='')
for name in ['github-rnoh-screen.json','github-rnoh-extra.json']:
 for row in json.loads((HERE/name).read_text()):g['downloads'][(row['repo'],row['path'])]=row['content'].encode()
for row in json.loads((HERE/'github-rnoh-tree.json').read_text()):g['trees'][row['repo']]={f['path']:f.get('sha') for f in row['tree']['tree'] if f['type']=='blob'}
RBASE='tester/harmony/skia/src/main/'
atom('rnoh-native-component-registration-parity','Register each RNOH native view across descriptor and binder maps','integration','react-native',
 'An RNOH package and TurboModule load, but a specific native canvas/view name is missing, blank or unbound.',
 'Register the supported component name in its descriptor provider and both native binder maps, then include that package and library in the actual host.',
 'RNOH C-API adapter exposing createComponentDescriptorProviders, createComponentJSIBinderByName and createComponentNapiBinderByName. PC canvas surface, mouse input and rendering require separate validation.',
 ['ComponentDescriptorProvider','ComponentJSIBinderByString','ComponentNapiBinderByString','SkiaDomView','SkiaPictureView','RNSkiaModule','PackageProvider'],
 ['rnoh-native-host-and-codegen','native-registration-and-handler-coverage'],
 ['Inventory the JS/native component names actually used by the app; module-level installation and component-level registration are separate checks.',
  'For each supported view, provide its component descriptor and matching exact-name entries in the JSI binder and NAPI binder maps required by the selected RNOH version.',
  'Return the native package from the host PackageProvider, compile/link its native library, and register the corresponding ArkTS/TurboModule package where that adapter requires it.',
  'Check the adapter\'s supported subset. The reviewed source registers SkiaDomView while SkiaPictureView entries are commented out; do not infer support from upstream package exports.',
  'When adding a missing view, implement its actual binder/surface contract and regenerate required descriptors before enabling map entries. A name-only placeholder is not a renderer.'],
 'This entry covers registration parity, not a complete Skia OH port. Skia native dependencies, surface ownership and thread scheduling remain separate requirements. A TurboModule returning successfully does not prove a component renders.',
 'Do not treat package registration as proof every upstream view exists, enable an unsupported descriptor without binders, or declare a blank canvas successful because the HAP starts.',
 'Render each advertised native view in a minimal OH page, exercise property updates and unmount/remount, and confirm unsupported names fail explicitly. On PC, also verify resize, DPI and pointer input.',
 [('rnoh-skia-ohos',RBASE+'cpp/SkiaPackage.cpp'),('rnoh-skia-ohos',RBASE+'cpp/CMakeLists.txt'),('rnoh-skia-ohos',RBASE+'ets/RNSkiaPackage.ts'),('rnoh-skia-ohos','tester/harmony/entry/src/main/cpp/PackageProvider.cpp'),('rnoh-skia-ohos','tester/harmony/entry/src/main/cpp/CMakeLists.txt'),('rnoh-skia-ohos','tester/harmony/entry/oh-package.json5')],
 ['Source-only review of registration; no rendering, build or physical PC claim.'])
sources['expo-device-ohos']=dict(bare='expo-device',url='https://gitcode.com/oh-react-native/expo-device',revision='9398b89f2fc3ed24fff81505420839b2ce1a72b1',ref='main',framework='react-native',license='MIT')
EXPO='harmony/expo_device/src/main/ets/ExpoDeviceTurboModule.ts'
atom('ohos-native-form-factor-classification','Distinguish OH form factor from runtime and window size','window',None,
 'An OH application selects phone behavior on a 2in1 device, or a narrow desktop window is incorrectly treated as proof of a phone device.',
 'Read the native device type, map 2in1 explicitly to the application\'s desktop category, and keep unknown form factors and window capabilities separate.',
 'OH products exposing deviceInfo.deviceType. PC classification is source-reviewed through two framework adapters; it does not verify fullscreen, input, display count or every product identifier.',
 ['deviceInfo.deviceType','2in1','DeviceType.DESKTOP','form factor','window size','unknown','PC detection'],
 ['flutter-ohos-pc-form-factor','viewport-scale-and-layout-lifetime','device-gesture-capability'],
 ['Read deviceInfo.deviceType through an actual OH native adapter; an OS/runtime label alone cannot distinguish phone, tablet and 2in1.',
  'Map known platform values to a documented application enum. Preserve unknown or unsupported values as unknown; the reviewed Expo mapping uses 2in1 -> DESKTOP.',
  'Keep form factor separate from the current window bounds. Use responsive layout for resize while using supported native capabilities for maximize/fullscreen and other window operations.',
  'Make initial lookup and cached-value behavior explicit in the adapter. Choose a reversible safe UI while classification is pending rather than enabling unsupported window calls.',
  'Declare only supported device types in the host manifest and verify actual input/window/display capabilities before offering the associated action.'],
 'A platform category is a policy input, not proof that an app supports every desktop feature. Phone-origin tests of a device-info plugin do not verify its 2in1 mapping on a physical PC. Unknown values must remain recoverable as product identifiers evolve.',
 'Do not equate every OH runtime with a phone, equate width with device identity, infer touch/keyboard availability from one enum, or treat a DESKTOP return value as completed PC adaptation.',
 'On the intended products compare native type, adapter enum and chosen behavior. Resize a PC window below a mobile breakpoint and verify responsive layout changes without changing hardware identity; test unknown values and lookup errors.',
 [('expo-device-ohos',EXPO),('expo-device-ohos','README.OpenHarmony_CN.md')])
common_prov=DB/'provenance/ohos-native-form-factor-classification.json';prov=json.loads(common_prov.read_text());old_prov=json.loads((DB/'provenance/flutter-ohos-pc-form-factor.json').read_text())
for record in old_prov['sources']:
 if record['original_path'].endswith('MethodCall.ets'):prov['sources'].append(record)
prov['transformations'].append('Separated the common native device-type classification from the existing Flutter channel/window adapter, corroborated by an independently inspected RNOH Expo mapping.')
dump(common_prov,prov)
g['extend']('flutter-ohos-pc-form-factor',
 'The device type is reported by the platform and may not distinguish every PC-like product.',
 'Use the shared [native form-factor classification](ohos-native-form-factor-classification.md) for the OS category and unknown-value contract. The device type is reported by the platform and may not distinguish every PC-like product.',
 [('expo-device-ohos',EXPO)],['ohos-native-form-factor-classification'])
for sid,path in [('hachimi-world-ohos','LICENSE'),('kuikly-mmkv','LICENSE'),('flameshot-ohos','README.md'),('tex-ide-ohos','README.md'),('flutter-embed-into-qt','LICENSE'),('rnoh-skia-ohos','react-native-skia/LICENSE.md'),('expo-device-ohos','LICENSE')]:snapshot(sid,path)
dump(HERE/'curation-manifest.json',dict(date=g['DATE'],added_entries=g['atoms'],extended_entries=g['extended'],split_entries=['flutter-ohos-pc-form-factor'],sources=sources,snapshots=list(g['records'].values())))
print(json.dumps(dict(added_total=len(g['atoms']),snapshots=len(g['records'])),indent=2))

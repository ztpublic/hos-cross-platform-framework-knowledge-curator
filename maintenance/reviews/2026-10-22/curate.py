"""Persist reviewed source bytes and atomic guidance; never runs source build code."""
import hashlib
import json
import pathlib
import subprocess

DB = pathlib.Path(__file__).resolve().parents[3]
REVIEW = pathlib.Path(__file__).resolve().parent
DATE = '2026-10-22'
SOURCES = {
    'filekit-ohos': dict(repo='HachimiWorld/FileKit-ohos', url='https://github.com/HachimiWorld/FileKit-ohos', revision='f44915a297d555a614b965f2245c09ade5183c8a', ref='0.12.0-OH-1.0.0', framework='kotlin-multiplatform', license='MIT', local_path=''),
    'liberika': dict(repo='AimesSoft/liberika', url='https://github.com/AimesSoft/liberika', revision='4fc74af5b2fabfbf82557d0221923a0872b9e45e', ref='main', framework='flutter', license='MPL-2.0; individual components retain their licenses.', local_path=''),
    'kuiklyui': dict(repo='Tencent-TDS/KuiklyUI', url='https://github.com/Tencent-TDS/KuiklyUI', revision='277b69d1bb043ae6ce87415ad0d28df17de1a12c', ref='main', framework='kotlin-multiplatform', license='KuiklyUI License (custom; retained notice).', local_path=''),
    'dsh-desktop-hos': dict(repo='fellow99/dsh-desktop-hos', url='https://github.com/fellow99/dsh-desktop-hos', revision='ef585fdecc33723fa82370184c7b0b7a6ef3ce70', ref='main', framework='electron', license='MIT', local_path=''),
    'kamel-ohos': dict(bare='Kamel', url='https://gitcode.com/oh-tpc/Kamel', revision='9a41666c6266521e56a1724ac35a42629f58a398', ref='main', framework='kotlin-multiplatform', license='Apache-2.0'),
    'persepolis-ohos': dict(bare='persepolis', url='https://gitcode.com/ohos-qt/persepolis', revision='6930d745a6ad44a51da06b1fe263beb3a5a29ef9', ref='main', framework='qt', license='GPL-3.0 (README attribution; no repository-level license file).'),
    'kotlinx-io-ohos': dict(bare='kotlinx-io', url='https://gitcode.com/oh-tpc/kotlinx-io', revision='97d2c325f6846ff2764f1509577e0aa1036f1bfc', ref='master', framework='kotlin-multiplatform', license='Apache-2.0'),
    'result-ohos': dict(bare='Result', url='https://gitcode.com/oh-tpc/Result', revision='062c5402975abeb04ccfa8a837b1f044f59002f3', ref='master', framework='kotlin-multiplatform', license='MIT'),
}

def dump(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n')

downloads = {}
for name in ['github-source-review.json', 'github-media-review.json', 'github-extra-review.json', 'github-final-source-review.json', 'github-update-docs.json', 'discovery-readmes.json']:
    for row in json.loads((REVIEW/name).read_text()):
        content = row.get('content', '')
        if content.startswith('{"content":[{"type":"text","text":"GitHub API error'):
            continue
        downloads[(row['repo'], row['path'])] = content.encode()
trees = {x['repo']: {f['path']: f.get('sha') for f in x['tree']['tree'] if f['type'] == 'blob'} for x in json.loads((REVIEW/'github-source-trees.json').read_text())}
records = {}

def snapshot(source_id, path):
    source = SOURCES[source_id]
    if 'bare' in source:
        repo = pathlib.Path('/Users/mac/projects/harmonyos-kb-research/2026-10-22')/(source['bare']+'.git')
        data = subprocess.check_output(['git', '--git-dir='+str(repo), 'show', source['revision']+':'+path])
        source['local_path'] = str(repo)
    else:
        data = downloads[(source['repo'], path)]
        expected = trees.get(source['repo'], {}).get(path)
        if expected:
            actual = hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()
            if expected != actual:
                raise ValueError(f'Git blob identity mismatch: {source_id} {path}: {expected} != {actual}')
    target = DB/'provenance/snapshots/projects'/source_id/DATE/path
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(data)
    record = dict(collection=source_id, original_path=path, snapshot=str(target.relative_to(DB)), sha256=hashlib.sha256(data).hexdigest(), revision=source['revision'], license=source['license'], url=source['url'], evidence_note='Source text reviewed during curation. No build, install, simulator or device execution performed.')
    records[(source_id,path)] = record
    return record

FBASE = 'filekit-core/src/ohosMain/kotlin/io/github/vinceglb/filekit/'
DBASE = 'filekit-dialogs/src/ohosMain/kotlin/io/github/vinceglb/filekit/dialogs/'
PBASE = 'HarmonyOS/entry/src/main/'
KBASE = 'core-render-ohos/src/main/cpp/libohos_render/foundation/thread/'
LBASE = 'packages/erika_flutter/'

atoms = []
def atom(id, title, area, framework, symptom, remedy, applicability, keywords, related, steps, conditions, mistakes, checks, sources, limitations=()):
    meta = dict(schema_version=1,id=id,title=title,topic_key=area+'.'+id,area=area,scope='framework-specific' if framework else 'common',frameworks=(framework if isinstance(framework,list) else [framework]) if framework else [],summary=remedy,when_to_use=symptom,applicability=applicability,keywords=keywords,related=related,status='scoped',evidence_tier='source-reviewed',pc_validation='not-run')
    body = '\n\n'.join(['# '+title,'## When to use',symptom,'## Recommended approach',remedy,'## Implementation steps','\n'.join(f'{i+1}. {s}' for i,s in enumerate(steps)),'## Conditions and alternatives',conditions,'## Common mistakes',mistakes,'## Check it works',checks])+'\n'
    (DB/'knowledge'/f'{id}.md').write_text('---\n'+json.dumps(meta,ensure_ascii=False,indent=2)+'\n---\n\n'+body)
    evidence = [snapshot(s,p) for s,p in sources]
    dump(DB/'provenance'/f'{id}.json',dict(schema_version=1,id=id,sources=evidence,transformations=['Extracted one independently actionable mechanism; project identities and research history remain in provenance.','Acceptance checks and defensive implementation advice are recommendations, not executed results.']+(['Generalized a native platform invariant from the reviewed integration; no independent cross-framework verification claimed.'] if not framework else []),limitations=['Only source text was reviewed; pc_validation is not-run.']+list(limitations)))
    atoms.append(id)

atom('kmp-filekit-picker-descriptor-ownership','Keep picker files alive with owned descriptors in KMP','storage','kotlin-multiplatform',
     'A Kotlin file API cannot read a selected OH URI as a sandbox path, or selection becomes unreadable after the native picker closes its file.',
     'Open the granted URI through the OH file API, duplicate its descriptor into the KMP file object, and close each file or stream owner explicitly.',
     'Experimental FileKit OH adapter with CPF Kotlin 2.2.21-1.0.0 and ohosArm64/ohosX64. PC file-provider grants, restart behavior and directory selection require separate verification.',
     ['FileKit','PlatformFile','fromFileDescriptor','dup','pread','pwrite','Picker URI has no sandbox path','bookmark','close'],
     ['document-picker-uri-access','sandbox-import-as-access-fallback','picker-cancellation-concurrency'],
     ['Initialize the dialog adapter on the ArkTS/UI thread with the current napi_env and owning UIAbilityContext before selecting. Dispose it from that ability\'s destruction callback on the same thread.',
      'Use the OH file module to open the granted URI. Call PlatformFile.fromFileDescriptor(fd, uri, name), which duplicates the descriptor; the caller still closes the original OH file.',
      'Read through source() or write through sink(). Each opened stream owns a further duplicate; close the stream even when the PlatformFile has already been closed.',
      'Close the PlatformFile after use. Do not persist its /proc/self/fd/<number> representation or call toKotlinxIoPath() on a picker-backed object.',
      'Copy into FileKit.filesDir before storing a bookmark or requiring restart durability. A descriptor number cannot preserve the original temporary grant across processes.'],
     'The reviewed streams use pread/pwrite with private offsets and retry EINTR. This permits separate read positions despite dup sharing the underlying open file description. Directory selection separately checks whether the selected directory is accessible as a real path; it is not covered by the file-descriptor remedy. Camera, sharing and physical-device permission differences were not established by this review.',
     'Do not construct PlatformFile(uri), treat ctime as creation time, serialize a live descriptor as a durable path, or rely on a cleaner to release scarce descriptors promptly.',
     'On the intended PC, select external files, read with two simultaneous streams, close the parent object while one stream is still open, and check independent read positions. Test partial multi-selection failure, cancellation, missing grants and restart after an explicit sandbox copy.',
     [('filekit-ohos',FBASE+'PlatformFile.ohos.kt'),('filekit-ohos',FBASE+'OhosDescriptor.kt'),('filekit-ohos',DBASE+'OhosSystemDialogs.kt'),('filekit-ohos','filekit-dialogs/build.gradle.kts'),('filekit-ohos','OHOS.zh-CN.md')],
     ['Upstream reports ARM64 simulator checks; no physical PC validation is inferred.'])

atom('native-curl-explicit-ca-bundle','Supply an accessible CA bundle to a native curl client','network',None,
     'HTTPS fails with CURL_PEER_FAILED_VERIFICATION in an OH sandbox even though the network is reachable and the server certificate should be trusted.',
     'Give the selected libcurl-based client an app-readable PEM trust bundle and keep certificate and hostname validation enabled.',
     'OH native clients whose selected curl build cannot discover an accessible default trust store. Applies to PC network access under the same sandbox/build conditions; other network engines may use system trust directly.',
     ['CURL_PEER_FAILED_VERIFICATION','CURLOPT_CAINFO','caInfo','PEM','CA bundle','Ktor Curl','TLS','certificate verification failed'],
     ['network-adapter-policy-propagation','application-path-namespace'],
     ['Inspect the actual engine, curl error and trust-store configuration. Separate missing roots from hostname mismatch, expiry, clock errors and an incomplete server chain.',
      'Package a maintained PEM root bundle as a resource. Copy its bytes to an application-context filesDir path readable by native code before constructing the client.',
      'Configure CURLOPT_CAINFO with that path, or the selected Ktor Curl engine\'s engine { caInfo = bundlePath } adapter. Keep peer and hostname verification active.',
      'Propagate the path through the real native initializer and rebuild the client when the trust configuration changes. Report installation/configuration errors explicitly.',
      'Version and update the bundle with the app. Treat private enterprise roots as explicit trust policy; do not disable validation to make a failing request succeed.'],
     'This is conditional on the selected curl engine and package. A path visible to hdc shell may be inaccessible to the application. A custom bundle can change trust relative to the platform store; verify intended public and enterprise endpoints independently.',
     'Do not set verification flags off, confuse a resource URI with an fopen-readable path, or assume INTERNET permission also supplies root certificates.',
     'Test a valid chain, an untrusted root, wrong hostname and expired certificate. Confirm the native client succeeds only for permitted chains and still works after restart and a bundle update.',
     [('kamel-ohos','kamel-core/src/ohosMain/kotlin/io/kamel/core/cache/HttpClientFactory.ohos.kt'),('kamel-ohos','kamel-core/src/commonMain/kotlin/io/kamel/core/Kamel.kt'),('kamel-ohos','example/harmonyApp/entry/src/main/ets/pages/Index.ets')],
     ['The source attributes its failure to inaccessible system roots; this is not a universal claim about all HarmonyOS products or curl builds.'])

atom('kmp-napi-string-allocator-ownership','Free KMP bridge strings with their producing allocator','native','kotlin-multiplatform',
     'Repeated ArkTS calls into a Kotlin/Native library leak native strings, crash during release, or expose pointers after the original buffer is freed.',
     'Copy the native UTF-8 result into a JS string, then release the original with the exact producer-provided disposal function on every result path.',
     'Kotlin/Native OH shared libraries wrapped by a C++ NAPI module. The generated C adapter and its header must match the producing library for the PC target ABI.',
     ['DisposeString','nativeHeap','IoFree','napi_create_string_utf8','CName','UTF-8','double free','allocator','Kotlin/Native'],
     ['kmp-ohos-compose-host','native-ffi-shared-buffer-ownership'],
     ['Inspect the generated header and exported implementation before deciding ownership. A const char* signature alone does not identify the allocator.',
      'For generated Kotlin String results, use that library\'s exported symbol table DisposeString. For an explicit nativeHeap allocation, call its exported free function instead; never substitute the generated disposer or an unrelated C allocator.',
      'Validate null results, then call napi_create_string_utf8 to copy the bytes into a JS-owned value. Release the original whether JS conversion succeeds or fails.',
      'Read JS input length first, allocate length+1 bytes, and copy UTF-8 with the NAPI-reported byte count. Keep input storage alive for the complete synchronous native call.',
      'Use structured errors at the boundary and scope cleanup with RAII where practical. Move blocking native work to a worker without passing napi_env or napi_value objects between threads.'],
     'The reviewed adapters use two different ownership contracts: generated String/DisposeString and explicit UTF-8 buffer/IoFree. They are alternatives, not interchangeable APIs. Returning a copied JS string does not make a base64 bridge zero-copy; large payloads need a bounded binary transport.',
     'Do not free generated Kotlin strings with free(), return stack-backed UTF-8 pointers, release before the NAPI copy, or claim JVM tests validate the native ownership boundary.',
     'Exercise empty, non-ASCII, error and long results repeatedly. Instrument native allocation/release counts, force JS conversion failure where feasible, and check that every owned result is released once.',
     [('kamel-ohos','example/nativeApp/src/ohosMain/kotlin/Bridge.kt'),('kamel-ohos','example/harmonyApp/entry/src/main/cpp/napi_init.cpp'),('kotlinx-io-ohos','example/nativeApp/src/ohosArm64Main/kotlin/org/jetbrains/kotlinx/io/example/NativeBridge.kt'),('kotlinx-io-ohos','example/ohosApp/entry/src/main/cpp/napi_init.cpp')])

atom('kmp-native-image-core-without-compose','Reuse KMP image loading when the Compose decoder is unavailable','media','kotlin-multiplatform',
     'A KMP image library cannot compile its Compose or Skia UI on the selected OH toolchain, while its fetch and cache logic remains reusable.',
     'Build the non-UI fetch/cache core for the OH target and send encoded image bytes to an ArkTS system decoder with an explicit feature contract.',
     'KMP libraries with separable fetch/cache and Compose/Skia layers. Requires compatible OH Kotlin, coroutines, IO and HTTP artifacts; PC display correctness must be measured on the selected decoder.',
     ['Compose','Skia','Kamel','PixelMap','image kit','ohosArm64','ohosX64','source sets','base64','runBlocking'],
     ['kmp-ohos-dependency-and-renderer','native-curl-explicit-ca-bundle','kmp-napi-string-allocator-ownership'],
     ['Audit source imports and separate Compose/Skia types from the reusable core. Place unsupported UI sources outside every active target source set; do not leave an indirect Compose dependency in the core API.',
      'Add compatible OH targets and platform actuals for filesystem/cache and the HTTP engine. Align the compiler metadata generation across all OH dependencies.',
      'Export the core through one Kotlin/Native bridge library and a registered C++ NAPI wrapper. Define byte payloads, cache metadata and errors rather than pretending to return a Compose Painter.',
      'Run a synchronous runBlocking/network bridge on a supported ArkTS worker. Transfer serializable request/results and respect the native library\'s concurrency and shutdown contract.',
      'Decode the returned encoded bytes with image.createImageSource and createPixelMap, then publish the PixelMap to ArkUI. Release replaced image sources and pixel maps.',
      'Expose retained and lost features: raw-byte caching is distinct from bitmap caching; a single response lacks streaming progress, and a first GIF frame is distinct from animation.'],
     'This restores a loading/caching capability through an ArkUI adapter. It does not establish compatibility of KamelImage, asyncPainterResource or a full Compose renderer. Base64 expands payloads and raises memory peaks; prefer bounded binary transfer when the bridge supports it.',
     'Do not count compilation of JVM tests as OH runtime validation, label a worker-backed call nonblocking at the C ABI, or report a cache hit merely because the HTTP engine labels the result network.',
     'Test HTTPS, corrupt images, cancellation/late responses, SVG differences, animated formats and cache behavior across process restart. Measure peak memory and verify resize/DPI behavior in the actual PC window.',
     [('kamel-ohos','kamel-core/build.gradle.kts'),('kamel-ohos','settings.gradle.kts'),('kamel-ohos','example/nativeApp/src/ohosMain/kotlin/Bridge.kt'),('kamel-ohos','example/harmonyApp/entry/src/main/ets/workers/KamelWorker.ets'),('kamel-ohos','example/harmonyApp/entry/src/main/ets/pages/Index.ets')],
     ['No full Compose UI is provided by this adaptation. Source device/simulator reports were not rerun.'])

atom('qt-ohos-clipboard-gui-thread','Read the OH Qt clipboard from the GUI thread','input','qt',
     'A Qt or PyQt clipboard-import action hangs or never opens its result window when QClipboard is read inside a worker thread.',
     'Perform the clipboard access and result dispatch on the Qt GUI thread; keep background parsing or polling separate from the QPA clipboard bridge.',
     'Qt 5/PyQt5 OH QPA integrations that marshal pasteboard access through the ArkTS bridge. PC tray and menu actions also need foreground and pasteboard authorization checks.',
     ['QClipboard','QThread','GUI thread','READ_PASTEBOARD','tray','clipboard import','deadlock','PyQt5'],
     ['qt-ohos-external-clipboard-paste','media-save-and-clipboard-permissions'],
     ['Identify the thread that executes the actual QClipboard call, including timer and tray-triggered helper paths.',
      'Route the read to an object living on the Qt GUI thread, using a queued signal when the initiator is a worker. Read the clipboard and dispatch the UI result from that slot.',
      'Connect completion handlers before starting any remaining worker. Pass a copied text or data payload to background parsing instead of a QClipboard or widget pointer.',
      'For the selected QPA, verify whether READ_PASTEBOARD must be requested by the UIAbility. Manifest declaration and a bridge permission check do not themselves produce a user grant.',
      'Remove worker-based automatic clipboard polling if it cannot meet the thread contract; retain an explicit user action and handle denial/empty content without blocking.'],
     'This is a scoped remedy for an identified worker-thread access path. An unrelated external-paste failure can still be a permission, MIME, focus or QPA-version issue. The reviewed permission request is asynchronous; wait for the result before relying on access.',
     'Do not use a blocking cross-thread callback while holding a GUI lock, start a thread before connecting its result, or infer external clipboard support from in-app copying alone.',
     'Copy text from another app, trigger import through both the menu and tray, and verify one responsive GUI result. Test denial, an empty clipboard, repeated actions and background/foreground transitions.',
     [('persepolis-ohos',PBASE+'resources/resfile/persepolis/launcher.py'),('persepolis-ohos',PBASE+'ets/qability/QAbility.ets')],
     ['The separate editor clipboard gap remains unresolved; this review does not demonstrate a universal QPA fix.'])

atom('qt-pyqt-ohos-subwindow-adoption','Give persistent PyQt subwindows an owner before native creation','window','qt',
     'A parentless PyQt utility window opens as an independent OH main window or follows the wrong QPA window route.',
     'Assign the existing main window as owner and the intended dialog type before the utility window is first shown, preserving its geometry and excluding transient system-managed windows.',
     'PyQt5 with an OH QPA that routes owned Qt dialogs to SubWindow. This route is version-specific and must be checked under PC move, resize, maximize and tray activation.',
     ['PyQt5','setParent','Qt.Dialog','SubWindow','QMessageBox','QDialog.exec_','QWidget.show','window owner'],
     ['qt6-ohos-subwindow-surfaces','qt-ohos-embedded-file-dialogs'],
     ['Create the main window first, then assign ownership to persistent utility windows before show() or native handle creation.',
      'Filter eligible parentless Qt.Window/Qt.Dialog instances; skip the main window, already-owned widgets, menus, popups and tooltips.',
      'Save geometry before setParent(mainWindow), set the intended dialog window flag, and restore geometry in the coordinate space required by the selected QPA.',
      'Prefer explicit constructors or a narrow factory. If legacy code needs interposition, cover both show() and QDialog.exec_() because modal dialogs may bypass the Python show override.',
      'Keep QMessageBox outside a blanket adoption hook: reparenting and geometry changes during a tray action can collapse its platform window. Test modal message boxes independently.'],
     'The reviewed adapter translates saved global coordinates relative to the main window. That is a source-specific workaround, not a guarantee for every QPA. Do not replace a working native picker or modal-box route solely to make all windows share one implementation.',
     'Do not reparent after the native window has been created, blanket-adopt every top-level widget, or reuse pre-parenting coordinates without checking their space.',
     'Open persistent utility windows and modal dialogs from menus and tray actions; verify ownership, visible content, geometry and close behavior. Repeat after moving/maximizing the main window and changing display scale.',
     [('persepolis-ohos',PBASE+'resources/resfile/persepolis/launcher.py'),('persepolis-ohos',PBASE+'ets/pages/SubWindowNativeNode.ets'),('persepolis-ohos','README.md')])

atom('native-ohos-worker-stack-budget','Set a native worker stack budget for deep Kotlin calls','native',None,
     'A deep Kotlin/Native call chain overflows an OH pthread worker stack although the same work succeeds on another host thread.',
     'Set an explicit measured stack size before pthread creation, check each creation failure, and preserve deterministic thread ownership and join behavior.',
     'Native OH worker threads that run deep generated/runtime call chains, including PC render workers. Stack defaults and memory budgets are implementation-dependent; no fixed size is universally required.',
     ['pthread_attr_setstacksize','stack overflow','Kotlin/Native','8 MiB','pthread_create','std::thread','worker stack'],
     ['native-event-loop-handle-teardown','async-bridge-shutdown-wait'],
     ['Confirm stack exhaustion from the crash evidence and call depth. Exclude use-after-free and unbounded recursion before increasing the budget.',
      'Initialize pthread_attr_t and set the stack size before pthread_create. The reviewed worker uses 8 * 1024 * 1024 bytes; choose a budget for the actual runtime and concurrent worker count.',
      'Check pthread_attr_init, pthread_attr_setstacksize and pthread_create return codes. Destroy attributes and free the heap-owned callable on every failed creation path.',
      'Transfer the callable to the worker only after successful creation. Join it before destroying state captured by the callable.',
      'Reject self-join and double join. Preserve the surrounding loop\'s startup readiness handshake and stop/close sequence when replacing std::thread.'],
     'The source reports an approximately 1 MiB default for its HarmonyOS build and uses 8 MiB for a deep Kotlin/Native workload. Neither value is a product-wide default or a benchmark. Retaining joinable-thread failure semantics avoids silently detaching code that still uses owner state.',
     'Do not increase every worker stack without considering memory use, lose callable ownership on pthread_create failure, or join the worker from itself.',
     'Reproduce the deep workload on the intended PC, measure stack headroom and total memory at maximum worker count, and exercise creation failure and shutdown with queued work. Run long enough to exclude merely delaying an unbounded recursion failure.',
     [('kuiklyui',KBASE+'CustomStackSizeThread.h'),('kuiklyui',KBASE+'KRThread.h'),('kuiklyui',KBASE+'KRThread.cpp'),('kuiklyui','docs/ChangeLog/2.29.0/changelog.md')],
     ['New changelog linked to an already-present implementation; no performance or crash reproduction was run during curation.'])

atom('native-bridge-stale-handle-identity','Reject stale bridge handles even when native addresses are reused','native',None,
     'A late ArkTS or channel callback controls or destroys a newly created native object after an older object was released.',
     'Expose generation-safe IDs backed by a live-object registry instead of using a native pointer address as the public object identity.',
     'In-process OH native bridges that create and destroy objects asynchronously. Relevant to PC media/window recreation; adapters must preserve their own thread and numeric-transport limits.',
     ['stale handle','ABA','pointer reuse','generation','monotonic ID','native registry','NAPI','unknown player'],
     ['flutter-ohos-texture-native-window-lifetime','native-event-loop-handle-teardown'],
     ['Allocate a new ID for each object lifetime. Use a monotonic counter that cannot wrap during the process lifetime or an index plus checked generation.',
      'Store ID-to-live-object mappings internally. Resolve every invoke, render, event, detach and destroy request through that registry.',
      'After destruction invalidate the mapping; a repeated destroy can be a documented no-op, while other stale requests return an explicit unknown-object error.',
      'Never recover an object by casting a caller-supplied integer back to a pointer. Keep the pointer private to the registry.',
      'Serialize registry mutation or add the synchronization required by the calling model. An atomic ID counter alone does not make an unordered_map thread-safe.',
      'Keep IDs exact across JS transport: stay within Number.MAX_SAFE_INTEGER when using a number, or choose a supported string/BigInt representation with explicit bounds.'],
     'This addresses lifetime identity, not permissions or ownership retention. A valid handle still needs an object lifetime guard while work is in flight. The reviewed implementation uses a process-wide monotonic int64 counter; its map assumes the bridge calling model supplies serialization.',
     'Do not use an object address as a stable public ID, reuse a free-list slot without a generation, or treat atomic ID allocation as complete thread safety.',
     'Create and destroy many objects while retaining old IDs. Force allocator-address reuse in a controlled harness and send late invoke/destroy calls; the new object must survive. Check counter bounds and concurrent registry access separately.',
     [('liberika',LBASE+'ohos/src/main/cpp/erika_flutter_plugin.cpp'),('liberika','docs/investigations/pr-150-159/ohos-runtime.zh.md')],
     ['Upstream reports a deterministic forced-address-reuse harness and simulator stale-ID checks. Its real-run baseline did not reproduce allocator address reuse.'])

atom('flutter-ohos-texture-native-window-lifetime','Bind a Flutter OH texture to an owned native window','media','flutter',
     'A custom Flutter OH video texture remains black, breaks after resizing, or crashes when its route or engine is destroyed.',
     'Register the texture surface, attach its surface ID through an owned OHNativeWindow, and serialize resize, detach and release across Dart, ArkTS and native code.',
     'Flutter OH embeddings exposing TextureRegistry and SurfaceTextureEntry, plus a renderer supporting OHNativeWindow output. PC DPI, window resizing and texture composition require target validation.',
     ['TextureRegistry','SurfaceTextureEntry','getSurfaceId','OHNativeWindow','CreateNativeWindowFromSurfaceId','unregisterTexture','devicePixelRatio','generation'],
     ['video-surface-backend-contract','flutter-player-surface-reconstruction','flutter-player-async-teardown','native-bridge-stale-handle-identity'],
     ['Use LayoutBuilder and devicePixelRatio to calculate a valid physical buffer size from the logical view. Keep width/height units and the separate renderer scale explicit.',
      'Obtain and register a texture ID, setTextureBufferSize, then retain its SurfaceTextureEntry and getSurfaceId result in the ArkTS adapter.',
      'Create OHNativeWindow from the surface ID in native code. Attach it to the renderer and store ownership only after successful attachment; destroy the new window immediately if attachment fails.',
      'On resize update both the embedding buffer size and the renderer surface metrics. Coalesce async work and invalidate late results with a view generation; release a texture whose creation completes after disposal.',
      'On replacement detach the previous renderer before binding the new player. On release detach the renderer, destroy the owned native window, then unregister the embedding texture.',
      'On engine teardown stop frame polling, release all texture bindings and native players, then clear channel/event handlers. Repeated release must not touch a replacement instance.'],
     'Texture registration does not prove displayed frames or hardware zero-copy. The upstream runtime report exercised the production native bridge in a simulator using XComponents, without starting the full Flutter engine; it does not verify Dart-to-texture composition on a PC.',
     'Do not unregister while the renderer still owns the window, multiply physical size by DPI twice, allow stale async creation to attach after disposal, or infer physical hardware decoding from a decoder label.',
     'In a complete Flutter app on the intended PC, verify first frame, clipping/overlays, repeated resize/DPI changes, rapid player switches, route disposal during creation and engine shutdown. Track texture/window counts and inspect actual rendered frames.',
     [('liberika',LBASE+'lib/src/erika_video_view.dart'),('liberika',LBASE+'ohos/src/main/ets/components/plugin/ErikaFlutterPlugin.ets'),('liberika',LBASE+'ohos/src/main/cpp/erika_flutter_plugin.cpp'),('liberika',LBASE+'ohos/src/main/cpp/CMakeLists.txt'),('liberika',LBASE+'pubspec.yaml'),('liberika','docs/investigations/pr-150-159/ohos-runtime.zh.md')])

extended = []
def extend(id, old, new, evidence, related=()):
    path = DB/'knowledge'/f'{id}.md'
    content = path.read_text()
    raw, body = content.split('---\n', 2)[1:]
    meta = json.loads(raw)
    if old not in body and new not in body:
        raise ValueError('Missing extension anchor: '+id)
    body = body.replace(old, new)
    meta['related'] = list(dict.fromkeys(meta['related']+list(related)))
    path.write_text('---\n'+json.dumps(meta,ensure_ascii=False,indent=2)+'\n---\n'+body)
    provpath = DB/'provenance'/f'{id}.json'
    prov = json.loads(provpath.read_text())
    for source_id,p in evidence:
        record=snapshot(source_id,p)
        if not any(s.get('snapshot')==record['snapshot'] for s in prov['sources']):prov['sources'].append(record)
    note='Extended the existing mechanism from independently reviewed source; original imported evidence and PC validation boundary retained.'
    if note not in prov['transformations']:prov['transformations'].append(note)
    dump(provpath,prov)
    extended.append(id)

extend('picker-cancellation-concurrency',
       '5. Release the guard, temporary files and any temporarily changed orientation in `finally`. On owner teardown, settle or invalidate its pending request so a late callback cannot reply twice.',
       '5. Release the presentation guard when the native dialog actually settles, and clean temporary files and presentation changes on the matching outcome. Cancelling a coroutine or timing out the caller may not dismiss system UI: keep the guard until that still-open dialog settles, discard its late selection, and close any abandoned descriptors. On owner teardown, cancel or invalidate the pending continuation so a late callback cannot reply twice.',
       [('filekit-ohos',DBASE+'OhosSystemDialogs.kt')],['kmp-filekit-picker-descriptor-ownership'])
extend('media-save-and-clipboard-permissions',
       '3. Declare permissions with appropriate user-facing reasons and request runtime grants only when needed. Keep Android-only permission branches on Android.',
       '3. Declare permissions with appropriate user-facing reasons and request runtime grants only when needed. For a backend that checks `READ_PASTEBOARD` but never requests it, use the owning UIAbility context with `abilityAccessCtrl.createAtManager().requestPermissionsFromUser(...)`, await the result, and handle denial. A manifest declaration alone leaves a user-grant permission ungranted. Keep Android-only permission branches on Android.',
       [('persepolis-ohos',PBASE+'ets/qability/QAbility.ets')],['qt-ohos-clipboard-gui-thread'])
extend('qt-ohos-external-clipboard-paste',
       'There is no confirmed general remedy in the reviewed source. The failure may be in the QPA bridge, the system pasteboard policy or the app\'s paste binding.',
       'There is no confirmed general remedy for this unresolved editor failure. When diagnosis identifies a worker-thread QClipboard call or an unrequested pasteboard grant, apply the separately scoped [GUI-thread clipboard contract](qt-ohos-clipboard-gui-thread.md) and [operation-specific permission checks](media-save-and-clipboard-permissions.md). These conditions were found in a different integration and do not establish that they repair this editor\'s bridge, MIME or paste binding.',
       [('persepolis-ohos',PBASE+'resources/resfile/persepolis/launcher.py'),('persepolis-ohos',PBASE+'ets/qability/QAbility.ets')],['qt-ohos-clipboard-gui-thread','media-save-and-clipboard-permissions'])
dep=DB/'knowledge/kmp-ohos-dependency-and-renderer.md'
text=dep.read_text()
anchor='## Common mistakes'
addition='A failure such as `metadata version is 2.4.0, but the compiler version 2.2.0 can read up to 2.3.0` requires aligning the OH compiler and dependency generation, rather than changing only the application target. One reviewed dependency set uses HarmonyOS Kotlin `2.4.20-0.1.0-07` with its OH coroutines/okio artifacts and Ktor `3.5.1-0.1.0-01`; this is a pinned example, not a universal upgrade prescription. Verify actual artifacts, target variants and the compiler distribution, since a Maven metadata listing alone does not prove the binary exists.\n\n'
if addition not in text:text=text.replace(anchor,addition+anchor);dep.write_text(text)
extend('kmp-ohos-dependency-and-renderer','## Common mistakes','## Common mistakes',
       [('kamel-ohos','gradle/libs.versions.toml'),('kamel-ohos','settings.gradle.kts'),('kamel-ohos','README.OpenHarmony_CN.md')],['kmp-native-image-core-without-compose'])

for source_id,path in [('filekit-ohos','LICENSE'),('filekit-ohos','samples/ohos-integration/README.md'),('liberika','LICENSE'),('liberika',LBASE+'README.md'),('kamel-ohos','LICENSE'),('persepolis-ohos','README.md'),('kotlinx-io-ohos','LICENSE')]:
    snapshot(source_id,path)
dump(REVIEW/'curation-manifest.json',dict(date=DATE,added_entries=atoms,extended_entries=extended,sources=SOURCES,snapshots=list(records.values())))
print(json.dumps(dict(added=atoms,snapshots=len(records)),indent=2))

"""Persist curator-authored guidance with exact, commit-pinned source evidence."""
import hashlib
import json
from pathlib import Path
import subprocess
from read_source import ROWS

HERE=Path(__file__).resolve().parent
DB=HERE.parents[2]
DATE='2026-10-27'
LICENSES={
 'kuiklyui':'KuiklyUI License; custom license, existing notice retained.',
 'liberika':'MPL-2.0; component notices apply.',
 'tauri-gproxy':'AGPL-3.0-or-later',
 'expo-device-ohos':'MIT', 'kamel-ohos':'Apache-2.0',
 'cc-switch-ohos':'MIT', 'qterminal-ohos':'GPL-2.0-or-later; bundled font has separate terms.',
 'strawberry-ohos':'GPL-3.0', 'letos-ohos':'GPL-3.0',
 'rnoh-keep-awake':'MPL-2.0', 'rnoh-splash-screen':'MIT',
 'electron-shell':'No repository-level license found; vendor files carry their own headers.',
 'coil-ohos':'Apache-2.0',
 'flutter-add2app':'No repository-level license found; reviewed FlutterUI.ets carries Apache-2.0 header; module manifest says OpenValley.',
 'flutter-record-ohos':'record_ohos/LICENSE is BSD-3-Clause; reviewed ArkTS files carry Apache-2.0 headers.',
 'flutter-app-links':'MIT',
 'marktext-ohos':'MIT root notice; vendor bridge files carry Huawei BSD-style headers. No blanket relicensing inferred.',
 'vbsqlite-ohos':'No repository-level license found.', 'kuikly-sqlite':'MIT; bundled SQLite has its own public-domain notice.',
 'rnoh-gesture-handler':'MIT',
}
SNAPS={}
ADDED=[]
EXTENDED=[]

def dump(path,data):
 path.parent.mkdir(parents=True,exist_ok=True)
 path.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n')

def snap(sid,path):
 key=(sid,path)
 if key in SNAPS:return SNAPS[key]
 r=ROWS[sid]
 data=subprocess.check_output(['git','-C',r['local_path'],'show',r['observed_commit']+':'+path],stderr=subprocess.PIPE)
 target=DB/'provenance/snapshots/projects'/sid/DATE/path
 target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
 record=dict(collection=sid,original_path=path,snapshot=str(target.relative_to(DB)),sha256=hashlib.sha256(data).hexdigest(),revision=r['observed_commit'],license=LICENSES[sid],url=r['url'],evidence_note='Pinned source review only; no source setup, build, device or PC execution.')
 SNAPS[key]=record
 return record

def add(id,title,area,frameworks,summary,symptom,app,keywords,related,steps,conditions,mistakes,checks,evidence,status='scoped',limitations=()):
 meta=dict(schema_version=1,id=id,title=title,topic_key=area+'.'+id,area=area,scope='framework-specific' if frameworks else 'common',frameworks=frameworks,summary=summary,when_to_use=symptom,applicability=app,keywords=keywords.split('|'),related=related,status=status,evidence_tier='source-reviewed',pc_validation='not-run')
 body=f'---\n{json.dumps(meta,ensure_ascii=False,indent=2)}\n---\n\n# {title}\n\n## When to use\n\n{symptom}\n\n## Recommended approach\n\n{summary}\n\n## Implementation steps\n\n'+ '\n'.join(f'{i}. {s}' for i,s in enumerate(steps,1))+f'\n\n## Conditions and alternatives\n\n{conditions}\n\n## Common mistakes\n\n{mistakes}\n\n## Check it works\n\n{checks}\n'
 (DB/'knowledge'/f'{id}.md').write_text(body)
 dump(DB/'provenance'/f'{id}.json',dict(schema_version=1,id=id,sources=[snap(*v) for v in evidence],transformations=['Generalized the reviewed mechanism and removed project/research identity from consumer guidance.','Checks describe proposed acceptance cases; they were not executed.']+(['Common classification is a curator inference from the native/platform contract, not corroboration on other frameworks.'] if not frameworks else []),limitations=['No application build, device, emulator or PC validation performed.','Selected implementation paths only; published-package behavior is not established.']+list(limitations)))
 ADDED.append(id)

def extend(id,evidence,note,extra=None):
 p=DB/'provenance'/f'{id}.json';obj=json.loads(p.read_text())
 for v in evidence:
  s=snap(*v)
  if s not in obj['sources']:obj['sources'].append(s)
 obj['transformations'].append(note)
 dump(p,obj)
 if extra:
  p=DB/'knowledge'/f'{id}.md';p.write_text(p.read_text().replace('## Common mistakes',extra+'\n\n## Common mistakes'))
 EXTENDED.append(id)

K='core-render-ohos/src/main/cpp/libohos_render/'
S='harmony/splash_screen/src/main/ets/SplashScreen.ts'
R='record_ohos/ohos/src/main/ets/components/plugin/'
G='react-native-harmony-gesture-handler/harmony/gesture_handler/src/main/cpp/'

add('native-arkui-node-attribute-api-guard','Guard native ArkUI attributes by runtime API level','native',[],
 'Check the running OS API before setting a newer ArkUI node attribute; a newer build SDK or exported dispatcher does not establish that its implementation exists.',
 'A native image node crashes at address zero or rejects NODE_IMAGE_SOURCE_SIZE on an older OH image.',
 'Native ArkUI node consumers on OH PC and other products. The reviewed source skips NODE_IMAGE_SOURCE_SIZE below API 21; independent product validation remains required.',
 'NODE_IMAGE_SOURCE_SIZE|OH_GetSdkApiVersion|setAttribute|API 21|null function pointer|image decode size',
 ['hap-minimum-api-compatibility','sdk-version-versus-os-limitation'],
 ['Read `OH_GetSdkApiVersion()` before dispatching the attribute; skip `NODE_IMAGE_SOURCE_SIZE` below 21 in the affected implementation.',
  'Keep the node-handle null check and preserve the renderer\'s ordinary image decode path when the size hint is unavailable.',
  'Check setAttribute results on supported versions and retain width/height units as physical pixels.'],
 'This is an optional decode-size hint. If an attribute is essential to correctness, declare an adequate minimum API or select another backend. Verify thresholds against the selected SDK headers.',
 'Calling a newer attribute and relying on an error return can still jump through an unimplemented compatible-library function.',
 'Exercise the image node below and at API 21, including resize and repeated image replacement. Confirm old images render without the hint and no null dispatch occurs.',
 [('kuiklyui',K+'utils/KRViewUtil.cpp')])

add('native-unicode-selection-boundaries','Expand native text selections on Unicode boundaries','input',[],
 'Compute half-open selection ranges with ICU grapheme, word and sentence breaks, then map that range to rendered glyph boxes without changing its logical endpoints.',
 'Native text selection splits an emoji or combining sequence, selects nothing on punctuation, or starts on the wrong line after a soft wrap.',
 'Custom native text renderers using UTF-16 offsets and ICU on OH PC. Applies to pointer and touch selection; ICU availability and glyph-offset conventions must match the renderer.',
 'ICU|UBRK_CHARACTER|UBRK_WORD|UBRK_SENTENCE|UTF-16|grapheme|emoji|soft wrap|selection',
 ['text-weight-versus-scale-policy'],
 ['Use `ubrk_open`, `ubrk_following`, `ubrk_preceding` and `ubrk_close` over the exact UTF-16 text used by layout.',
  'For word selection, inspect the break rule status and fall back from whitespace or punctuation to the touched grapheme cluster. Define paragraph terminators separately.',
  'Preserve `[start,end)` as the logical range. Derive visible rectangles from the first and last glyph intersecting it; lock the first matching line at soft-wrap boundaries.',
  'Keep drag-handle character updates separate from initial word/sentence expansion, and validate offsets before applying fallback arithmetic.'],
 'The source links the platform ICU library. Empty text, invalid offsets and failed break-iterator creation require explicit safe handling; a one-code-unit fallback is insufficient for arbitrary Unicode.',
 'Treating UTF-16 code units as characters or deriving logical end offsets from zero-width line terminator boxes corrupts the selected range.',
 'Test CJK, surrogate pairs, combining marks, ZWJ emoji, punctuation, CR/LF and wrapped spans; copied text must equal the requested logical interval.',
 [('kuiklyui',K+'expand/components/richtext/KRRichTextView.cpp'),('kuiklyui','core-render-ohos/src/main/cpp/CMakeLists.txt')])

add('kmp-kuikly-statusbar-scroll-ownership','Give the OH renderer one owner for status-bar scroll-to-top','input',['kotlin-multiplatform'],
 'Disable the built-in node back-to-top action when the renderer owns the status-bar event, and let each active scroll container choose callback handling or default scrolling.',
 'A status-bar click scrolls twice, ignores a business callback, or moves a hidden KMP page.',
 'Kuikly native OH renderer revisions with common-event integration. Relevant only on PC/window configurations that expose a clickable status bar and emit its event; desktop chrome is not assumed equivalent.',
 'Kuikly|COMMON_EVENT_CLICK_STATUSBAR|NODE_SCROLL_BACK_TO_TOP|scrollToTop|active page|API 15',
 ['kmp-kuikly-ohos-host','kmp-kuikly-ohos-resize-sync','native-arkui-node-attribute-api-guard'],
 ['Subscribe by renderer instance identity before its view tree is constructed; release the subscription during destruction.',
  'Marshal delivery onto the owning UI thread and skip paused, hidden or destroyed render instances.',
  'From API 15, set `NODE_SCROLL_BACK_TO_TOP` to false on managed scroll nodes. Repeat this idempotently when a node attaches.',
  'If `scrollToTop` is registered, call it without auto-scrolling; otherwise scroll the container to the origin. Reset the callback when the property is removed.'],
 'Keep existing behavior below the attribute API gate. Multiple visible instances and nested scrollers need an explicit routing policy; the source iterates containers in each active instance.',
 'Leaving both the OS action and custom event active causes duplicate effects; a global subscription without instance cleanup reaches stale views.',
 'Test callback present/absent/reset, hidden tabs, multiple instances, nested lists and repeated mount/destroy. Confirm one event has the intended effect.',
 [('kuiklyui',K+'expand/components/scroller/KRScrollerView.cpp'),('kuiklyui',K+'view/KRRenderView.cpp'),('kuiklyui',K+'expand/events/common_event/KRCommonEventManager.cpp')])

add('native-subwindow-async-transaction','Serialize asynchronous subwindow show and hide','window',[],
 'Put window creation, content loading, showing and hiding on one failure-tolerant Promise chain; report show completion only after the native window is visible.',
 'A hide request runs before an asynchronously created splash window exists, leaving an overlay visible or leaking its image resources.',
 'OH WindowStage-owned subwindows, including PC startup overlays. Window ownership, resize and fullscreen behavior are product/API specific.',
 'createSubWindow|setUIContent|showWindow|destroyWindow|Promise|splash|PixelMap|ImageSource',
 ['webview-controller-and-lifetime','async-bridge-shutdown-wait'],
 ['Associate the operation queue with the owning WindowStage. Await `createSubWindow`, `setUIContent` and `showWindow` in order before resolving show.',
  'Queue hide behind any pending show. Absorb a previous queue rejection into the queue tail while returning the current operation\'s error to its caller.',
  'On any intermediate failure, destroy the partially created window and release owned ImageSource/PixelMap resources through their supported release APIs.',
  'Make repeated show/hide idempotent, detach stale handles before release, and drain queued cleanup during owner teardown. Track asynchronous release completion if the shutdown contract requires it.'],
 'A process-global static window is suitable only for an explicitly single-owner design. A replacement WindowStage needs a new generation so old work cannot publish into it.',
 'Mixing callbacks with an outer Promise that returns after image decode makes completion mean the wrong thing. Clearing a field alone does not release native image memory.',
 'Queue show-hide-show immediately, inject failure at every creation stage, and destroy the owner during decode. Verify no overlay remains and release callbacks complete.',
 [('rnoh-splash-screen',S)],limitations=['The reviewed source has static cached ownership and callback-based image release. Per-WindowStage generations and fully awaited image cleanup are curator recommendations, not claimed source fixes.'])

add('rnoh-splash-native-readiness','Start the RN surface after the native splash is ready','host',['react-native'],
 'Configure the native splash from RNAbility.onWindowStageCreate, wait for its show Promise, then create the React Native surface and hide the splash when application initialization completes.',
 'An RNOH splash never shows, JS show lacks WindowStage configuration, or a startup loop prevents the RN page from loading.',
 'RNOH hosts using a native subwindow splash adapter with the matching package generation. PC overlays need separate resize/focus checks.',
 'RNAbility|SplashScreen.show|onWindowStageCreate|TurboModule|startup|WindowStage',
 ['native-subwindow-async-transaction','rnoh-native-host-and-codegen'],
 ['Register the splash page route and native package for the selected RNOH version.',
  'In `onWindowStageCreate`, supply the Ability context, WindowStage, image Resource, background and route to native show; call the base RNAbility stage creation after that Promise resolves.',
  'Define a show-failure policy that either continues RN startup with diagnostics or reports a startup failure; do not silently leave the RN surface uncreated.',
  'Hide from JS after app readiness and route module destruction through the native serialized cleanup. Initial JS show alone cannot invent native stage configuration.'],
 'The reviewed API exposes void JS show/hide and an awaitable native show. A JS call returning does not prove native teardown finished.',
 'Busy-waiting in stage creation blocks asynchronous content loading; invoking the base creation before a racy splash can hide against an empty window handle.',
 'Cold-start offline, hide immediately after mount, recreate the RN instance and fail image loading. Confirm the RN page starts or an intentional error is surfaced.',
 [('rnoh-splash-screen',S),('rnoh-splash-screen','example/harmony/entry/src/main/ets/entryability/EntryAbility.ets'),('rnoh-splash-screen','harmony/splash_screen/oh-package.json5')])

add('rnoh-window-keep-screen-on','Apply RNOH keep-awake to the owning RN window','window',['react-native'],
 'Resolve rnInstance.getRNWindow() from a UITurboModule and call setWindowKeepScreenOn on that window, with explicit activation and deactivation.',
 'A React Native keep-awake module is missing on OH or affects the wrong window in a PC app.',
 'RNOH UITurboModule implementations with getRNWindow support. Keep-screen-on is a window policy; it does not grant background execution or system-wide wake locks.',
 'KeepAwake|UITurboModule|getRNWindow|setWindowKeepScreenOn|screen timeout',
 ['rnoh-ohos-package-adaptation','continuous-background-task-contract'],
 ['Keep the JS TurboModule name, ArkTS package, C++ package and autolinking configuration aligned.',
  'Resolve `ctx.rnInstance.getRNWindow()` and await the resulting `setWindowKeepScreenOn(true)` operation; catch/log failure if the public JS API is void.',
  'Set false when the feature no longer needs the screen. At app integration level reconcile multiple request owners and window replacement rather than letting one consumer clear another\'s request.'],
 'The reviewed module logs async errors and provides activate/deactivate; it does not implement reference counting or teardown restoration. Add those policies where the application needs them.',
 'Using an arbitrary last window can change the wrong PC window. A void JS call is not evidence that the window accepted the request.',
 'Test two RN windows, rapid activation changes, navigation away and stage recreation. Check idle behavior while visible and confirm ordinary timeout returns afterward.',
 [('rnoh-keep-awake','package.json'),('rnoh-keep-awake','harmony/keep_awake/src/main/ets/KeepAwakeTurboModule.ets')])

add('flutter-ohos-pcm-record-stream','Deliver OH microphone PCM through a Flutter record stream','media',['flutter'],
 'Implement streaming with AudioCapturer readData callbacks and a per-recorder EventChannel, copying each callback buffer before delivery and awaiting capture startup.',
 'Flutter file recording works on OH but startStream returns no bytes, reuses corrupted buffers, or reports success before AudioCapturer starts.',
 'Flutter OH record adapters using raw PCM16 capture. On PC, microphone permission, actual input routes, background policy and device-supported formats remain separate checks.',
 'startStream|AudioCapturer|readData|pcm16bits|SAMPLE_FORMAT_S16LE|ENCODING_TYPE_RAW|EventChannel|microphone',
 ['audio-session-concurrency-policy','native-event-loop-handle-teardown','plugin-graph-source-and-api'],
 ['Reject unsupported stream encoders explicitly. Build AudioCapturerOptions with supported rate/channel values, `SAMPLE_FORMAT_S16LE`, `ENCODING_TYPE_RAW` and microphone source.',
  'Create the capturer, register readData before starting, then acknowledge the MethodChannel request only after start succeeds.',
  'Copy each nonempty buffer with `new Uint8Array(buffer).slice()` and publish it to the recorder-specific bytes EventChannel. Keep recording state on its separate event channel.',
  'For logical pause, discard callbacks while paused; the hardware may remain active. On stop/dispose, disable delivery, remove the same readData callback, stop and release before reusing the capturer.',
  'Serialize mode switches and settle failed startup with cleanup. Do not infer streaming encoder support from an API that unconditionally reports encoder availability.'],
 'Raw PCM is not AAC/WAV. Consumer framing and backpressure must be explicit; the source forwards chunks without a bounded queue. Source cleanup serialization is useful but does not make all plugin lifecycle races fixed.',
 'Retaining the callback\'s native buffer or returning success immediately after creating the recorder can produce corrupted chunks or silent startup failure.',
 'Check first nonempty PCM bytes, sample rate/channel framing, denied permission, unsupported encoder, rapid stop/start, pause/resume and stream-to-file switching. Monitor memory with a slow consumer.',
 [('flutter-record-ohos',R+'AudioStreamRecorder.ets'),('flutter-record-ohos',R+'AudioUtils.ets'),('flutter-record-ohos',R+'RecorderWrapper.ets'),('flutter-record-ohos',R+'RecorderRecordStreamHandler.ets'),('flutter-record-ohos','record_ohos/pubspec.yaml')],limitations=['No microphone or background behavior tested. The source wrapper has void disposal paths; complete teardown/error recovery and backpressure require application review.'])

add('flutter-ohos-add-to-app-container-lifecycle','Forward lifecycle for an embedded Flutter OH container','host',['flutter'],
 'Register the native Ability and WindowStage with FlutterManager, initialize the shared engine and plugins, and forward the embedded FlutterBoostEntry container lifecycle and visibility.',
 'Flutter embedded in an ArkUI tab stays blank, loses plugins, or keeps running as visible after a native tab switch.',
 'Flutter OH embeddings exposing FlutterManager, FlutterPage and a matching OH FlutterBoost adapter. The source is a mobile-origin example; PC resizing, focus and multiple windows were not validated.',
 'add-to-app|FlutterBoostEntry|FlutterManager|FlutterPage|GeneratedPluginRegistrant|onVisibleAreaChange|tab visibility',
 ['native-host-and-shared-dart','flutter-engine-cache-coherence','viewport-scale-and-layout-lifetime'],
 ['Use a compatible Flutter module HAR, engine HAR and FlutterBoost implementation; register generated plugins on the initialized engine.',
  'Pair `pushUIAbility`/`popUIAbility` with Ability creation/destruction, and `pushWindowStage`/`popWindowStage` with stage creation/destruction.',
  'Construct one FlutterBoostEntry for each container route and arguments. Call its appear hook before obtaining FlutterView and pass the matching view ID to FlutterPage.',
  'Forward page-show/hide and container-disappear hooks. For an embedded tab, use visibility notifications because an ArkUI child is not necessarily a routed page.',
  'Guard deferred visibility callbacks against disposed containers and coordinate shared-engine ownership across concurrent containers.'],
 'These APIs are embedding-version specific. The source shares an engine through FlutterBoost; do not replace a modern host template with its old API names without checking the selected SDK.',
 'Only forwarding UIAbility foreground/background misses tab visibility. Reusing a view ID from a destroyed container can reconnect the wrong surface.',
 'Switch tabs repeatedly, open native/Flutter routes, destroy a stage with a pending callback and resize a PC window. Verify plugin calls and engine/container counts.',
 [('flutter-add2app','ohos_app/entry/src/main/ets/entryability/EntryAbility.ets'),('flutter-add2app','ohos_app/entry/src/main/ets/pages/FlutterUI.ets'),('flutter-add2app','ohos_app/modules/flutter_module/oh-package.json5')])

add('tauri-ohos-web-storage-fallback','Handle unavailable Web Storage before Tauri frontend imports','storage',['tauri'],
 'Probe Web Storage before frontend modules execute, provide an explicitly temporary fallback when needed, and route durable preferences through a supported backend store.',
 'A Tauri OH frontend crashes during module initialization because localStorage is null under its custom scheme, or preferences disappear after relaunch.',
 'Experimental Tauri OH/ArkWeb custom-scheme generations exhibiting unavailable Web Storage. This is a backend/version condition, not a claim that all ArkWeb schemes lack persistence; PC validation remains unrun.',
 'localStorage|null|tauri://localhost|ArkWeb|module initialization|memory fallback|preferences',
 ['tauri-ohos-ability-host','file-secret-store-guarantees'],
 ['Probe storage access and a small write/read/remove operation inside try/catch before the first module that reads preferences.',
  'If startup requires a synchronous fallback, install it before ES module evaluation and implement the Storage methods your app uses with string key/value semantics.',
  'Label in-memory storage as session-only. For durable data, use a registered OH-compatible store or app-owned sandbox file API and reconcile initialization before dependent features read it.',
  'Test durable reload independently from initial rendering; keep original data intact during migration.'],
 'The reviewed inline polyfill prevents null access and explicitly loses data on restart. It is not proof of a persistent store or a fully conforming Storage replacement.',
 'Adding a fallback after imports is too late for top-level i18n/preference access. A rendered UI does not establish persistence.',
 'Test null storage, throwing getters, module startup and a full application restart. Confirm temporary versus durable values follow the intended contract.',
 [('cc-switch-ohos','src/index.html'),('cc-switch-ohos','README.md')])

add('tauri-ohos-platform-plugin-gates','Gate Tauri OH plugins across dependencies and commands','plugins',['tauri'],
 'Select OH-compatible plugin dependencies, gate plugin registration and command implementations together, and expose unsupported operations as explicit UI capability states.',
 'A Tauri OH build selects Linux-only dependencies, or the app renders but tray, updater, dialog and opener calls fail at runtime.',
 'Tauri OH experimental branches where target_os is linux and target_env is ohos. Individual plugin/backend support differs by revision; this guidance does not declare those features universally unsupported.',
 'target_env = ohos|cfg desktop|tauri-plugin-dialog|updater|capabilities|ACL|webkit2gtk',
 ['rust-ohos-target-cfg','tauri-ohos-dependency-cohort','tauri-ohos-plugin-command-registration'],
 ['For desktop Linux dependencies, exclude `target_env = "ohos"` explicitly. Match the application manifest, lockfile and framework patch versions.',
  'Gate imports, Builder.plugin registration and affected command bodies using the same actual backend availability; inspect how the selected fork defines desktop/mobile cfgs.',
  'Use an OH capability file with the fork\'s `openHarmony` platform identifier and only supported permissions. An ACL entry cannot create a missing native implementation.',
  'Return a clear unsupported result from unavailable operations and disable/explain the matching UI. Retain a functional quit/restore path if a tray backend is absent.'],
 'Runtime feature probing may be needed even when compilation succeeds. A rejection from an unregistered plugin is not evidence that the entire application is broken.',
 'Only gating Cargo dependencies leaves unconditional Rust imports or frontend invocations. Treating the Linux target_os label as desktop Linux selects the wrong stack.',
 'Inspect resolved dependency sources and exercise each exposed command on the intended OS build. Regress supported desktop targets after changing conditional code.',
 [('cc-switch-ohos','src-tauri/Cargo.toml'),('cc-switch-ohos','src-tauri/Cargo.lock'),('cc-switch-ohos','src-tauri/src/lib.rs'),('cc-switch-ohos','src-tauri/capabilities/ohos.json')])

add('electron-ohos-sessionmanager-prerequisite','Check the Electron window-session prerequisite on OH','host',['electron'],
 'Check product window capabilities and native display initialization separately from library/symbol presence; ArkTS guards alone cannot repair an unsupported prebuilt Electron window backend.',
 'An Electron OH HAP installs, aa start succeeds, ArkTS reports 801 window errors and CrBrowserMain then crashes with an invalid display.',
 'The reviewed Electron v37 OH host on a default-form-factor OpenHarmony image lacking Window.SessionManager. A same-source 2in1 success report is not current PC verification.',
 'SystemCapability.Window.SessionManager|801|CrBrowserMain|OH_WindowManager_GetWindowProperties|display id -1|SIGSEGV',
 ['electron-ohos-platform-capability-gates','ohos-native-form-factor-classification','native-dependency-closure'],
 ['Record the exact OS/API/product capability set and runtime revision; compare Window.SessionManager with WindowManager.Core support.',
  'Capture ArkTS window errors, native display IDs and fault logs. Verify native library dependencies and symbols independently rather than assuming a dlopen problem.',
  'Guard optional ArkTS window-decor/status calls with capability checks and async error handling, then inspect whether the native runtime still requires the absent service.',
  'Use a compatible product image or a runtime with an implemented/tested fallback. Keep the source case unresolved until startup and window behavior pass on that combination.'],
 'The source diagnosis is a maintainer report supported by host guards and logs; the prebuilt native runtime implementation was not independently audited. A display-ID fallback is proposed, not demonstrated.',
 'An exported NDK function may be present while its product capability is unavailable. Catching 801 prevents only the ArkTS exception, not a later native failure.',
 'On each intended PC/product image, verify process survival, a usable first frame, valid display identity, resize and window lifecycle. aa start acceptance alone is insufficient.',
 [('electron-shell','docs/ohos-sessionmanager-adaptation.md'),('electron-shell','web_engine/src/main/ets/ability/WebAbility.ets'),('electron-shell','web_engine/src/main/ets/components/WebNodeHandleWindow.ets'),('electron-shell','electron/oh-package.json5')],status='unresolved',limitations=['Native crash causality and same-HAP 2in1 success are source-reported; no native binary or device reproduction performed.'])

add('native-gstreamer-plugin-registry','Locate GStreamer plugins and its writable registry in an OH HAP','media',[],
 'Derive the packaged plugin directory from the loaded application library, set GStreamer search paths before initialization, and place its generated registry in writable sandbox storage.',
 'A native OH media player cannot discover packaged GStreamer elements or tries to write its registry into the read-only bundle.',
 'Native GStreamer builds embedded in OH HAPs, including PC. Plugin ABI/dependency closure and any external scanner execution must match the selected port.',
 'GStreamer|GST_PLUGIN_PATH|GST_PLUGIN_SYSTEM_PATH|GST_REGISTRY|dladdr|gstreamer-1.0|registry',
 ['native-dependency-closure','bundle-data-write-location'],
 ['Use `dladdr` on an address owned by the loaded application library to locate its directory, then resolve the staged `gstreamer-1.0` subdirectory.',
  'Before GStreamer initializes, set `GST_PLUGIN_PATH` and `GST_PLUGIN_SYSTEM_PATH` to that verified directory for the intended isolated plugin set.',
  'Create a writable sandbox directory and set `GST_REGISTRY` to a file there. Keep optional GST_DEBUG_FILE output writable too.',
  'Verify actual element discovery and recursive native dependencies; a plugin filename present in the HAP is only packaging evidence.'],
 'If a plugin scanner is required, its ability to execute is a separate packaging/sandbox contract. Use a versioned registry location or invalidate stale plugin caches across upgrades.',
 'Deriving paths from the host executable points at the Ability shell. Moving the registry does not make an incompatible plugin loadable.',
 'Start with an empty registry, enumerate needed elements, restart offline and upgrade the plugin cohort. Confirm no search of developer-machine paths or writes into the bundle.',
 [('strawberry-ohos','src/engine/gststartup.cpp'),('strawberry-ohos','README.md')])

add('native-gstreamer-explicit-audio-sink','Select a real OH GStreamer audio output backend','media',[],
 'Choose an available OH audio sink explicitly and package its full decoding/output chain; do not rely on a desktop autodetect sink absent from the port.',
 'GStreamer reports that autoaudiosink could not be created, or an OH player starts but cannot decode and output an MP3.',
 'OH GStreamer ports lacking the autodetect module and providing an OH-compatible OpenSL ES sink. The reviewed source reports PC playback; no curation playback test ran.',
 'autoaudiosink|openslessink|GStreamer|Q_OS_OHOS|gst-libav|MP3|OpenSL ES',
 ['native-gstreamer-plugin-registry','native-dependency-closure','audio-session-concurrency-policy'],
 ['Confirm the OH platform branch is compiled, then select `openslessink` when no user-selected output is configured and that sink is actually supplied.',
  'Inspect element creation and negotiation independently: typefind/demux, decoder, conversion and audio sink each need a real implementation.',
  'For the affected GStreamer port that disables built-in MP3 typefind, include its matching gst-libav/FFmpeg path. A sink-only fix cannot repair format recognition.',
  'Verify the sink was ported to the product\'s supported OpenSL ES interfaces; unsupported control calls must be handled according to whether they are essential.'],
 'The reviewed application contains the sink-selection branch. The README describes external gst-libav/OpenSL ES prerequisites whose implementation is not in that application snapshot; their full porting recipe is not independently validated here.',
 'A no-op sink or compiling output stub is not audible playback. Defining a platform macro does not supply absent plugin binaries.',
 'Test element availability, an ID3-tagged MP3, audible output, pause/resume, device routing and window background/foreground transitions.',
 [('strawberry-ohos','src/engine/gstengine.cpp'),('strawberry-ohos','CMakeLists.txt'),('strawberry-ohos','README.md')])

add('qt-ohos-terminal-direct-key-input','Route hardware keys directly to an OH Qt terminal widget','input',['qt'],
 'For a terminal that consumes hardware KeyPress events, disable input-method registration on the actual focused terminal widget and its relevant children, then verify the focus chain.',
 'Printable hardware keyboard characters do not reach an OH Qt terminal even though shortcuts or mouse input work.',
 'Qt 6 OH terminal widgets affected by IMF interception in the reviewed QPA/runtime generation. This scoped workaround can disable IME composition and is not a general text-editor fix.',
 'WA_InputMethodEnabled|IMF|KeyPress|QTermWidget|TerminalDisplay|hardware keyboard|IME',
 ['qt6-ohos-subwindow-input','qt-close-event-origin'],
 ['Log focus and KeyPress versus input-method events to establish whether IMF is consuming the hardware path.',
  'On the terminal subtree only, apply `setAttribute(Qt::WA_InputMethodEnabled, false)` to the container and actual focusable descendants.',
  'Find descendants by type/tree membership rather than assuming objectName equals className; revisit dynamically created children.',
  'After asynchronous native window activation, restore focus to the terminal using a lifecycle-aware signal where possible. Treat a fixed delay as a source-specific workaround.'],
 'If users require CJK or other composed input, repair/implement the input-method contract instead of applying this workaround indiscriminately. PTY availability and shell execution are separate from key routing.',
 'Disabling IME only on the parent misses the real TerminalDisplay child; enabling all text widgets for direct keys breaks composition elsewhere.',
 'Test printable ASCII, control keys, shortcuts, focus after reopen, and the intended composition policy on a PC keyboard. Check dynamically added terminal tabs.',
 [('qterminal-ohos','src/termwidget.cpp'),('qterminal-ohos','src/mainwindow.cpp'),('qterminal-ohos','README.md')])

add('kmp-ohos-coil-native-decoder','Register an OH native decoder in a KMP image loader','media',['kotlin-multiplatform'],
 'Supply an OH-specific Coil Decoder.Factory through the platform component registry and bind the native ImageSource/PixelMap APIs with the matching Kotlin/Native target.',
 'A KMP image loader has an OH target but no usable image decoder, or native image symbols fail to link.',
 'Coil/OvCompose forks with an actual ohosArm64 target and platform registry hook. PC raster size, memory use, format/alpha and display behavior require separate tests.',
 'Coil|OhosImageDecoder|Decoder.Factory|addOhosComponents|OH_ImageSourceNative|OH_PixelmapNative|cinterop',
 ['kmp-native-image-core-without-compose','kmp-ohos-dependency-and-renderer','hdr-output-surface-and-capability'],
 ['Select an OH-capable Kotlin/Compose/Coil dependency cohort; keep the decoder in the OH source set and register its Factory through the platform component hook.',
  'Define matching cinterop headers and native link libraries (`image_source` and `pixelmap` in the reviewed binding).',
  'Create and release ImageSource, decoding options, PixelMap and info objects using explicit ownership and error checks; Kotlin memScoped alone does not release native resource objects.',
  'Before transferring pixels to Skia, negotiate/query actual pixel format, alpha, dimensions and buffer size. Do not assume AUTO dynamic range yields RGBA8888 or that width×height×4 is always valid.'],
 'The reviewed decoder demonstrates the integration hook but assumes four-byte RGBA and unpremultiplied alpha while requesting AUTO dynamic range. Its complete pixel conversion is not promoted as a validated recipe. Network fetcher support is a separate component.',
 'A registered decoder does not prove safe HDR conversion, downsampling or correct network fetching; performance claims require measurement.',
 'Test malformed/empty input, large dimensions, transparent images, SDR/HDR sources, repeated cancellation and missing-library packaging. Compare pixel output to a known reference.',
 [('coil-ohos','coil-core/src/ohosArm64Main/kotlin/coil3/decode/OhosImageDecoder.kt'),('coil-ohos','coil-core/src/ohosArm64Main/kotlin/coil3/RealImageLoader.ohos.kt'),('coil-ohos','coil-core/src/ohosArm64Main/cinterop/image.def'),('coil-ohos','coil-core/build.gradle.kts'),('coil-ohos','buildSrc/src/main/kotlin/coil3/multiplatform.kt')],limitations=['Pixel format/size assumptions remain unaudited at runtime; do not copy the full source decoder as a proven remedy.'])

add('kmp-ohos-sqlite-cinterop','Link the SQLite implementation behind an OH KMP binding','storage',['kotlin-multiplatform'],
 'Treat the OH Kotlin target, cinterop declarations and real SQLite native implementation as one dependency chain; a klib or shared target declaration does not package sqlite3 automatically.',
 'A KMP SQLite API compiles but the OH host fails on sqlite3 symbols or cannot load the SQLite shared library.',
 'Kotlin/Native OH targets using SQLite C interop, whether bundled statically or through a separately packaged OH library. PC database paths and concurrent-window access need independent validation.',
 'sqlite3|cinterop|ohosArm64|linkerOpts|headers|klib|libsqlite3.so|Kotlin Native',
 ['database-backend-and-data-contract','native-dependency-closure','sqlite-read-write-transaction-contention','kmp-ohos-compose-target'],
 ['Build the actual OH target and connect its source set to the native driver; Android JNA and iOS system SQLite are different backends.',
  'Point cinterop at the correct sqlite3 headers and inspect def-file linker options. Choose either a built static archive or a real ABI-compatible SQLite shared library.',
  'For dynamic linkage, include the native SQLite library in the host dependency/package closure and ensure its requested loader name and exports match the binding.',
  'Keep database files and WAL/SHM sidecars in writable app storage. Preserve prepared-statement, string/blob ownership and connection/thread policy separately from packaging.'],
 'The two reviewed libraries demonstrate different integration shapes; they are not interchangeable binary dependencies. One embeds the bundled SQLite implementation in its cinterop declaration; the other documents dynamic host packaging. The latter\'s Gradle linker option names `-lsqlite3.h`, which needs correction/verification against the real library name before reuse. Source-reported multiplatform support does not establish PC execution.',
 'Shipping only a klib or assuming the OH image provides Android/iOS SQLite imports leaves the final native dependency unresolved.',
 'Inspect final native dependencies and run create/insert/query/close/reopen on the intended PC, including WAL sidecars, transaction failure and the selected concurrent-access policy.',
 [('kuikly-sqlite','kuiklySqlite/build.ohos.gradle.kts'),('kuikly-sqlite','kuiklySqlite/src/nativeMain/cinterop/sqlite3.def'),('kuikly-sqlite','kuiklySqlite/src/nativeMain/cinterop/CMakeLists.txt'),('vbsqlite-ohos','VBSQLite/build.gradle.kts'),('vbsqlite-ohos','VBSQLite/src/nativeInterop/cinterop/interop.def'),('vbsqlite-ohos','ohosApp/entry/src/main/cpp/CMakeLists.txt'),('vbsqlite-ohos','README.md')])

add('rnoh-gesture-scroll-frame-hold','Hold RNOH scroll frames while gesture arbitration waits','input',['react-native'],
 'Implement a temporary frame/delegate hold for a pending native gesture, retain nested-scroll ownership, and restore the current ScrollView policy when arbitration finishes.',
 'An RNOH pan/waitFor interaction scrolls the parent before the gesture is decided, or leaves a list stuck after cancellation.',
 'RNOH C++ gesture-handler ports using ScrollViewComponentInstance and ArkUI native scroll nodes. Relevant to nested PC touch/trackpad interactions; mouse-wheel and pointer-type parity are not established.',
 'ScrollFramesHold|waitFor|onScrollFrameBegin|NODE_SCROLL_NESTED_SCROLL|SELF_ONLY|gesture arbitration',
 ['rnoh-native-component-registration-parity','device-gesture-capability'],
 ['Install the hold only while the ScrollView owner exists, replacing its ScrollNode delegate with a forwarding delegate.',
  'While waiting, return zero frame offset and avoid reporting a drag to RN. Keep nested scroll SELF_ONLY so unused delta does not escape to a parent.',
  'Save forward/backward nested-scroll modes when forcing ownership; forward ordinary events to the owner.',
  'On release/destruction, restore the original delegate and reconcile modes with edge position and any newer prop-driven policy rather than blindly restoring a stale snapshot.',
  'Ensure cancellation/drop/owner destruction releases every hold on the owning UI thread.'],
 'This depends on RNOH internal ScrollView/delegate APIs and the selected gesture port. It is not a portable JS-only workaround or proof of every gesture recognizer\'s correctness.',
 'Rejecting a recognizer too early changes waitFor semantics; returning zero without holding nested-scroll policy can still move the parent.',
 'Test success/failure/cancel, nested edges, props changing during a hold, destroyed ScrollViews and repeated gestures. Confirm no stuck scroll or duplicate drag start/stop.',
 [('rnoh-gesture-handler',G+'ScrollFramesHold.cpp'),('rnoh-gesture-handler',G+'ScrollFramesHold.h'),('rnoh-gesture-handler',G+'native/NativeGestureManagerScroll.cpp'),('rnoh-gesture-handler','react-native-harmony-gesture-handler/docs/NATIVE-CODE-MAP.md'),('rnoh-gesture-handler','react-native-harmony-gesture-handler/package.json')])

add('electron-ohos-harmonyshare-image-receive','Route received OH images to the owning Electron window','integration',['electron'],
 'Map the Electron/internal window to its system window ID, register the product-specific image receive capability there, stage the received data in the sandbox and deliver a renderer-compatible file.',
 'A PC editor cannot receive a nearby shared image, sends it to the wrong window, or passes an authority file URI or HEIC image that the renderer cannot consume.',
 'Ported Electron hosts with a native HarmonyShare adapter on Huawei products exposing Collaboration.HarmonyShare. This is not generic OpenHarmony support and may require product/service/API authorization.',
 'HarmonyShare|dataReceive|general.image|windowId|HEIC|ImagePacker|file URI|Electron window',
 ['document-picker-uri-access','gallery-image-format-preservation','electron-ohos-platform-capability-gates'],
 ['Check `SystemCapability.Collaboration.HarmonyShare`, obtain the intended UIAbility context and map the live internal window to its system window ID.',
  'Register dataReceive for that system window and the supported UTD/count. Receive into an app-owned directory and wait for the actual received record.',
  'Open the URI using the file API that understands its authority. If the renderer lacks the format, decode and create a bounded, aspect-preserving compatible image without mislabelling its extension.',
  'Route pending/success/failure to the originating live renderer window with request identity. Unregister before losing the native window and release file/image/packer resources on every path.'],
 'The source produces JPEG despite an older comment saying PNG. Its fixed 1280 maximum edge is an application choice, not a platform limit. Consumer guidance adds bounded reads, cleanup and request/lifecycle guards that are not all implemented in the source.',
 'Stripping an authority URI into an arbitrary path bypasses its access contract. A pending placeholder needs a terminal failure outcome as well as success.',
 'Test two editor windows, unsupported service, HEIC and transparent input, oversized/corrupt data, failure during conversion and closing the target window while receiving.',
 [('marktext-ohos','web_engine/src/main/ets/adapter/HarmonyShareAdapter.ets'),('marktext-ohos','web_engine/src/main/ets/jsbindings/HarmonyShareAdapterBind.ets'),('marktext-ohos','README.md')],limitations=['Selected vendor adapter and binder only; renderer delivery, service privilege and physical-PC sharing were not executed. Source cleanup/error gaps are not claimed repaired.'])

extend('native-dependency-closure',[('letos-ohos','HarmonyOS/entry/src/main/cpp/CMakeLists.txt'),('letos-ohos','HarmonyOS/entry/src/main/cpp/patch_needed.py'),('strawberry-ohos','cmake/fix_ohos_needed_paths.py')],
 'Added independent Qt 6 OH application source evidence for absolute build-path DT_NEEDED caused by missing SONAME; source post-link rewriting was inspected, not executed.',
 'Inspect the exact `DT_NEEDED` strings as well as missing filenames. A dependency built without a SONAME can put an absolute development-machine path into the consumer. Prefer rebuilding/relinking with a stable intended loader name; if a scoped post-link rewrite is necessary, preserve ELF layout and re-inspect every dependency before packaging. Renaming the packaged file alone does not rewrite the consumer.')
extend('incoming-wants-before-runtime-ready',[('flutter-app-links','ohos/src/main/ets/components/plugin/AppLinksOhosPlugin.ets')],
 'Added AbilityAware launchWant/onNewWant implementation evidence. The source caches initial/latest URI only; it does not prove FIFO delivery of all pre-listener Wants, nor lifecycle-reset correctness.')
extend('tauri-ohos-dependency-cohort',[('cc-switch-ohos','src-tauri/Cargo.toml'),('cc-switch-ohos','src-tauri/Cargo.lock'),('cc-switch-ohos','src-tauri/gen/ohos/entry/oh-package-lock.json5')],
 'Added a full-application consumer with exact Tauri/tao/wry/Ability pins. Shared fork revisions are duplicate dependency lineage, not independent runtime corroboration.')
extend('tauri-ohos-ability-host',[('cc-switch-ohos','src-tauri/gen/ohos/entry/src/main/ets/entryability/EntryAbility.ets'),('cc-switch-ohos','src-tauri/gen/ohos/entry/oh-package.json5')],
 'Added an application host whose pinned implementation extends RustAbility and names its cdylib. Do not substitute newer NativeAbility terminology across dependency generations.')
extend('rendering-budget-and-progressive-ui',[('liberika','packages/erika_flutter/lib/src/erika_player.dart'),('liberika','packages/erika_flutter/ohos/src/main/ets/components/plugin/ErikaFlutterPlugin.ets')],
 'Reviewed new shared Dart JSON parsing for diagnostics/logs through a background isolate, plus OH method forwarding. This corroborates moving pure data decoding off the UI isolate, not invoking platform channels from an arbitrary isolate.')

dump(HERE/'curation-manifest.json',dict(added=ADDED,extended=EXTENDED,source_ids=sorted({k[0] for k in SNAPS}),snapshots=list(SNAPS.values())))
print(json.dumps(dict(added=ADDED,extended=EXTENDED,snapshots=len(SNAPS)),indent=2))

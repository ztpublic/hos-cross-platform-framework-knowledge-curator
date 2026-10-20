#!/usr/bin/env python3
"""One-time reviewed import recipe. Refuses to replace an existing knowledge DB."""
from pathlib import Path
import json, re, csv, hashlib, subprocess, shutil
ROOT=Path(__file__).resolve().parents[2]
F=Path('/Users/mac/projects/flutter-experiment/flutter-oh-knowledge')
Q=Path('/Users/mac/projects/qt-experiments/ohos_qt-skills')
COLLECTIONS={'flutter':F,'qt':Q}
REVISIONS={k:subprocess.check_output(['git','-C',str(p),'rev-parse','HEAD'],text=True).strip() for k,p in COLLECTIONS.items()}
ENTRIES={}; PROVS={}; MAP={}

def source(collection,path):
    origin=COLLECTIONS[collection]/path
    target=ROOT/'provenance/snapshots'/collection/path
    target.parent.mkdir(parents=True,exist_ok=True); shutil.copyfile(origin,target)
    return {'collection':collection,'original_path':path,'snapshot':str(target.relative_to(ROOT)), 'sha256':hashlib.sha256(target.read_bytes()).hexdigest(),'revision':REVISIONS[collection], 'license':'MIT; see maintenance/licenses/qt-MIT.txt' if collection=='qt' else 'No database-level license found; upstream project license context retained in original provenance.', 'evidence_note':'Exact local working-file snapshot, potentially different from HEAD. Source-reported validation was not rerun.'}

def add(ident,title,area,scope,summary,when,steps,conditions,mistakes,checks,sources,keywords=(),related=(),status='scoped'):
    meta={'schema_version':1,'id':ident,'title':title,'topic_key':area+'.'+ident,'area':area,'scope':'common' if scope=='common' else 'framework-specific','frameworks':[] if scope=='common' else [scope], 'summary':summary,'when_to_use':when,'keywords':list(keywords),'status':status,'evidence_tier':'imported-guidance','pc_validation':'not-run','applicability':conditions.split('\n')[0],'related':list(related)}
    body=f'# {title}\n\n## When to use\n\n{when}\n\n## Recommended approach\n\n{summary}\n\n## Implementation steps\n\n{steps}\n\n## Conditions and alternatives\n\n{conditions}\n\n## Common mistakes\n\n{mistakes}\n\n## Check it works\n\n{checks}\n'
    ENTRIES[ident]=(meta,body)
    PROVS[ident]={'schema_version':1,'id':ident,'sources':[source(*s) for s in sources],'transformations':['Rewritten as one situation-to-solution contract; source identity retained only in provenance.','Classification follows mechanism rather than source framework. Common applicability is a conditional synthesis, not verification in another framework.'],'limitations':['Imported guidance only; original builds, devices and upstream APIs were not revalidated.','HarmonyOS PC acceptance checks are proposed, not performed.']}
    for s in sources: MAP.setdefault(s,[]).append(ident)

COMMON=set('alternate-app-icon-contract capability-gated-backend-initialization chunked-native-file-transfer continuation-cached-state-and-readiness continuous-background-task-contract device-gesture-capability distributed-device-discovery-versus-sync external-handoff-file-uri ffi-library-and-symbol-availability file-export-contract foreground-recovery-and-save-lifetime incoming-wants-before-dart-ready live-view-session-and-fallback local-auth-policy-and-attempt-lifetime media-save-and-clipboard-permissions native-ai-image-fallback native-arkweb-game-host native-dependency-completeness native-event-loop-handle-teardown native-form-snapshot-bridge native-linker-toolchain-workarounds native-playback-state-observation native-qr-scan-session-contract native-web-login-cookie-completion network-adapter-policy-propagation oauth-browser-callback-correlation ocr-feature-evolution offline-data-rescue-startup-isolation persistent-file-access-grants picker-cancellation-concurrency resumable-file-download-contract rtc-join-and-participant-correlation sandbox-import-as-access-fallback sandbox-paths-and-app-identity secure-storage-backend-selection sqlite-read-write-transaction-contention system-lyric-session-and-privacy system-reminder-identity-and-lifetime terminal-guest-working-directory vpn-native-lifecycle-and-routing'.split())
RENAME={'incoming-wants-before-dart-ready':'incoming-wants-before-runtime-ready'}
# Split independently actionable subsections; retain narrower adapter details when required.
SPLITS={
'build-toolchain-and-evidence-tiers': [('Diagnose mixed compiler/engine caches before changing application semantics','flutter-engine-cache-coherence','flutter','Flutter startup fails despite matching version strings.','Align compiler, platform kernels, engine and embedding artifacts from the same pinned distribution.')],
'database-backend-and-data-contract': [('Share a database with a native extension deliberately','shared-database-extension-contract','common','A native extension opens an empty or incompatible store instead of application data.','Verify physical store identity and configuration, use one migration owner and scope extension access.'),('Migrate string-encoded preferences to native JSON values by schema','typed-preference-schema-migration','common','Preferences change from encoded strings to typed values or synchronize between schema versions.','Migrate only schema-declared keys and publish a complete validated snapshot with explicit version reconciliation.')],
'foreground-recovery-and-save-lifetime':[('Keep reply watchdogs and native protection separate','transport-reply-watchdog','common','A resume or network change starts redundant health probes and reconnects.','Share one pending transport probe, classify its result and coalesce reconnect work separately from native background eligibility.')],
'external-handoff-file-uri':[('Calendar import through a receiver has a user-owned completion step','calendar-file-handoff-completion','common','Opening a calendar file is incorrectly reported as completed event insertion.','Keep the file and read grant alive; report receiver opening separately from the user completing calendar import.'),('Deep actions require a receiver contract as well as a valid URI','external-deep-action-contract','common','A valid deep-action URI opens no receiver or opens the wrong screen.','Validate receiver routing and encoded parameters; distinguish accepted launch from completed navigation.')],
'incoming-wants-before-dart-ready':[('Separate schedule consumption from completed delivery','scheduled-event-delivery-stages','common','A scheduled action is consumed but runtime startup or its handler fails.','Track consumption, submission and acknowledgement separately, retaining bounded failure records without implying automatic retry.')],
'media-save-and-clipboard-permissions':[('Preserve encoded image format across a file-based gallery API','gallery-image-format-preservation','common','An image saved through a temporary file is misclassified or loses its original format.','Match validated encoded bytes to suffix and MIME type, await gallery consumption and clean up the owned staging file.')],
'native-arkweb-game-host':[('Keep recovery controls outside synthetic game input capture','arkweb-synthetic-input-exclusions','common','A synthetic game-touch adapter prevents recovery buttons from receiving pointer events.','Exclude native HTML controls and recovery overlays from game-input capture and bind bridge work to the active session.')],
'native-form-snapshot-bridge':[('Read current domain data when a saved display snapshot is insufficient','native-form-current-data-provider','common','A home-screen card shows stale daily data while the application runtime is absent.','Use a verified native read-only data provider and derive fresh display state without starting the main runtime.')],
'native-shell-state-synchronization':[('Synchronize native material settings as effective state','flutter-native-material-state','flutter','A Flutter setting claims a native HDS material level is applied before the shell accepts it.','Validate one typed setting map and replay desired material state after native readiness, preserving failed requests for retry.')],
'picker-result-readable-content':[('Read large URI selections through bounded native handles','native-uri-streaming-read','common','Large selected files allocate their entire content before the importer can stream them.','Return selection metadata first and read through owned bounded native descriptors with serialized read/close operations.')],
'rendering-budget-and-progressive-ui':[('Reduce startup work without delaying correctness prerequisites','startup-work-prerequisites','common','First display is delayed by housekeeping or scans, or deferral starts consumers before storage and grants are ready.','Complete correctness prerequisites first, then schedule owned optional work after a usable first frame.')],
'resumable-file-download-contract':[('Scale uploads and previews without collecting the entire file','bounded-upload-staging','common','Attachment uploads or previews exhaust memory or keep temporary files alive after cancellation.','Stream an owned staging file with backpressure and enforce separate transfer and preview limits.'),('Give resumable uploads a stable target and an independent owner','resumable-upload-identity','common','An upload retry creates another remote object or accepts an unrelated staged prefix.','Retain a transfer-owned remote target and immutable local content, validate identity on resume and separate navigation from transfer lifetime.')],
'rust-bridge-target-and-packaging':[('Preserve the real Cargo and rustup environment','rust-build-host-discovery','common','A build hook derives Cargo home from a distribution shim and loses the intended toolchain.','Honor explicit Cargo/rustup configuration and derive a home only from a verified rustup-managed layout.')],
'secure-storage-backend-selection':[('Identify file-backed secret storage by its actual protection','file-secret-store-guarantees','common','A file-backed credential provider is mistaken for encrypted or hardware-protected storage.','Describe filesystem access control and replacement guarantees accurately and select the provider only when they meet policy.')],
'video-surface-backend-contract':[('Await native teardown before replacing a backend','flutter-player-async-teardown','flutter','A player replacement races asynchronous native disposal or late stream events.','Use one idempotent teardown Future, cancel subscriptions and await native release before replacement.'),('Serialize surface reconstruction with source loading','flutter-player-surface-reconstruction','flutter','Changing renderer options races loading or leaves a new controller bound to old views.','Serialize source and output changes, retain retry state and rebind all controller-owned subscriptions.'),('Branch custom renderers by the active output type','flutter-native-video-output-selection','flutter','A texture-only custom widget shows no picture for a native platform-view backend.','Mount the widget matching the active native output and reconstruct through the same queue when the host changes.')],
'viewport-scale-and-layout-lifetime':[('Separate WebView page zoom from application paint zoom','flutter-arkweb-page-zoom','flutter','Application scaling and ArkWeb zoom multiply twice or reset incorrectly.','Keep page zoom independent from Flutter paint transforms and preserve the original page style when using a CSS fallback.'),('Recompute native layout when density changes','arkui-density-and-inset-layout','common','A PC window keeps its physical size but logical layout becomes wrong after a density or display change.','Use the owning UIContext to convert current physical bounds, listen for density changes and merge avoid areas once per edge.')],
}

def generalize(t):
    # Prose-only substitutions, applied only to entries whose mechanism is shared.
    changes={'a Flutter app':'an application','A Flutter app':'An application','Flutter application':'cross-platform application','Flutter UI':'application UI','Dart/native':'shared/native','Dart/Rust':'application/Rust','Dart-to-Rust':'application-to-Rust','Dart request':'application request','Dart caller':'application caller','Dart join operation':'application join operation','Dart subscriptions':'application subscriptions','Dart handlers':'application handlers','Dart model':'shared model','Dart callback':'application callback','Dart session':'application session','Dart stream':'application event stream','Dart Future':'application future','Dart memory':'application memory','Dart timer':'application timer','Dart boundary timers':'application boundary timers','Dart facade':'application facade','Dart receiver':'application receiver','Dart announces readiness':'the application runtime announces readiness','Dart explicitly becomes ready':'the application runtime explicitly becomes ready','Dart becomes ready':'the application runtime becomes ready','Dart reads it':'the application runtime reads it','Dart actually supplies it':'the application actually supplies it','Dart processing':'application processing','Dart is':'the application runtime is','before Dart subscribes':'before the application subscribes','until Dart':'until the application runtime','from Flutter':'from the application runtime','when Flutter is absent':'when the application runtime is absent','without Flutter':'without the application runtime','while Flutter is frozen':'while the application runtime is frozen','Flutter is absent':'the application runtime is absent','when Flutter':'when the application runtime','with Flutter data':'with shared application data','Keep the Flutter UI':'Keep the application UI','Hiding a Flutter widget':'Hiding an application widget','a small Dart model':'a small shared model','shared Dart':'shared application code','A Dart SSH/SFTP':'An SSH/SFTP','bounded Dart file':'bounded file','normal Flutter startup':'normal application startup','Flutter and native application data':'shared and native application data','as Flutter.':'as the shared application layer.'}
    for a,b in changes.items():t=t.replace(a,b)
    t=t.replace('An OH PC may use desktop-style Dart UI while still requiring an OH HAR/MethodChannel backend; a Windows/Linux FFI route is usable only when its libraries also exist for the OH target.','An OH PC can use a desktop layout while still requiring an OH native adapter; a Windows/Linux FFI route is usable only when its libraries also exist for the OH target.')
    t=t.replace('Bind native channels only after Flutter binding setup.','Bind native bridges only after the owning runtime initializes.')
    t=t.replace('On OH, a dedicated `FlutterAbility` can register only the required file-selection plugin and a maintenance channel.','On OH, a dedicated recovery ability can register only the required selection and maintenance adapters.')
    t=t.replace('An application runtime file-selection Future','A file-selection future')
    t=t.replace('A Flutter file-selection Future','A file-selection future').replace('Match the public Dart API:','Match the public adapter API:').replace('A Dart timeout','An application timeout')
    t=t.replace('A custom `XFile` adapter can implement `openRead(start, end)`','A streaming adapter can expose a ranged read operation').replace('Close the token in Dart `finally`','Close the token in the caller\'s `finally`')
    t=t.replace('Have Flutter compute','Have the application runtime compute').replace('opens a Flutter action','opens an application action').replace('attempt to render the Flutter widget tree','attempt to render the shared framework widget tree')
    return t


def import_flutter():
    for f in sorted((F/'knowledge').glob('*.md')):
        raw=f.read_text(); old=json.loads(raw.split('---',2)[1]); body=raw.split('---',2)[2].strip()+'\n'
        name=f.stem; ident=RENAME.get(name,name); scope='common' if name in COMMON else 'flutter'
        original_conditions=re.search(r'## Conditions and alternatives\n\n(.*?)(?=\n#|\Z)',body,re.S)[1].strip()
        extra=[]
        for heading,sid,sscope,when,approach in SPLITS.get(name,[]):
            pattern=r'### '+re.escape(heading)+r'\n\n(.*?)(?=\n#{2,3}\s|\Z)'
            match=re.search(pattern,body,re.S)
            if not match: raise ValueError((name,heading))
            text=match[1].strip()
            if sscope=='common':text=generalize(text)
            conditions=generalize(original_conditions) if sscope=='common' else original_conditions
            # Parent's unrelated setup is replaced by the extracted atom's actual prerequisites.
            if sid=='rust-build-host-discovery':conditions='Applies to build hosts invoking Cargo/rustup; independent of which UI framework consumes the OH target library.'
            if sid=='native-uri-streaming-read':conditions='Requires an OH native file adapter that supports authorized descriptors and bounded reads. This protocol is not a standard method set guaranteed by every picker plugin.'
            if sid=='arkui-density-and-inset-layout':conditions='Applies to ArkUI shells using vp and a live owning UIContext; verify density and avoid-area events in the selected SDK and PC window modes.'
            add(sid,heading,old['area'],sscope,approach,when,text,conditions,'Do not infer completion or portability from construction, a queued request, or a method name alone. Preserve the lifetime and failure distinctions above.','Exercise the failure described above, repeat after owner recreation, and verify the actual native result. Include the concrete edge cases listed in the implementation.', [('flutter','knowledge/'+f.name),('flutter','provenance/'+name+'.json')],old['keywords'],[ident])
            body=body[:match.start()]+f'See [{heading}]({sid}.md) for this independent contract.\n'+body[match.end():]
            extra.append(sid)
        if scope=='common':body=generalize(body)
        m={k:old[k] for k in ['title','area','summary','when_to_use','keywords']}
        if scope=='common':
            for k in ['title','summary','when_to_use']:m[k]=generalize(m[k])
        meta={'schema_version':1,'id':ident,'topic_key':old['topic_key'].replace(name,ident),'scope':'common' if scope=='common' else 'framework-specific','frameworks':[] if scope=='common' else ['flutter'],**m,'status':'scoped','evidence_tier':'imported-guidance','pc_validation':'not-run','applicability':('Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.' if scope=='common' else 'Flutter OH with the selected embedding/plugin versions; PC behavior requires the stated acceptance checks.'),'related':extra}
        # Explicit examples remain examples; wrapper semantics are not claims about other frameworks.
        if scope=='common' and re.search(r'\bFlutter\b|\bDart\b|MethodChannel|EventChannel|BinaryCodec',body):
            body=body.replace('## Conditions and alternatives\n','## Conditions and alternatives\n\nNamed Flutter/Dart APIs below are adapter examples. The shared contract applies only through an implementation with equivalent native semantics; it does not establish other frameworks\' support.\n',1)
        ENTRIES[ident]=(meta,body)
        PROVS[ident]={'schema_version':1,'id':ident,'sources':[source('flutter','knowledge/'+f.name),source('flutter','provenance/'+name+'.json')],'upstream_provenance':json.loads((F/'provenance'/(name+'.json')).read_text()),'transformations':['Retained actionable source guidance, normalized metadata and PC evidence boundary.','Separated independently actionable subsections: '+', '.join(extra) if extra else 'Single-contract entry retained.','Generalized runtime prose where the native contract does not depend on Flutter.' if scope=='common' else 'Retained Flutter-specific API/embedding conditions.'],'limitations':['Exact local knowledge import, including uncommitted source bytes; upstream source and runtime tests were not rerun.','PC validation not run; common classification does not claim implementation in all frameworks.']}
        for path in ['knowledge/'+f.name,'provenance/'+name+'.json']:MAP.setdefault(('flutter',path),[]).append(ident)


def qt(ident,title,area,scope,when,approach,steps,conditions,mistakes,checks,paths,keywords=(),related=(),status='scoped'):
    add(ident,title,area,scope,approach,when,'\n'.join(f'{i}. {s}' for i,s in enumerate(steps,1)),conditions,mistakes,checks,[('qt',p) for p in paths],keywords,related,status)


def finish():
    for ident,(meta,body) in ENTRIES.items():
        for old,new in RENAME.items():body=body.replace('('+old+'.md)','('+new+'.md)')
        refs=re.findall(r'\]\(([a-z0-9-]+)\.md\)',body)
        meta['related']=sorted(set(meta['related']+refs)-{ident})
        (ROOT/'knowledge'/f'{ident}.md').write_text('---\n'+json.dumps(meta,ensure_ascii=False,indent=2)+'\n---\n\n'+body)
        (ROOT/'provenance'/f'{ident}.json').write_text(json.dumps(PROVS[ident],ensure_ascii=False,indent=2)+'\n')
    with (ROOT/'maintenance/import-map.csv').open('w',newline='') as out:
        w=csv.writer(out,lineterminator='\n');w.writerow(['collection','source_path','entry_ids','disposition','reason'])
        for (c,p),ids in sorted(MAP.items()):w.writerow([c,p,'|'.join(sorted(set(ids))),'split' if len(set(ids))>1 else 'imported-or-merged','Mechanism reviewed and consumer guidance normalized; original bytes preserved.'])
    shutil.copyfile(Q/'LICENSE',ROOT/'maintenance/licenses/qt-MIT.txt')
    source('flutter','projects.json')
    sources=[]
    for c,p in COLLECTIONS.items():
        sources.append({'id':c+'-knowledge-import','kind':'knowledge-collection','url':subprocess.check_output(['git','-C',str(p),'remote','get-url','origin'],text=True).strip(),'local_path':str(p),'observed_commit':REVISIONS[c],'analyzed_commit':None,'import_scope':'Documents enumerated by maintenance/import-map.csv; not a fresh source-project review.','working_tree_status':subprocess.check_output(['git','-C',str(p),'status','--short'],text=True),'frameworks':['flutter'] if c=='flutter' else ['qt'],'snapshot_identity':'Per-file SHA-256; HEAD alone does not describe imported working content.'})
    (ROOT/'maintenance/sources.json').write_text(json.dumps({'schema_version':1,'sources':sources},indent=2)+'\n')
    (ROOT/'maintenance/runs/2026-10-20-initial-import.json').write_text(json.dumps({'mode':'import','outcome':'imported-pending-validation','source_flutter_entries':len(list((F/'knowledge').glob('*.md'))),'entries':len(ENTRIES),'verification_performed':['local document extraction and source hash capture'],'not_performed':['fresh upstream API/source verification','build','sign','install','device','runtime'],'limitations':['Electron, React Native and Kotlin Multiplatform adapter knowledge has not been researched in this import.','Original project registries retained as source snapshots, not adopted as independently analyzed projects.']},indent=2)+'\n')

if __name__=='__main__':
    if list((ROOT/'knowledge').glob('*.md')): raise SystemExit('Refusing to overwrite an existing knowledge database')
    import_flutter()
    exec((Path(__file__).with_name('qt_atoms.py')).read_text())
    exec((Path(__file__).with_name('polish.py')).read_text())
    finish()

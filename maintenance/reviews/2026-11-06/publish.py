"""Record dispositions and advance checkpoints after index/check passed; executes no source code."""
import json,subprocess,re,shutil,datetime,hashlib
from pathlib import Path
HERE=Path(__file__).resolve().parent;DB=HERE.parents[2];DATE='2026-11-06';RUN='2026-11-06-update-and-discovery.json'
def dump(p,d):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n')
man=json.loads((HERE/'curation-manifest.json').read_text());raw=json.loads((HERE/'ref-audit-raw.json').read_text())
reg=json.loads((DB/'maintenance/sources.json').read_text());S={s['id']:s for s in reg['sources']}
OK={'kuiklyui':{'45e6129','fb374b7','5c7cc1d','5dfd9b3','dd7d1de','cbe47b3','a4060b5','59c8e21','677c36f','f0d0389'},'liberika':{'aad19aa','3187f87','7334e0f','aa6e81a','33cb7c9','e83f1c8'},'rnoh-gesture-handler':{'9111b6b','fb3b58f'},'dsh-desktop-hos':{'6ca5653','1f706a8','cfac473'},'drawio-in-harmony':{'ca26149'},'tauri-gproxy':{'997daf2','dbb132b'}}
led=[];moved=[r for r in raw if r[2] and r[2]!=r[3]]
ranges=[]
for r in moved:
 sid=r[0];s=S[sid];p=s['local_path'];old=s['analyzed_commit'];new=r[2]
 log=subprocess.check_output(['git','-C',p,'log','--format=%h%x09%P%x09%s',f'{old}..{new}'],text=True,errors='replace').splitlines()
 for l in log:
  h,par,sub=l.split('\t',2);h7=h[:7]
  if sid=='ohos-jupyter':d,why='duplicate','History rewrite: tip tree 8168f585 is identical to the previously analyzed tree.'
  elif h7 in OK.get(sid,()):d,why='reviewed','Diff or message and affected files read; see source-entry map for any contribution.'
  elif len(par.split())>1:d,why='duplicate','Merge integration; its parents are dispositioned separately.'
  elif re.match(r'(docs|chore|ci|test|build\(deps|release|Update native artifact)|RELEASE|store-listing|Add privacy',sub):d,why='irrelevant','Subject and path triage: documentation, release, CI or store material.'
  elif re.search(r'\b(android|ios|macos|windows|linux|h5|web)\b',sub,re.I) and not re.search('ohos',sub,re.I):d,why='irrelevant','Subject and path triage: another platform.'
  elif sid=='tauri-gproxy':d,why='irrelevant','Subject and path triage: gateway/business change; only .gitlab/Dockerfile.ohos touches an OH path in this range (image digest bumps).'
  else:d,why='irrelevant','Subject and path triage: shared code with no distinct OH mechanism identified; not a line-by-line audit.'
  led.append(dict(source=sid,commit=h,subject=sub[:140],disposition=d,reason=why))
 ranges.append(dict(id=sid,previous=old,observed=new,commits=len(log)))
dump(HERE/'commit-dispositions.json',led)
CD=[
('https://atomgit.com/LiquidStudio/zcode','source-reviewed-contributed','USB compatibility layer contributed one atom; PTY, sqlite, HNP and resfile work already covered.'),
('https://atomgit.com/OpenHarmonyPCDeveloper/ohos_carnac_electron','screened-no-new-atom','main.js has no OH branch; dropping the global hook is a README claim.'),
('https://atomgit.com/he-nan/electron-ohos-bridge','source-reviewed-contributed','Real N-API addon, AKI adapter and OH_Input calls. Hash-only evidence because of all-rights-reserved headers.'),
('https://atomgit.com/OpenHarmonyPCDeveloper/ohos_insomnia_electron','screened-no-new-atom','Pure-JS native addon replacement is covered by the addon ABI entry.'),
('https://atomgit.com/OpenHarmonyPCDeveloper/ohos-electron-builder','screened-no-new-atom','Covered by the builder packaging entry; command-line signing noted as a lead for hap-signing-identity.'),
('https://atomgit.com/OpenHarmonyPCDeveloper/ohos_Anki','unqualified','Description says PyQt6+QtWebEngine, but the build is a Qt5 QML stub over a JSON store with no CPython, PyQt or web layer.'),
('https://atomgit.com/ohos-qt/qt-creator','source-reviewed-contributed','Real Q_OS_HARMONY branches; askpass helper and focus workaround. Sampled, not a full audit.'),
('https://atomgit.com/OpenHarmonyPCDeveloper/ohos_krita','source-reviewed-contributed','Dependency build and main.cc fixes; default CMake build is a shell. Rawfile tree extraction and software-canvas seed rejected as atoms: fixed marker, swallowed failure and override of user choice make them poor guidance.'),
('https://atomgit.com/OpenHarmonyPCDeveloper/ohos_Shotcut','screened-no-new-atom','Shell only with covered guards; full port is blocked according to its docs.'),
('https://atomgit.com/bigmolihuan/hlbmerge_flutter','unqualified','Phone-only deviceTypes and stock host; PC not targeted.'),
('https://atomgit.com/oh-flutter/tracking_query','screened-no-new-atom','phone+2in1 with stock host and no OH branches.'),
('https://atomgit.com/XJ-Cyber/flutter_ohos_list_performance_demo','unqualified','Phone-only generic list technique.'),
('https://atomgit.com/CPF-RN/rn_ohfeatures','screened-no-new-atom','Real TurboModules for phone features; DocumentPicker is covered.'),
('https://atomgit.com/CPF-RN/rntpc_clipboard','source-reviewed-contributed','Pasteboard queue and image record structure.'),
('https://github.com/ftsukic/rnoh-keyboard-insets','source-reviewed-contributed','Fabric component with keyboard animation handling.'),
('https://atomgit.com/wdracky/kmp-ohos-adapters','unqualified','ArkTS rewrites and articles only; no Kotlin target or interop.'),
('https://atomgit.com/oh-tpc/compose-webview-multiplatform','source-reviewed-contributed','Real ohosArm64/ohosX64 targets; example host contributed packaging and bridge atoms.'),
('https://atomgit.com/jianguoxu/Tauri-harmony','source-reviewed-contributed','Resolved OH Tauri fork, cdylib and Ability host; TLS backend selection contributed.'),
('https://atomgit.com/nutpi/InkNote','different-framework','Flutter with flutter_rust_bridge, not Tauri; not reviewed further for adapter atoms.'),
('https://atomgit.com/gcw_Y9SuENJM/harmonyos-pc-handwriting-pad','source-reviewed-contributed','Driver extension, USB DDK and HID DDK sample.'),
('https://github.com/xristosstarnetgames439-del/HuiwenMCP','not-fetched-suspicious-metadata','Disposable-looking owner, high stars and same-day push with a broad description. Not cloned or opened; treat with caution.'),
('https://github.com/dmalkerq/rnohp','not-fetched-suspicious-metadata','Random-name repository family with placeholder description (also azyfg-aley/rnoh, Excellenceiuchannel/rnohfkxf, james2124661/rnohnm, scottana4/rnohae). Likely spam.'),
('https://atomgit.com/PERMISSION-DENIED/electron-harmonyos-kb','deferred-lead','Knowledge collection, not screened in this pass; also openharmony-skills/electron-ohos-adapt, OpenHarmonyPCDeveloper/ohos_Electron-skills and CPF-RN/usage-docs.'),
('https://atomgit.com/OpenHarmonyPCDeveloper/ohos_github-desktop_electron','deferred-lead','Beyond the 20-candidate cap; also ohos_theia, ohos_rstudio, ohos_Scribus, ohos_git-cola, ohos-qt/openshot, ohos-pyqt/vidcutter, qq8864/tauri-ohos, CPF-KMP-CMP/ohos-kmp-toolchain, OHOS-CMP/compose-multiplatform-core.'),
]
dump(HERE/'candidate-dispositions.json',[dict(url=u,disposition=d,reason=w,commit=None) for u,d,w in CD])
shutil.copy('/tmp/kb2611/ag.json',HERE/'atomgit-search-results.json');shutil.copy('/tmp/kb2611/gh-search.json',HERE/'github-search-results.json');shutil.copy('/tmp/kb2611/cand.json',HERE/'candidate-fetch.json')
# licenses
CAND={c['id']:c for c in json.load(open('/tmp/kb2611/cand.json'))}
LIC={'qt-creator-ohos':['LICENSE.GPL3-EXCEPT','LICENSE.GPL3','COPYING','LICENSE'],'krita-ohos':['COPYING'],'rntpc-clipboard':['LICENSE'],'rnoh-keyboard-insets':['LICENSE'],'compose-webview-ohos':['LICENSE','LICENSE.txt'],'pc-handwriting-pad':['LICENSE']}
lic=[]
for i,names in LIC.items():
 c=CAND[i];inv=open(f'/tmp/kb2611/inv/{i}.txt').read().splitlines()
 for n in names:
  if n in inv:
   b=subprocess.check_output(['git','-C',c['local_path'],'show',c['observed_commit']+':'+n]);(DB/f'maintenance/licenses/{i}-{n}').write_bytes(b);lic.append(f'{i}-{n}');break
 else:lic.append('MISSING '+i)
print(lic)
NEW={'qt-creator-ohos':('qt','GPL-3.0 with Qt exception; file headers also list Qt-Commercial.',['ohos-exec-helper-permission-section','qt-ohos-focusin-after-modal-exec']),'krita-ohos':('qt','GPL-3.0',['ohos-musl-symbol-interposition-localize','ohos-fontconfig-sandbox-bootstrap']),'rntpc-clipboard':('react-native','MIT',['pasteboard-serialize-setdata','pasteboard-image-pixelmap-plus-encoded-record']),'rnoh-keyboard-insets':('react-native','MIT',['rnoh-keyboard-avoidance-animated-insets']),'compose-webview-ohos':('kotlin-multiplatform','Apache-2.0',['kmp-ohos-libcxx-shared-staging','arkweb-javascriptproxy-deferred-reply']),'tauri-harmony-jianguoxu':('tauri','No repository-level license found.',['rust-ohos-tls-without-openssl']),'zcode-ohos':('electron','No repository-level license found.',['ohos-usbfs-preload-compat']),'electron-ohos-bridge':('electron','No repository license at root; reviewed adapter files state Huawei, all rights reserved; vendored aki is Apache-2.0. Hash-only evidence retained.',['electron-ohos-arkts-call-bridge','ohos-input-injection-authorization']),'pc-handwriting-pad':('common','Apache-2.0',['ohos-usb-driver-extension-binding','ohos-virtual-hid-device-injection'])}
for sid,(fw,l,k) in NEW.items():
 c=CAND[sid]
 reg['sources'].append(dict(id=sid,kind='project',url=c['url'],framework=fw,aliases=[c['url'].replace('atomgit.com','gitcode.com')] if 'atomgit' in c['url'] else [],selected_ref=c['selected_ref'],observed_commit=c['observed_commit'],analyzed_commit=c['observed_commit'],license=l,local_path=c['local_path'],working_tree_status='bare research repository; no checkout',knowledge_ids=k,checked_at=DATE,last_run=RUN,coverage='Selected-mechanism source review at the pinned shallow snapshot; no whole-repository/history or feature-completeness claim.',source_reported_validation='Any build/device/PC claims in retained source documents are author reports only; no application build or runtime test was executed.'))
for r in raw:
 s=S[r[0]];s['checked_at']=DATE;s['last_run']=RUN
 if r[2] and r[2]!=r[3]:
  s['observed_commit']=r[2];s['analyzed_commit']=r[2]
  s['update_ref']='refs/kb-update/'+s['selected_ref']
  if r[0]=='ohos-jupyter':s['history_note']='Remote history was rewritten; the new tip is not a descendant of 18b16874 but has the identical tree. Local checkout and its staged state were left untouched.'
for i,kk in {'dsh-desktop-hos':['electron-ohos-platform-binding-loader-shim','native-self-extract-gnu-long-names','native-hmdfs-no-hardlink-publish','native-ohos-flock-rejected'],'drawio-in-harmony':['electron-ohos-hardware-shortcut-modifier-replay','electron-ohos-asar-store-precheck'],'kuiklyui':['napi-handle-scope-without-js-frame','native-arkui-callback-no-throw-text','native-custom-text-app-font-scale-cap','kmp-kuikly-ohos-sync-gesture-context-scheduling','ffi-library-and-symbol-availability'],'liberika':['native-avcodec-native-buffer-format','native-bridge-weak-optional-runtime-symbol','native-wgpu-gles-adapter-limits'],'rnoh-gesture-handler':['rnoh-gesture-scroll-frame-hold']}.items():
 S[i]['knowledge_ids']=sorted(set(S[i].get('knowledge_ids',[]))|set(kk))
dump(DB/'maintenance/sources.json',reg)
import collections
cnt=collections.Counter(x['disposition'] for x in led)
run=dict(schema_version=1,id=RUN[:-5],date=DATE,mode='update and discovery',outcome='complete for recorded incremental ranges and selected mechanisms; discovery capped at 20 screened candidates with deferred leads, not ecosystem exhaustion',source_ids=[r[0] for r in raw]+list(NEW),source_reviews=ranges,added_entries=man['added'],extended_entries=man['extended'],counts=dict(entries_before=236,entries_after=236+len(man['added']),refs_checked=len(raw)-2,refs_advanced=len(moved),commits=len(led),commit_dispositions=dict(cnt),new_sources=len(NEW),candidates_screened=20,atomgit_results=len(json.load(open(HERE/'atomgit-search-results.json')))),review_records=['reviews/2026-11-06/commit-dispositions.json','reviews/2026-11-06/candidate-dispositions.json','reviews/2026-11-06/candidate-fetch.json','reviews/2026-11-06/atomgit-search-results.json','reviews/2026-11-06/github-search-results.json'],verification_performed=['kb.py index and check','Realistic symptom and framework searches, recorded in the update report','Key claims from delegated screening re-read against pinned source before authoring','git diff --check'],limitations=['No source setup, build, signing, installation, emulator or PC run was performed.','Gitee search API returned no results anonymously (empty or HTTP 400); the GitCode web search was blocked. AtomGit anonymous search returned 10 per page and hit HTTP 429 late in the second pass.','GitHub search was restricted to repositories updated since 2026-08-01 and reached a rate limit on its last query.','Large repositories were sampled by path selection, not audited.','Commit triage beyond reviewed commits used subject and path only.','The flutter collection remote relation is unchanged from the prior record; local bytes were compared by hash, remote objects were not fetched.'])
dump(DB/'maintenance/runs'/RUN,run)
print(cnt,len(led))

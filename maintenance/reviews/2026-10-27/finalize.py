"""Publish source checkpoints only after successful database validation."""
import collections
import hashlib
import json
from pathlib import Path
import subprocess
from read_source import ROWS

HERE=Path(__file__).resolve().parent
DB=HERE.parents[2]
HELPER='/Users/mac/.codex/skills/harmonyos-pc-knowledge-curator/scripts/kb.py'
DATE='2026-10-27'
RUN='2026-10-27-update-and-discovery.json'

def dump(path,data):
 path.parent.mkdir(parents=True,exist_ok=True);path.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n')

def git(sid,*args):
 return subprocess.check_output(['git','-C',ROWS[sid]['local_path'],*args],stderr=subprocess.PIPE)

def meta(id):
 return json.loads((DB/'knowledge'/f'{id}.md').read_text().split('---',2)[1])

manifest=json.loads((HERE/'curation-manifest.json').read_text())
assert len(manifest['added'])==len(set(manifest['added']))
subprocess.run(['python3',HELPER,'index'],check=True)
subprocess.run(['python3',HELPER,'check'],check=True)
audit=json.loads((HERE/'ref-audit.json').read_text())
ledger=json.loads((HERE/'commit-dispositions.json').read_text())
assert len(ledger)==sum(len(r.get('commits',[])) for r in audit)
assert not any(r['disposition']=='deferred' for r in ledger)
for r in audit:
 if r.get('worktree_after') is not None:
  assert r['worktree_before']==r['worktree_after']
  assert r['head_before']==r['head_after']

registry=json.loads((DB/'maintenance/sources.json').read_text())
existing={s['id']:s for s in registry['sources']}
snapshots=manifest['snapshots']
source_map=collections.defaultdict(lambda:collections.defaultdict(list))
for id in manifest['added']+manifest['extended']:
 p=json.loads((DB/'provenance'/f'{id}.json').read_text())
 for s in p['sources']:
  if s['snapshot'] in {x['snapshot'] for x in snapshots}:
   source_map[s['collection']][id].append(s['original_path'])
licenses={x['collection']:x['license'] for x in snapshots}
new_ids=sorted(set(source_map)-set(existing))
assert len(new_ids)==19
for sid in new_ids:
 r=ROWS[sid]
 framework='react-native' if sid=='tsl-editor' else r['framework']
 paths=sorted({p for files in source_map[sid].values() for p in files})
 s=dict(id=sid,kind='project',url=r['url'],framework=framework,aliases=[],selected_ref=r['selected_ref'],observed_commit=r['observed_commit'],analyzed_commit=r['observed_commit'],license=licenses[sid],local_path=r['local_path'],working_tree_status='bare research repository; no checkout',reviewed_paths=paths,knowledge_ids=sorted(source_map[sid]),checked_at=DATE,last_run=RUN,coverage='Complete selected-mechanism source review at the pinned snapshot; no whole-repository/history or feature-completeness claim.',source_reported_validation='Any build/device/PC claims in retained source documents are author reports only; no application build or runtime test was executed.')
 registry['sources'].append(s);existing[sid]=s
 # Pin root notices and README exactly, without touching binary/signing material.
 inventory=git(sid,'ls-tree','--name-only',r['observed_commit']).decode().splitlines()
 root_files=[p for p in inventory if p.lower().startswith(('license','copying')) or p=='README.md']
 if sid=='flutter-record-ohos':root_files.append('record_ohos/LICENSE')
 for path in root_files:
  data=git(sid,'show',r['observed_commit']+':'+path)
  target=DB/'provenance/snapshots/projects'/sid/DATE/path
  target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
  rec=dict(collection=sid,original_path=path,snapshot=str(target.relative_to(DB)),sha256=hashlib.sha256(data).hexdigest(),revision=r['observed_commit'],license=licenses[sid],url=r['url'],evidence_note='License/context snapshot; source execution not authorized or performed.')
  if not any(x['snapshot']==rec['snapshot'] for x in snapshots):snapshots.append(rec)
  if path!='README.md':
   notice=DB/'maintenance/licenses'/(sid+'-'+path.replace('/','-'))
   notice.write_bytes(data)
   s.setdefault('license_notices',[]).append(str(notice.relative_to(DB)))

for r in audit:
 s=existing[r['id']]
 s['checked_at']=DATE;s['last_run']=RUN
 if s['kind']=='knowledge-collection':
  s['observed_commit']=r['head_before']
  s['remote_observed_commit']=r.get('observed_commit')
  s['analyzed_commit']=None
  s['working_tree_status']=r['worktree_after']
  s['collection_review']=dict(status='complete-for-current-local-inventory',reviewed_on=DATE,map='maintenance/import-map.csv',inventory_review='maintenance/reviews/2026-10-27/collection-inventory.json',identity='exact local per-file SHA-256, including modified/untracked content',outcome='Mapped consumer/provenance content unchanged; two newly inventoried Flutter generated indexes are orchestration material, not reimports.')
  continue
 s['observed_commit']=r.get('observed_commit',s.get('observed_commit'))
 s['local_path']=r['local_path']
 if r.get('relationship')=='advanced':
  assert s.get('analyzed_commit')==r['previous_analyzed_commit']
  s['analyzed_commit']=r['observed_commit']
  s['incremental_review']=dict(previous_analyzed_commit=r['previous_analyzed_commit'],observed_commit=r['observed_commit'],commit_count=len(r['commits']),dispositions='maintenance/reviews/2026-10-27/commit-dispositions.json',coverage='Every reachable commit screened with merge-aware changed paths; relevant native/host/dependency/shared-code diffs and final snapshots reviewed. This is not a line-by-line audit of all generic business/media code.')
 elif r.get('relationship')=='previously-partial':
  s['analyzed_commit']=None
  s['review_status']='partial retained: unchanged vendor gitlinks and absent .gitmodules; backend resolution still incomplete'
 s['last_ref_audit']='maintenance/reviews/2026-10-27/ref-audit.json'

tauri=existing['tauri-open-harmony']
alias='https://github.com/SJMC-Dev/tauri-ohos-experimental'
if alias not in tauri['aliases']:tauri['aliases'].append(alias)
tauri['alias_evidence']='maintenance/reviews/2026-10-27/fork-comparisons.json; identical selected-ref commit, not independent runtime evidence'

# Build a candid candidate ledger: metadata-only screening is not source qualification.
candidates={}
def put(url,**fields):
 key=url.removesuffix('.git').rstrip('/').lower()
 candidates.setdefault(key,dict(url=url)).update(fields)

special={
 'david921518/tauri-mobile':('resource-list','README is a development-resource list, without an adaptation implementation.'),
 '2doright/airslate-pc-server':('different-target','Tauri desktop server accompanies a separate HarmonyOS application; no Tauri OH port established.'),
 'nanhapy/sms-bridge':('different-target','Windows Tauri receiver plus separate HarmonyOS phone client.'),
 'bluedeer000/clipboard-sync':('different-target','macOS client plus HarmonyOS phone; not a Tauri OH host.'),
 'Jesse205/F-OH-PWA':('different-target','PWA and desktop clients distribute HAP files; not a Tauri OH runtime.'),
 'bernardotmr/tauri-demo':('insufficient','README wraps an external download and generic desktop requirements; no source-backed OH remedy selected.'),
 'linlinjinzhen/tauri-ohos-test':('duplicate-family','Compared commit graph: zero ahead and one behind the already reviewed canonical test source.'),
 'jimmy54/tauri-demo':('duplicate-family','Compared commit graph: zero ahead and eight behind the already reviewed canonical demo.'),
 'SJMC-Dev/tauri-ohos-experimental':('duplicate','Selected branch identical to canonical tauri experimental checkpoint.'),
 'XingHaiOvO/APNG-Generator':('different-host','README separates Qt desktop, Flutter mobile and native ArkTS HarmonyOS implementations.'),
 'guoxiucai/ohos-scrcpy-app':('different-target','Flutter runs on macOS/Windows; the OH device hosts a separate system HAP.'),
 '123ningwang/KuiklyLoadView':('unqualified','README covers Android/iOS/Web, no concrete OH implementation established.'),
 'react-native-oh-library/react-native-harmony-gesture-handler':('moved','Followed declared migration to CPF-RN/rntpc_react-native-gesture-handler and reviewed native implementation there.'),
 'react-native-oh-library/react-native-harmony-screens':('moved','Followed declared migration to CPF-RN/rntpc_react-native-screens; fetched source inventory is primarily JS plus prebuilt HAR.'),
 'ohosvscode/electron-ohos-napi-shim':('known-mechanism-lead','README describes NAPI shim usage; owning-VM symbol resolution already represented. No binary runtime inference.'),
 'wildfirechat/hm-pc-chat':('screened-no-new-atom','Retried correct master branch. Electron 34 runtime/template, app asset injection and proprietary addon/product permission conditions repeat represented mechanisms; source README only, no addon source/runtime review.'),
}
for name in ['github-candidate-readmes.json','github-extra-screen.json']:
 for r in json.loads((HERE/name).read_text()):
  repo=r['repo'];url='https://github.com/'+repo
  disp,reason=special.get(repo,('metadata-screened','README/source-family screening did not establish an additional independently actionable OH-PC mechanism. No adaptation or duplicate claim from metadata alone.'))
  if r.get('error'):
   disp='inaccessible';reason=r['error']
  put(url,selected_ref=r.get('ref'),disposition=disp,reason=reason,evidence=name)
for name in ['candidate-fetch.json','candidate-wave2.json','candidate-wave3.json','candidate-wave4.json']:
 for r in json.loads((HERE/name).read_text()):
  sid=r['id']
  if sid in new_ids:disp='source-reviewed-contributed';reason='Pinned host/target/implementation inputs reviewed; consumer/provenance mapping retained.'
  elif sid=='gitee-kmp-harmony':disp='inaccessible';reason=r.get('error')
  elif sid=='xi-qt-ohos':disp='insufficient';reason='Selected main tree contains only README; organization dashboard claim is not implementation proof.'
  elif sid=='tauri-plugins-ohos':disp='unqualified';reason='Source inventory, dialog Cargo/mobile code and native grep have no OH implementation branch; package/branch name does not establish an OH plugin backend.'
  elif sid=='rnoh-screens':disp='insufficient-source';reason='Fetched migrated source has JS and a prebuilt HAR; native behavior not reviewed or reverse engineered. No new remedy selected from version table.'
  elif sid=='qtc-harmony-plugin':disp='tooling-screened';reason='Host Qt Creator development plugin, not an app runtime adaptation. No distinct mechanism beyond represented toolchain/SDK coherence selected.'
  elif sid=='psychopy-ohos':disp='screened-no-new-atom';reason='README and runtime/process-path screening repeat Electron runtime, private executable/dependency and local service boundaries. No full Python/native backend review claimed.'
  elif sid=='mqtt-client-tauri':disp='partial';reason='OH RustAbility/cdylib source present, but Tauri dependencies point to absent local ../tauri-ohos. Source-reported build not independently resolvable; version/patch lessons already covered.'
  elif sid=='m3u8dl-ohos':disp='not-qualified';reason='Selected tree has desktop Rust/Tauri source but no OH host/target/dependency inputs; repository title alone is insufficient.'
  elif sid=='shuyonote':disp='not-qualified';reason='Plans discuss OH; current selected tree has desktop/Web drivers without an actual OH host. No port inferred from roadmap.'
  elif sid=='cj-tauri':disp='different-framework';reason='Cangjie implementation inspired by Tauri, not Rust Tauri; HarmonyOS backend is a placeholder. No speculative framework atom.'
  else:disp='screened-no-new-atom';reason='No additional source-backed mechanism selected.'
  put(r['url'],source_id=sid,selected_ref=r['selected_ref'],observed_commit=r.get('observed_commit'),disposition=disp,reason=reason,evidence=name)
put('https://github.com/wildfirechat/hm-pc-chat',selected_ref='master',disposition=special['wildfirechat/hm-pc-chat'][0],reason=special['wildfirechat/hm-pc-chat'][1],evidence='closure-searches.json')
put('https://gitlab.com/leenhawk1/gproxy',disposition='known-family-lead',reason='Release page explicitly refers to artifacts built by GitHub Actions; already-reviewed gproxy source family. No independent runtime corroboration or exact mirror identity claim.')
put('https://gitlab.com/rzkn/qtbase',disposition='source-mirror-lead',reason='Qt framework source mirror surfaced; no additional app-specific mechanism established.')
put('https://github.com/HuakunShen/monio',disposition='insufficient-runtime-evidence',reason='Search/source description explicitly limits OH evidence to Rust checking, without link/package/device execution; no new working-input recipe extracted.')
put('https://gitcode.com/yjx000001/OpenSPH-HarmonyOS',disposition='known-host-mechanism-lead',reason='wxWidgets application using QAbility/dlopen entry mechanism already represented. No unreviewed wxWidgets category added.')
put('https://gitcode.com/gcw_vbU49vcA/bih_sfarklib',disposition='infrastructure-lead',reason='Large package/build registry, not independently reviewed adaptation projects. Package count and command smoke reports not imported as app/PC verification.')
put('https://gitee.com/yqg.tom/EasyTier',disposition='unqualified',reason='Search surfaced a workspace including an OH Rust contribution and a Tauri desktop GUI; this does not establish a Tauri OH port.')
put('https://codeberg.org/explore/repos',disposition='access-limited',reason='Web robots restriction; no negative ecosystem claim.')
dump(HERE/'candidate-dispositions.json',list(candidates.values()))

dump(HERE/'source-entry-map.json',[dict(source_id=sid,revision=ROWS[sid]['observed_commit'],entries=[dict(id=id,paths=paths) for id,paths in sorted(entries.items())]) for sid,entries in sorted(source_map.items())])
manifest['snapshots']=snapshots
dump(HERE/'curation-manifest.json',manifest)
dump(DB/'maintenance/sources.json',registry)
counts=collections.Counter(meta(id)['scope'] for id in manifest['added'])
fw=collections.Counter(f for id in manifest['added'] for f in meta(id)['frameworks'])
run=dict(schema_version=1,id=RUN[:-5],date=DATE,mode='update and expanded discovery',outcome='complete for recorded incremental ranges and selected mechanisms; documented accessible-search saturation, not global ecosystem exhaustion',source_ids=[s['id'] for s in registry['sources']],source_reviews=audit,added_entries=manifest['added'],extended_entries=manifest['extended'],counts=dict(database_before=215,database_after=236,added=len(manifest['added']),added_scope=dict(counts),added_framework_labels=dict(fw),extended=len(manifest['extended']),merged=0,split=0,registered_projects_before=39,registered_projects_after=58,new_projects=19,collections=2,unchanged_project_refs=33,advanced_project_refs=5,retained_partial_projects=1,screened_commits=len(ledger),commit_dispositions=dict(collections.Counter(x['disposition'] for x in ledger)),shortlisted_candidate_records=len(candidates)),review_records=['maintenance/reviews/2026-10-27/'+v for v in ['baseline.json','ref-audit.json','collection-inventory.json','commit-dispositions.json','candidate-dispositions.json','source-entry-map.json','curation-manifest.json','github-broad-search.json','github-framework-search.json','github-alias-search.json','github-fork-filtered-search.json','fork-comparisons.json','web-registry-searches.json','closure-searches.json']],verification_performed=['Pre-edit baseline index/check: 215 entries','Post-edit index/check: 236 entries, including metadata, exact snapshot hashes, related links and CSV equality','Source checkout/index/status preservation checked around fetch','Pending: final retrieval and diff checks recorded in validation.json'],limitations=['No source-project setup/migration/build/signing/install/device/emulator execution. All PC validation remains not-run.','All 121 commits were screened by paths, with merge-aware stats/diffs and selected native/shared implementation review; not an exhaustive line-by-line audit of generic business/media code.','Nineteen new records are selected-mechanism reviews at pinned snapshots, not feature-complete migrations or whole source histories.','Imported 60-project Flutter registry remains an imported snapshot; not relabelled as 60 current project-ref reviews.','One existing Tauri prototype remains partial with null checkpoint; one newly screened MQTT source has unresolved local framework dependencies and was not registered as a completed review.','Gitee KMP fetch remains inaccessible and Codeberg web access is robots-limited; metadata-only leads are not proof of absence.','Successive follow-up searches eventually returned represented source families, non-ports, plans, insufficient evidence or inaccessible leads in the reviewed result pages. Search cannot prove no valuable repository exists anywhere.','Source samples contain caveats: temporary Web Storage, native decoder format assumptions, plugin cleanup/race gaps, product-service gates, and an unsupported/incorrect SQLite linker option. Consumer guidance keeps these explicit.','All preexisting uncommitted curation content, imported snapshots and source-to-entry maps preserved.'])
dump(DB/'maintenance/runs'/RUN,run)
print(json.dumps(run['counts'],indent=2))

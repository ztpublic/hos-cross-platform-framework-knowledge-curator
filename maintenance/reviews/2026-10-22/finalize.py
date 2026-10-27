import collections
import hashlib
import json
import pathlib
import subprocess

DB=pathlib.Path(__file__).resolve().parents[3]
HERE=pathlib.Path(__file__).resolve().parent
SKILL='/Users/mac/.codex/skills/harmonyos-pc-knowledge-curator/scripts/kb.py'
RUN='2026-10-22-update-and-discovery'
def dump(p,v):
 p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n')
manifest=json.loads((HERE/'curation-manifest.json').read_text())
registry=json.loads((DB/'maintenance/sources.json').read_text())
original={s['id']:s for s in registry['sources']}
checks=[]
for command in ['index','check']:
 r=subprocess.run(['python3',SKILL,command],cwd=DB,capture_output=True,text=True,check=True);checks.append(r.stdout.strip())
searches=[('CURL_PEER_FAILED_VERIFICATION','kmp','native-curl-explicit-ca-bundle'),('files_3.ppm','qt','qt-ohos-save-picker-name-and-format'),('SkiaDomView','rn','rnoh-native-component-registration-parity'),('ecma_vm cannot run in multi-thread','kmp','kmp-ohos-resource-environment-main-thread'),('ptsname','electron','electron-ohos-conditional-pty-adapter'),('RTLD_NOLOAD','electron','electron-ohos-napi-symbol-owner'),('fromFileDescriptor','kmp','kmp-filekit-picker-descriptor-ownership')]
for q,framework,expected in searches:
 r=subprocess.run(['python3',SKILL,'search',q,'--framework',framework],capture_output=True,text=True,check=True)
 hits=[json.loads(l)['id'] for l in r.stdout.splitlines() if l.strip()]
 if not hits or hits[0]!=expected:raise ValueError((q,hits,expected))
 checks.append(dict(query=q,framework=framework,first_result=hits[0]))
subprocess.run(['git','diff','--check'],cwd=DB,check=True)
checks.append('git diff --check passed')

updates=json.loads((DB/'maintenance/update-2026-10-22-branches.json').read_text())+json.loads((DB/'maintenance/update-2026-10-22-other-branches.json').read_text())
reviewed=[]
for row in updates:
 s=original[row['id']];previous=s['analyzed_commit'];observed=row.get('observed')
 if not observed:raise ValueError('Unresolved registered ref '+row['id'])
 commits=[]
 if previous!=observed:
  compare=json.loads((HERE/(row['id']+'-compare.json')).read_text())
  if compare['status']!='ahead' or compare['total_commits']!=len(compare['commits']):raise ValueError('Incomplete range '+row['id'])
  for c in compare['commits']:
   sha=c['sha'];disposition='reviewed';reason='Reviewed full comparison and related source: new release notes identify the worker stack contract; no executable code changed in the range.'
   if row['id']=='dsh-desktop-hos':
    if sha.startswith('35a1947'):reason='Reviewed capability inventory and workflow declaration correction; documents runtime limitations already represented by capability-gating/non-PTY entries. No executable changes.'
    else:disposition='duplicate';reason='README/maintainer-guide relocation and links; no additional atomic remedy or executable changes.'
   commits.append(dict(commit=sha,subject=c['commit']['message'].splitlines()[0],disposition=disposition,reason=reason))
 s['observed_commit']=observed;s['analyzed_commit']=observed;s['checked_at']='2026-10-22';s['last_run']=RUN+'.json'
 reviewed.append(dict(source_id=s['id'],selected_ref=s['selected_ref'],from_commit=previous,to_commit=observed,outcome='reviewed' if commits else 'unchanged',commits=commits))

source_to_entries=collections.defaultdict(list)
for p in (DB/'provenance').glob('*.json'):
 prov=json.loads(p.read_text())
 for source_id in {s.get('collection') for s in prov['sources']}:
  if source_id:source_to_entries[source_id].append(prov['id'])
used={s['collection'] for s in manifest['snapshots']}
new=[]
for source_id in sorted(used-set(original)):
 spec=manifest['sources'][source_id]
 paths=sorted({s['original_path'] for s in manifest['snapshots'] if s['collection']==source_id})
 row=dict(id=source_id,kind='project',url=spec['url'],framework=spec['framework'],aliases=[],selected_ref=spec['ref'],observed_commit=spec['revision'],analyzed_commit=spec['revision'],license=spec['license'],local_path=spec.get('local_path',''),working_tree_status='Bare research clone; no source setup/build/checkout executed.' if spec.get('bare') else 'No checkout; pinned source text read through GitHub connector.',reviewed_paths=paths,knowledge_ids=sorted(source_to_entries[source_id]),last_run=RUN+'.json',checked_at='2026-10-22',coverage='Complete snapshot review for the listed mechanisms and paths; not an audit of every historical commit or application feature.',source_reported_validation='See entry provenance and retained source documents; no reported build/device result was rerun.')
 if spec.get('frameworks'):row['frameworks']=spec['frameworks']
 if source_id=='flutter-embed-into-qt':row['aliases']=['https://gitee.com/openharmony-sig/flutter_embed_into_qt','https://gitee.com/getmoon/flutter_embed_into_qt'];row['alias_note']='Archived lineage points to the current GitCode repository; old Gitee revisions are not counted as independent corroboration.'
 registry['sources'].append(row);new.append(source_id)

for s in registry['sources'][:2]:
 s['last_run']=RUN+'.json';s['checked_at']='2026-10-22';s['collection_review']['reviewed_on']='2026-10-22';s['collection_review']['inventory_review']='maintenance/reviews/2026-10-22/collection-full-inventory.json'
 if s['id']=='flutter-knowledge-import':s['remote_observed_commit']='90817867e21831c68a778ed1f30311514f482394';s['remote_relationship']='Remote main is an ancestor of the newer imported local HEAD; preserve exact local snapshots and analyzed_commit:null.'
 # Collection imports are never relabelled as independent source-project reviews.

candidates=[]
def candidate(project,disposition,reason,evidence='README/source inventory screening'):
 candidates.append(dict(project=project,disposition=disposition,reason=reason,evidence_level=evidence))
for sid in new:candidate(manifest['sources'][sid]['url'],'source-reviewed-added','Added independently actionable knowledge with pinned snapshots and explicit PC evidence boundaries.','Pinned implementation and target/build-input review')
rows=[
 ('gycrosskit/stately-ohos','duplicate','Adaptation branch documentation mainly adds OH target/artifact integration already represented; default main explicitly is not the published OH implementation.'),
 ('gycrosskit/koin-ohos','duplicate','Same target/dependency contract; read the explicit adaptation-branch README rather than treating main as the OH release.'),
 ('react-native-oh-library/react-native-ping','insufficient','Top-level documentation offers no distinct PC integration remedy; not promoted from package name alone.'),
 ('OrdinaryManZ/HarmonyAdaptQtDemo','insufficient','Current root has only a brief teaching README; no reviewable mechanism beyond existing Qt startup/packaging guidance.'),
 ('fellow99/oc-desktop-hos','duplicate','Wrapper documentation repeats the known Electron OH runtime, native ABI, packaging and capability-gating family; not counted as independent runtime validation.'),
 ('fellow99/pi-desktop-hos','duplicate','Same documented runtime/packaging family; no additional atom selected from the screened material.'),
 ('electerm/electerm-harmony','different-host','Actual host is ArkTS/ArkWeb plus native Node backend, not proof of an Electron OH port; no new atom beyond represented embedding boundaries selected.'),
 ('liriliri/echo','not-target','Desktop HDC controller for other OH devices, not evidence of an app running on OH PC.'),
 ('cuihairu/memex','not-target','README separates Qt desktop from an ArkTS OH client whose compile leg is pending; no Qt OH port established.'),
 ('oh-tpc/Result','duplicate','OH target and C-name/string bridge repeat reviewed KMP integration and ownership contracts; inspected docs retained, no independent feature claim.'),
 ('OpenHarmonyPCDeveloper/ohos_CodeCountTool','duplicate','Actual Qt host/background worker and URI normalization are source-present; independently actionable platform mechanisms already represented.'),
 ('kmp-cmp-tpc/KMPCMPOHOS','duplicate','OH target, native host and rawfile sample repeat the existing Compose host/resource contracts.'),
 ('weixin_52908342/OH-LiteIDE','insufficient','README describes external business build and host/target qmake mismatch; no distinct reviewed implementation beyond existing Qt SDK coherence entry.'),
 ('OpenHarmonyPCDeveloper/ohos_QjackCtl','insufficient','Host does not contain business source and explicitly lacks JACK backend; do not convert a working UI or NULL-returning shim into audio-routing success.'),
 ('HachimiWorld/haze-ohos','insufficient','Default snapshot has no OH-labelled implementation paths; upstream README alone is not an OH adaptation recipe. Client uses a separate CPF fork.'),
 ('Aloereed/mpc-qt-ohos','duplicate','Screened source inventory points to Qt/media packaging and platform stubs; no distinct mechanism beyond existing video/packaging entries selected.'),
 ('Cubeiic-HanXuan/cube-shell-cpp','insufficient','CMake and plan disagree on PTY. Exported forkpty/QProcess symbols are source/link evidence, not sandbox terminal verification; existing capability/PTY entries cover the safe decision.'),
 ('oh-react-native/react-native-app-settings','duplicate','Native implementation is product-specific settings startAbility with Promise acceptance, not completed permission modification; product gates/launch-result boundary already represented.'),
 ('OpenHarmonyPCDeveloper/ohos_kdevelop','not-qt-port','Primary README describes a packaged Electron application despite Qt/KDevelop project identity; generic runtime/packaging contract is already represented.'),
 ('OpenHarmonyPCDeveloper/ohos_electron_porter','duplicate','README describes automation of represented resfile/shim/template/permission steps; draw.io HAP build report is not feature/runtime verification.'),
 ('Joy777666/flojoy-studio-ohos','different-host','Search record explicitly replaces Electron with ArkTS/ArkWeb and CPython. No new in-scope framework mechanism established; direct page fetch timed out.'),
 ('licq_workspace/kmp-cmp-harmony','unavailable','Direct Gitee project page unavailable. Star-list description alone is not source evidence; no checkpoint or new knowledge claim.'),
 ('vnotex/vnote#2702','not-implemented','Primary issue discusses a possible port; no source adaptation established.'),
 ('GitLab rzkn/qtbase','duplicate-lead','Search surfaced a Qt source mirror, not a new app adaptation; no separate corroboration claimed.'),
 ('Codeberg HarmonyOS search','unavailable','Robots restrictions prevented access; no negative claim about all hosted projects.'),
]
for p,d,r in rows:candidate(p,d,r)
dump(HERE/'candidate-dispositions.json',candidates)
dump(HERE/'source-entry-map.json',{sid:sorted(source_to_entries[sid]) for sid in sorted(used)})
scope=collections.Counter();labels=collections.Counter();status=collections.Counter()
for id in manifest['added_entries']:
 meta=json.loads((DB/'knowledge'/f'{id}.md').read_text().split('---\n')[1]);scope[meta['scope']]+=1;labels.update(meta['frameworks']);status[meta['status']]+=1
run=dict(schema_version=1,date='2026-10-22',mode=['update','discover','collection-inventory-review'],outcome='complete-with-search-limitations',entries_before=187,entries_after=209,added_entries=manifest['added_entries'],extended_entries=manifest['extended_entries'],split_entries=manifest['split_entries'],merged_entries=[],new_source_ids=new,source_ids=sorted(set(original)|set(new)),registered_project_update=reviewed,collection_review='maintenance/reviews/2026-10-22/collection-full-inventory.json',counts=dict(registered_projects_checked=20,unchanged=18,advanced=2,new_commits_reviewed=3,new_projects=len(new),added_common=scope['common'],added_framework_specific=scope['framework-specific'],added_framework_labels=dict(labels),added_status=dict(status),snapshot_files=len(manifest['snapshots']),shortlisted_candidates=len(candidates)),candidate_dispositions='maintenance/reviews/2026-10-22/candidate-dispositions.json',verification=checks,search_coverage=dict(registries=['GitHub','GitCode/AtomGit','Gitee'],supplementary=['GitLab','Codeberg'],families=['Framework and platform aliases across Flutter, Qt, Electron, RN/RNOH, KMP/CMP','PC/2in1, target/build inputs and platform integration','Follow-up source families, archived mirrors, and broad closure queries'],github_search_records=['maintenance/discovery-2026-10-22-github.json','maintenance/discovery-2026-10-22-github-followup.json'],closure='maintenance/reviews/2026-10-22/closure-search.json',stop_reason='After extracting the later Expo/VSCodium/RNOH leads, the final broad search produced known source families, upstream documentation, non-adaptations or unrelated HAP results, without an additional source-backed distinct mechanism.'),limitations=['This is practical saturation of documented public searches, not an exhaustive inventory of every registry, private repo, branch or unindexed fork.','Some Gitee pages and Codeberg access were unavailable; candidates are recorded without fabricated negative results.','The 60-project registry in the Flutter import remains imported provenance, not a fresh independent review of those 60 projects.','No source application setup, build, signing, installation, emulator, physical device or runtime test was performed.','One new optional-argument lifetime issue is unresolved; all new PC validation fields remain not-run.'])
# Only publish checkpoints after the complete consumer/index/provenance validation above.
dump(DB/'maintenance/sources.json',registry)
dump(DB/'maintenance/runs'/f'{RUN}.json',run)
dump(HERE/'verification.json',checks)
report=['# Knowledge database update — 2026-10-22','',
 'Updated the database from 187 to **209 atomic entries**: **22 added**, **6 extended**, **1 common/native mechanism separated from its Flutter adapter**, and **0 duplicate mechanisms merged**. The split common entry is included in the 22 additions.','',
 'The additions contain **7 common** and **15 framework-specific** entries. Framework labels: '+', '.join(f'{k}: {v}' for k,v in sorted(labels.items()))+'. The Qt/Flutter embedding entry carries both labels, so label counts overlap.','',
 'Checked all **20 independently registered project refs**: 18 unchanged, two advanced, with all three new commits dispositioned. Registered **13 additional projects**, for 33 independently registered projects plus two imported collections. All analyzed checkpoints were published only after index/check passed.','',
 'Compared **127 Flutter collection files** and **160 Qt Markdown files** against saved inventories/snapshots, including local modifications, provenance and the Flutter project registry: no content changes. The Flutter remote main is older than the imported local HEAD; no snapshot or checkpoint was downgraded. The imported 60-project registry was preserved as an import, not relabelled as a fresh review.','',
 '## Added mechanisms','']
for id in manifest['added_entries']:
 meta=json.loads((DB/'knowledge'/f'{id}.md').read_text().split('---\n')[1]);report.append(f"- [{meta['title']}](../knowledge/{id}.md) — {meta['scope']}; {meta['status']}.")
report+=['','## New source coverage','']
for sid in new:
 spec=manifest['sources'][sid];report.append(f"- [{sid}]({spec['url']}) — `{spec['ref']}`, `{spec['revision']}`; "+', '.join(source_to_entries[sid])+'.')
report+=['','## Search closure and evidence','',f"Screened {len(candidates)} shortlisted project/registry candidates or source families; detailed outcomes are in [candidate dispositions](reviews/2026-10-22/candidate-dispositions.json). Primary searches covered GitHub, GitCode/AtomGit and Gitee; supplementary GitLab and Codeberg checks are recorded. The last broad pass produced known families, documentation, non-ports and unrelated HAP hits. No further source-backed distinct mechanism was identified in those accessible results.",'',
 'This is a bounded, documented saturation result, not proof that no valuable repository exists anywhere. An unavailable Gitee lead and Codeberg restrictions remain explicit; no monitoring was scheduled.','',
 'Validation passed: deterministic index generation; metadata, provenance hashes, related links and CSV equality for 209 entries; seven realistic symptom/framework retrieval checks; and git diff --check. Eighty pinned source/notice snapshots were retained. The native MMKV optional-argument lifetime issue remains unresolved. New device/simulator claims are source reports only; **no application build, install or runtime/PC validation was run**.','',
 'The VSCodium PTY report includes multiple app identities and does not establish ordinary-app PTY availability. The guidance requires validation with the intended legitimate app identity; it does not recommend impersonating a system application.','',
 '[Machine-readable run](runs/'+RUN+'.json) · [Source registry](sources.json) · [Source-to-entry map](reviews/2026-10-22/source-entry-map.json)','']
(DB/'maintenance/UPDATE-2026-10-22.md').write_text('\n'.join(report))
print(json.dumps(dict(entries=209,added=22,extended=6,new_projects=len(new),shortlist=len(candidates),new_common=dict(scope)),indent=2))

"""Persist curator-authored guidance with exact, commit-pinned source evidence (no source code is executed)."""
import hashlib,json,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
DB=HERE.parents[2]
DATE='2026-11-06'
R='/Users/mac/projects/harmonyos-kb-research/'
SRC={s['id']:s for s in json.loads((DB/'maintenance/sources.json').read_text())['sources']}
LATEST={r[0]:r[2] for r in json.loads((HERE/'ref-audit-raw.json').read_text())}
EXTRA={}  # new sources: id -> dict(local_path,observed_commit,url,license)
LICENSES={'dsh-desktop-hos':'MIT','drawio-in-harmony':'GPL-3.0','kuiklyui':'KuiklyUI License; custom license, existing notice retained.','liberika':'MPL-2.0; component notices apply.','rnoh-gesture-handler':'MIT'}
SNAPS={};ADDED=[];EXTENDED=[]
NOCOPY=set()  # sources whose files carry all-rights-reserved headers: store a hash-only evidence note, not the bytes
def dump(p,d):
 p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n')
def row(sid):
 if sid in EXTRA:return EXTRA[sid]
 s=SRC[sid];return dict(local_path=s['local_path'],observed_commit=LATEST[sid],url=s['url'],license=LICENSES.get(sid,s.get('license','unknown')))
def snap(sid,path):
 k=(sid,path)
 if k in SNAPS:return SNAPS[k]
 r=row(sid)
 data=subprocess.check_output(['git','-C',r['local_path'],'show',r['observed_commit']+':'+path],stderr=subprocess.PIPE)
 t=DB/'provenance/snapshots/projects'/sid/DATE/path
 orig=hashlib.sha256(data).hexdigest()
 if sid in NOCOPY:
  t=t.with_name(t.name+'.evidence.json')
  data=(json.dumps(dict(original_path=path,revision=r['observed_commit'],url=r['url'],original_sha256=orig,original_bytes=len(data),note='Source bytes not retained because file headers reserve all rights and no repository license grants redistribution. Read the pinned revision for the original.'),indent=2)+'\n').encode()
 t.parent.mkdir(parents=True,exist_ok=True);t.write_bytes(data)
 rec=dict(collection=sid,original_path=path,snapshot=str(t.relative_to(DB)),sha256=hashlib.sha256(data).hexdigest(),revision=r['observed_commit'],license=r['license'],url=r['url'],evidence_note='Pinned source review only; no source setup, build, device or PC execution.')
 SNAPS[k]=rec;return rec
def add(id,title,area,frameworks,summary,symptom,app,keywords,related,steps,conditions,mistakes,checks,evidence,status='scoped',limitations=(),reports=(),tier='source-reviewed'):
 for r in related:assert (DB/'knowledge'/f'{r}.md').exists() or r in PENDING,r
 meta=dict(schema_version=1,id=id,title=title,topic_key=area+'.'+id,area=area,scope='framework-specific' if frameworks else 'common',frameworks=frameworks,summary=summary,when_to_use=symptom,applicability=app,keywords=keywords.split('|'),related=related,status=status,evidence_tier=tier,pc_validation='not-run')
 body=f'---\n{json.dumps(meta,ensure_ascii=False,indent=2)}\n---\n\n# {title}\n\n## When to use\n\n{symptom}\n\n## Recommended approach\n\n{summary}\n\n## Implementation steps\n\n'+'\n'.join(f'{i}. {s}' for i,s in enumerate(steps,1))+f'\n\n## Conditions and alternatives\n\n{conditions}\n\n## Common mistakes\n\n{mistakes}\n\n## Check it works\n\n{checks}\n'
 (DB/'knowledge'/f'{id}.md').write_text(body)
 pv=dict(schema_version=1,id=id,sources=[snap(*v) for v in evidence],transformations=['Generalized the reviewed mechanism and removed project/research identity from consumer guidance.','Checks describe proposed acceptance cases; they were not executed.']+(['Common classification is a curator inference from the native/platform contract, not corroboration on other frameworks.'] if not frameworks else []),limitations=['No application build, device, emulator or PC validation performed by this curation.','Selected implementation paths only; published-package behavior is not established.']+list(limitations))
 if reports:pv['source_reported_evidence']=list(reports)
 dump(DB/'provenance'/f'{id}.json',pv);ADDED.append(id)
def extend(id,evidence,note,extra=None):
 p=DB/'provenance'/f'{id}.json';o=json.loads(p.read_text())
 for v in evidence:
  s=snap(*v)
  if s not in o['sources']:o['sources'].append(s)
 o['transformations'].append(note);dump(p,o)
 if extra:
  q=DB/'knowledge'/f'{id}.md';t=q.read_text();assert '## Common mistakes' in t;q.write_text(t.replace('## Common mistakes',extra+'\n\n## Common mistakes',1))
 EXTENDED.append(id)
PENDING=set()

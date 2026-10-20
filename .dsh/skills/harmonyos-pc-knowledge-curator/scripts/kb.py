#!/usr/bin/env python3
"""Deterministic CSV index and consumer search; Python standard library only."""
import argparse
import csv
import hashlib
import io
import json
from pathlib import Path
import re
import sys
import tempfile
import os
from collections import Counter

FRAMEWORKS = {'flutter', 'qt', 'electron', 'react-native', 'kotlin-multiplatform'}
ALIASES = {'rn': 'react-native', 'react native': 'react-native', 'kmp': 'kotlin-multiplatform', 'kotlin multiplatform': 'kotlin-multiplatform'}
FIELDS = 'schema_version id title scope frameworks area topic_key status evidence_tier pc_validation when_to_use summary keywords applicability related path provenance'.split()
STRINGS = 'id title scope area topic_key status evidence_tier pc_validation when_to_use summary applicability'.split()
HEADINGS = ['When to use', 'Recommended approach', 'Implementation steps', 'Conditions and alternatives', 'Common mistakes', 'Check it works']


def atomic_write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp = tempfile.mkstemp(prefix='.' + path.name, dir=path.parent)
    try:
        with os.fdopen(fd, 'w', encoding='utf-8', newline='') as f:
            f.write(text)
        os.replace(tmp, path)
    finally:
        if os.path.exists(tmp): os.unlink(tmp)


def contained(root, name):
    p = (root / name).resolve()
    if not p.is_relative_to(root.resolve()):
        raise ValueError('path escapes database: ' + name)
    return p


def read_entries(root):
    entries = []
    for p in sorted((root / 'knowledge').glob('*.md')):
        t = p.read_text(encoding='utf-8')
        match = re.fullmatch(r'---\n(.*?)\n---\n(.*)', t, re.S)
        if not match: raise ValueError(f'{p.name}: expected JSON frontmatter')
        m = json.loads(match[1]); body = match[2]
        if not isinstance(m, dict): raise ValueError(f'{p.name}: metadata must be object')
        entries.append((p, m, body))
    return entries


def csv_text(entries):
    out = io.StringIO(newline='')
    w = csv.DictWriter(out, fieldnames=FIELDS, lineterminator='\n'); w.writeheader()
    for p, m, body in sorted(entries, key=lambda e: e[1]['id']):
        row = {k: m[k] for k in FIELDS if k not in {'path', 'provenance'}}
        for k in ['frameworks', 'keywords', 'related']: row[k] = '|'.join(row[k])
        row['path'] = f"knowledge/{m['id']}.md"
        row['provenance'] = f"provenance/{m['id']}.json"
        w.writerow(row)
    return out.getvalue()


def validate(root, entries, check_index=True):
    errors = []; ids = set(); topics = set()
    for p, m, body in entries:
        tag = p.name
        def err(s): errors.append(f'{tag}: {s}')
        for k in STRINGS:
            if not isinstance(m.get(k), str) or not m[k].strip(): err(f'missing/non-string {k}')
        for k in ['frameworks', 'keywords', 'related']:
            if not isinstance(m.get(k), list) or any(not isinstance(v, str) or not v or '|' in v for v in m[k]): err(f'invalid {k} array')
        if any(k not in m for k in STRINGS + ['frameworks', 'keywords', 'related']): continue
        ident = m['id']
        if m.get('schema_version') != 1: err('schema_version must be 1')
        if not re.fullmatch('[a-z0-9]+(?:-[a-z0-9]+)*', ident) or p.stem != ident: err('invalid id/filename')
        if ident in ids: err('duplicate id')
        ids.add(ident)
        if m['topic_key'] in topics: err('duplicate topic_key')
        topics.add(m['topic_key'])
        if m['scope'] not in {'common', 'framework-specific'}: err('invalid scope')
        if isinstance(m['frameworks'], list):
            if not set(m['frameworks']) <= FRAMEWORKS: err('unknown framework')
            if len(set(m['frameworks'])) != len(m['frameworks']): err('duplicate framework')
            if (m['scope'] == 'common') != (not m['frameworks']): err('scope/framework mismatch')
        if m['status'] not in {'scoped', 'unresolved', 'verified'}: err('invalid status')
        if m['evidence_tier'] not in {'imported-guidance', 'source-reviewed', 'build-tested', 'device-tested', 'runtime-tested'}: err('invalid evidence tier')
        if m['pc_validation'] not in {'not-run', 'verified'}: err('invalid PC validation')
        if m['status'] == 'verified' and m['pc_validation'] != 'verified': err('verified status needs PC evidence')
        for heading in HEADINGS:
            if f'## {heading}\n' not in body: err('missing section ' + heading)
        if re.search(r'\b[0-9a-f]{40}\b|/Users/|\b20\d{2}-\d{2}-\d{2}\b', json.dumps(m) + body): err('maintenance identity/date leaked into consumer guidance')
        prov = root / 'provenance' / (ident + '.json')
        try:
            data = json.loads(prov.read_text())
            if data.get('id') != ident or data.get('schema_version') != 1: err('provenance identity/schema mismatch')
            if not data.get('sources'): err('no provenance sources')
            for field in ['transformations', 'limitations']:
                if not isinstance(data.get(field), list): err('missing provenance ' + field)
            for source in data.get('sources', []):
                for field in ['collection','original_path','snapshot','sha256','revision','license']:
                    if field not in source: err('source missing ' + field)
                src = contained(root, source['snapshot'])
                if hashlib.sha256(src.read_bytes()).hexdigest() != source['sha256']: err('source snapshot hash mismatch: ' + source['snapshot'])
            if m['pc_validation'] == 'verified':
                test = data.get('pc_test', {})
                if not all(test.get(k) for k in ['device_class','os_api','build_inputs','behavior','artifact','artifact_sha256']): err('missing PC test evidence')
                else:
                    evidence = contained(root, test['artifact'])
                    if hashlib.sha256(evidence.read_bytes()).hexdigest() != test['artifact_sha256']: err('PC evidence hash mismatch')
        except (OSError, ValueError, KeyError, TypeError) as ex: err('provenance: ' + str(ex))
    for p, m, body in entries:
        for ref in m.get('related', []):
            if ref not in ids: errors.append(f'{p.name}: unknown related id {ref}')
        for target in re.findall(r'\]\(([^)]+)\)', body):
            if re.match(r'[a-z]+://|#', target): continue
            try:
                dest = contained(root, str(Path('knowledge') / target.split('#')[0]))
                if not dest.exists(): errors.append(f'{p.name}: broken link {target}')
            except ValueError as ex: errors.append(str(ex))
    extra = {p.stem for p in (root / 'provenance').glob('*.json')} - ids
    if extra: errors.append('orphan provenance: ' + ', '.join(sorted(extra)))
    if check_index and not errors:
        path = root / 'index.csv'
        if not path.exists() or path.read_bytes() != csv_text(entries).encode('utf-8'): errors.append('index.csv is absent or stale; run index')
    return errors


def occurrences(text, term):
    pattern = re.escape(term)
    if re.fullmatch(r'[a-z0-9_.-]+', term): pattern = r'(?<![a-z0-9_])' + pattern + r'(?![a-z0-9_])'
    return len(re.findall(pattern, text.casefold()))


def search(entries, query, framework=None, scope=None, limit=10):
    terms = [ALIASES.get(q.casefold(), q.casefold()) for q in re.findall(r'"([^"]+)"|(\S+)', query) for q in [q[0] or q[1]]]
    results=[]
    for p,m,body in entries:
        if scope and m['scope'] != scope: continue
        if framework and m['scope'] != 'common' and framework not in m['frameworks']: continue
        weighted = [(m['title'], 8), (' '.join(m['keywords']), 6), (m['when_to_use'], 4), (m['summary'], 4), (m['applicability'], 2), (body, 1)]
        score = sum(min(occurrences(t,q),3)*w for q in terms for t,w in weighted)
        if score: results.append((score,m))
    return sorted(results,key=lambda x:(-x[0],x[1]['id']))[:limit]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--database', type=Path)
    sub=parser.add_subparsers(dest='command',required=True)
    for command in ['init','index','check','stats']: sub.add_parser(command)
    sp=sub.add_parser('search'); sp.add_argument('query'); sp.add_argument('--framework'); sp.add_argument('--scope',choices=['common','framework-specific']); sp.add_argument('--limit',type=int,default=10)
    a=parser.parse_args()
    root = a.database
    if root is None:
        cfg=Path(__file__).resolve().parents[1]/'config.json'
        root=Path(json.loads(cfg.read_text())['database'])
    root=root.expanduser().resolve()
    if a.command=='init':
        if any((root/p).exists() for p in ['knowledge','provenance','index.csv']): raise ValueError('database already exists; init refuses overwrite')
        for name in ['knowledge','provenance','maintenance/runs']: (root/name).mkdir(parents=True,exist_ok=True)
        atomic_write(root/'index.csv',csv_text([])); print('Initialized ' + str(root)); return
    if not (root/'knowledge').is_dir() or not (root/'provenance').is_dir(): raise ValueError('not a knowledge database: ' + str(root))
    entries=read_entries(root)
    errors=validate(root,entries,check_index=a.command!='index')
    if errors:
        for err in errors: print(err,file=sys.stderr)
        sys.exit(1)
    if a.command=='index': atomic_write(root/'index.csv',csv_text(entries)); print(f'Indexed {len(entries)} entries')
    elif a.command=='check': print(f'OK: {len(entries)} entries; metadata, provenance hashes, links and CSV index agree')
    elif a.command=='stats':
        print(json.dumps({'total':len(entries),'scope':dict(Counter(m['scope'] for _,m,_ in entries)),'framework_specific':{f:sum(f in m['frameworks'] for _,m,_ in entries) for f in sorted(FRAMEWORKS)},'pc_validation':dict(Counter(m['pc_validation'] for _,m,_ in entries))},indent=2))
    else:
        framework=ALIASES.get((a.framework or '').casefold(),(a.framework or '').casefold()) or None
        if framework and framework not in FRAMEWORKS: raise ValueError('unknown framework ' + framework)
        results=search(entries,a.query,framework,a.scope,a.limit)
        for score,m in results:
            print(json.dumps({'id':m['id'],'title':m['title'],'scope':m['scope'],'frameworks':m['frameworks'],'score':score,'summary':m['summary'],'pc_validation':m['pc_validation'],'path':str(root/'knowledge'/(m['id']+'.md'))},ensure_ascii=False))
        if not results: print('No matching consumer entries.')


if __name__=='__main__':
    try: main()
    except (OSError,ValueError,KeyError,TypeError) as ex:
        print('Error: '+str(ex),file=sys.stderr); sys.exit(1)

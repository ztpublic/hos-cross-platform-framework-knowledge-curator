"""Read-only source audit; fetch objects without checking out or running source code."""
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import subprocess

DB = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
RESEARCH = Path('/Users/mac/projects/harmonyos-kb-research/2026-10-27')
ENV = dict(os.environ, GIT_TERMINAL_PROMPT='0', GIT_SSH_COMMAND='ssh -o BatchMode=yes -o ConnectTimeout=12')

def git(path, *args, timeout=50):
    p = subprocess.run(['git', '-C', str(path), *args], capture_output=True, env=ENV, timeout=timeout)
    if p.returncode:
        raise RuntimeError(p.stderr.decode(errors='replace')[-2000:])
    return p.stdout.decode(errors='replace').strip()

def dump(name, obj):
    (HERE/name).write_text(json.dumps(obj, ensure_ascii=False, indent=2)+'\n')

def audit(s):
    r = dict(id=s['id'], url=s['url'], selected_ref=s.get('selected_ref'), previous_analyzed_commit=s.get('analyzed_commit'))
    path = Path(s.get('local_path') or RESEARCH/(s['id']+'.git'))
    r['local_path'] = str(path)
    try:
        if not path.exists():
            path.mkdir(parents=True)
            git(path, 'init', '--bare')
        bare = git(path, 'rev-parse', '--is-bare-repository') == 'true'
        r['worktree_before'] = None if bare else git(path, 'status', '--porcelain')
        r['head_before'] = git(path, 'rev-parse', 'HEAD') if not bare else None
        ref = s.get('selected_ref') or git(path, 'symbolic-ref', '--short', 'HEAD')
        r['selected_ref'] = ref
        git(path, 'fetch', '--no-tags', '--no-recurse-submodules', s['url'], 'refs/heads/'+ref)
        r['observed_commit'] = git(path, 'rev-parse', 'FETCH_HEAD')
        old = s.get('analyzed_commit')
        if old:
            try:
                git(path, 'cat-file', '-e', old+'^{commit}')
            except RuntimeError:
                git(path, 'fetch', '--no-tags', '--no-recurse-submodules', s['url'], old)
            try:
                git(path, 'merge-base', '--is-ancestor', old, r['observed_commit'])
                r['relationship'] = 'unchanged' if old == r['observed_commit'] else 'advanced'
            except RuntimeError:
                r['relationship'] = 'not-descendant'
            r['commits'] = [dict(zip(['commit','parents','subject'], line.split('\t',2))) for line in git(path, 'log', '--reverse', '--format=%H%x09%P%x09%s', old+'..'+r['observed_commit']).splitlines()]
            r['diff_stat'] = git(path, 'diff', '--stat', old, r['observed_commit'])
            if r['commits']:
                (HERE/(s['id']+'-range.diff')).write_text(git(path, 'diff', '--no-ext-diff', '--no-renames', old, r['observed_commit'])+'\n')
                (HERE/(s['id']+'-commit-stats.txt')).write_text(git(path, 'log', '-m', '--reverse', '--stat', '--format=commit %H %P%n%s', old+'..'+r['observed_commit'])+'\n')
        else:
            r['relationship'] = 'collection' if s['kind']=='knowledge-collection' else 'previously-partial'
        r['worktree_after'] = None if bare else git(path, 'status', '--porcelain')
        r['head_after'] = git(path, 'rev-parse', 'HEAD') if not bare else None
        r['outcome'] = 'fetched'
    except Exception as e:
        r['outcome'] = 'inaccessible-or-incomplete'
        r['error'] = str(e)
    return r

def main():
    HERE.mkdir(parents=True, exist_ok=True)
    RESEARCH.mkdir(parents=True, exist_ok=True)
    # Hash baseline knowledge, provenance, registry and README, including preexisting uncommitted work.
    dump('baseline.json', {str(p.relative_to(DB)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [*sorted((DB/'knowledge').glob('*.md')), *sorted((DB/'provenance').glob('*.json')), DB/'maintenance/sources.json', DB/'README.md', DB/'index.csv']})
    sources = json.loads((DB/'maintenance/sources.json').read_text())['sources']
    rows=[]
    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
        for f in concurrent.futures.as_completed([pool.submit(audit,s) for s in sources]):
            r=f.result(); rows.append(r)
            dump('ref-audit.json', sorted(rows,key=lambda r:r['id']))
            print(r['id'], r['outcome'], r.get('relationship',''), len(r.get('commits',[])), flush=True)

if __name__=='__main__':
    main()

"""Read pinned source objects; never build, check out, reset, or edit source apps."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from urllib.parse import quote

OUT = Path(__file__).resolve().parent
DB = OUT.parents[2]
FLUTTER = Path('/Users/mac/projects/flutter-experiment')
registry = json.loads((DB / 'maintenance/sources.json').read_text())['sources']
flutter = json.loads((DB / 'provenance/snapshots/flutter/projects.json').read_text())['projects']
sources = {p['id']: dict(p, revision=p.get('analyzed_commit'), repository=p['url'])
           for p in registry if p.get('kind') == 'project'}
sources.update({p['id']: dict(p, revision=p['analyzed_head'],
                            local_path=str(FLUTTER / p['clone_path'])) for p in flutter})

selected = {
 'ohos-ktouch': ['README.OpenHarmony_CN.md', 'src/ktouchcontext.cpp',
                 'harmony_pc/HARMONYOS_PORT.md'],
 'scidavis-for-harmonyos': ['README.md', 'entry/src/main/cpp/CMakeLists.txt'],
 'klayout-on-harmony-pc': ['docs/PORTING-NOTES.md', 'patches/klayout-0.30.10-ohos.patch'],
 'freecad-on-harmonyos': ['README.md', 'docs/sketcher-rendering-debug-playbook.md',
     'patches/freecad-1.1.2/ohos-enable-navicube-axiscross.patch',
     'patches/freecad-1.1.2/ohos-native-uitools.patch'],
 'dsh-desktop-hos': ['README.md', 'specs/010-tool-bash/spec.md',
     'scripts/lib/sharp-stub-body.js', 'web_engine/src/main/ets/adapter/ImageAdapter.ets',
     'web_engine/src/main/ets/adapter/StatusBarManager.ets'],
 'doubao-watermark-lab': ['src/main.js'],
 'ohos-jupyter': ['README.md',
     'electron/src/main/resources/resfile/resources/app/src/js/staticLabServer.js'],
 'drawio-in-harmony': ['FEASIBILITY.md', 'FEATURE_MATRIX.md'],
 'react-native-harmony-seed': ['docs/rnoh/react-native-webview.md'],
 'pure-live-ohos': ['README_OHOS.md'],
 'kelivo': ['README.md', 'lib/features/settings/pages/settings_page.dart',
     'lib/shared/widgets/markdown_with_highlight.dart'],
 'beecount-oh': ['lib/utils/notification_factory.dart', 'lib/pages/mine_page.dart'],
 'bugaoshan': ['lib/services/ocr_service.dart'],
 'piliplus': ['lib/pages/music/view.dart'],
 'etohos': ['lib/utils/text_field_utils.dart'],
 'oronbox': ['README.md', 'lib/src/core/services/ohos_ble_channel.dart'],
 'mi-workbench': ['OHOS_BUILD.md', 'lib/screens/device_screen.dart'],
 'neon': ['packages/neon_framework/lib/src/platform/ohos.dart'],
 'connssh': ['lib/pages/sftpview.dart'],
 'fusheng-music': ['README.md'],
 'nipaplay': ['packages/universal_gamepad_ohos_stub/lib/universal_gamepad.dart'],
}
pattern = re.compile(
 'MVP|not.support|unsupported|not.implement|不支持|未实现|未适配|禁用|移除|暂缺|'
 'nopython|noruby|canUse(Camera|Push|Window)|toSpeech|stubBridge|recognizeAsync|'
 r'QUiLoader|navicube|default_disabled|addToStatusBar|new Tray|auto.updat|no.op|empty\(|'
 'AppClipboard|Clipboard|allowMultiple|filesToDownload.length|no thumbnails', re.I)

def run(root, args):
    env = dict(os.environ, GIT_NO_LAZY_FETCH='1')
    return subprocess.run(['git', '-c', 'core.quotePath=false', '-C', root, *args],
                          capture_output=True, timeout=35, env=env)

evidence = []
fingerprints = {}
for pid, paths in selected.items():
    p = sources[pid]
    root = p.get('local_path', '')
    if root and (Path(root) / '.git').exists():
        status = run(root, ['status', '--porcelain=v1', '-uno'])
        head = run(root, ['rev-parse', 'HEAD'])
        fingerprints[pid] = {'head': head.stdout.decode().strip(),
                             'tracked_status_sha256': hashlib.sha256(status.stdout).hexdigest()}
    for path in paths:
        record = {'project': pid, 'revision': p['revision'], 'path': path,
                  'repository': p['repository'], 'license': p.get('license', 'unknown')}
        if p['repository'].startswith('https://github.com/'):
            record['url'] = p['repository'].rstrip('/') + '/blob/' + p['revision'] + '/' + quote(path)
        if root and (Path(root) / '.git').exists():
            result = run(root, ['show', p['revision'] + ':' + path])
            if result.returncode:
                record.update(status='unavailable', error=result.stderr.decode(errors='replace')[:400])
                evidence.append(record)
                continue
            data = result.stdout
            record['read_method'] = 'git-show-pinned-object'
        else:
            snap = DB / 'provenance/snapshots/projects' / pid / path
            data = snap.read_bytes()
            record.update(read_method='preserved-source-snapshot', snapshot=str(snap.relative_to(DB)))
        lines = data.decode(errors='replace').splitlines()
        hits = [i for i, line in enumerate(lines) if pattern.search(line)]
        record.update(status='read', sha256=hashlib.sha256(data).hexdigest(),
                      line_count=len(lines), matching_lines=[i + 1 for i in hits],
                      excerpts=[{'line': i + 1, 'text': lines[i][:500]} for i in hits[:36]])
        evidence.append(record)

history = []
for pid, revision, paths in [
 ('bugaoshan', '24585fd3ca55cf98834107929098001b80ab3729', ['lib/serivces/ocr_service.dart']),
 ('bugaoshan', 'a691ace11e4ea07ea317d50ae656967f9538259c', ['lib/services/ocr_service.dart']),
 ('piliplus', '3ef92124b8cc6746e8e9c183e4aaf94109d6bfcb', ['lib/pages/music/view.dart']),
]:
    p = sources[pid]
    result = run(p['local_path'], ['show', '--format=', revision, '--', *paths])
    data = result.stdout
    diff_lines = data.decode(errors='replace').splitlines()
    history.append({'project': pid, 'revision': revision, 'paths': paths,
        'url': p['repository'] + '/commit/' + revision,
        'status': 'read' if result.returncode == 0 else 'unavailable',
        'diff_sha256': hashlib.sha256(data).hexdigest(),
        'selected_diff_lines': [s for s in diff_lines if re.search(
            r'PlatformNotSupported|OS.isHarmony|fl_chart|TODO flutter|child: LineChart', s)][:18]})

preservation = []
for pid, before in fingerprints.items():
    root = sources[pid]['local_path']
    after = {'head': run(root, ['rev-parse', 'HEAD']).stdout.decode().strip(),
             'tracked_status_sha256': hashlib.sha256(run(root, ['status', '--porcelain=v1', '-uno']).stdout).hexdigest()}
    preservation.append({'project': pid, 'before': before, 'after': after, 'unchanged': before == after})

flutter_counts = dict(zip([p['id'] for p in flutter], [
    0,0,2,7,2,0,0,0,5,18,0,0,0,0,0,0,4,1,9,2,0,5,0,0,0,2,0,0,0,0,
    0,0,0,0,1,0,0,0,0,5,0,0,0,5,0,0,4,0,5,0,7,2,1,1,0,1,0,0,0,1]))
direct_counts = {'dsh-desktop-hos':294, 'klayout-on-harmony-pc':2, 'ohos-pc-launcher':0,
    'compose-multiplatform-harmony':0, 'react-native-harmony-seed':11,
    'doubao-watermark-lab':1, 'ohshark':2, 'vlc-qt6-ohos':2,
    'freecad-on-harmonyos':10, 'scidavis-for-harmonyos':1, 'electron-egg-ohos':2,
    'ohos-ktouch':41, 'drawio-in-harmony':19, 'ohos-jupyter':3, 'harmonykmp':0,
    'ohos-electron-hap':0, 'notepad-ohos':1}
inventory = []
for pid, p in sources.items():
    inventory.append({'id': pid, 'repository': p['repository'], 'revision': p['revision'],
        'framework': p.get('framework', 'flutter'),
        'coverage': ('selected-feature-source-review' if pid in selected else
                     'candidate-screen-only'),
        'screen_limitation': ('Broad Git grep interrupted while resolving promisor blobs; partial only.'
                             if pid == 'pilinara' else None),
        'candidate_matching_line_count': flutter_counts.get(pid, direct_counts.get(pid)),
        'screen_method': ('pinned-git-grep-flutter' if pid in flutter_counts else
                          'pinned-git-grep-documentation' if pid in direct_counts else
                          'preserved-snapshot-rg'),
        'exclusion_of_absence_claim': 'No candidate or no detailed finding does not establish feature parity.'})

payload = {'schema_version': 1, 'date': '2026-10-21',
 'scope': 'Pinned registered revisions, not a latest-branch update or runtime reproduction.',
 'files': evidence, 'historical_diffs': history, 'source_preservation': preservation,
 'limitations': ['Source and documentation only; no app built, signed, installed or exercised.',
    'Before/after fingerprints cover selected readable source repositories, tracked status and HEAD only; untracked files and index bytes were not audited.',
    'Some earlier read-only object access fetched missing promisor blobs; no checkout/reset/clean was used.',
    'Excerpts are navigation evidence, not complete source snapshots; git object revision and full-file SHA-256 identify the original bytes.']}
(OUT / 'evidence.json').write_text(json.dumps(payload, ensure_ascii=False, indent=2) + '\n')
(OUT / 'coverage.json').write_text(json.dumps({'schema_version': 1, 'projects': inventory,
    'registry_inputs': ['maintenance/sources.json', 'provenance/snapshots/flutter/projects.json'],
    'screen': {'flutter': '59 completed keyword screens, one partial (pilinara)',
       'direct': '17 repository documentation screens; 3 preserved-snapshot screens',
       'counts_origin': 'Observed output of discovery commands during this investigation; counts include false positives. PiliNara count is partial.',
       'flutter_regex': '(鸿蒙.{0,16}(不支持|暂不|禁用)|ohos.{0,30}(not support|unsupported|disabled)|not support.{0,30}ohos|HarmonyOS.{0,30}(not support|unsupported)|暂不支持|移除.{0,16}(功能|鸿蒙)|TODO.{0,20}OHOS)',
       'flutter_pathspecs': ['*.md', 'lib/*.dart', 'lib/**/*.dart'],
       'direct_regex': '(不支持|未实现|禁用|暂不支持|not supported|not implemented|MVP|stub|disable.*(python|print|tray|plugin|updat|scripting))',
       'direct_pathspecs': ['*.md', '*.cmake', 'CMakeLists.txt'],
       'meaning': 'Candidate discovery only. Selected source review is explicitly marked per project.'},
    'imported_qt_collection': 'Used as secondary leads and counterexample context; its transitive project set was not independently inventoried or fully re-reviewed.'}, ensure_ascii=False, indent=2) + '\n')
print(json.dumps({'projects': len(inventory), 'selected_projects': len(selected),
    'files_read': sum(x['status'] == 'read' for x in evidence),
    'files_unavailable': [x for x in evidence if x['status'] != 'read'],
    'historical_diffs': len(history),
    'selected_repositories_unchanged': all(x['unchanged'] for x in preservation)}, ensure_ascii=False))

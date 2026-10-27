"""Fetch candidate source objects and save inventories; execute no project code."""
import concurrent.futures
import json
from audit_sources import git, dump, RESEARCH, HERE

CANDIDATES = [
 ('cc-switch-ohos','https://gitcode.com/OpenHarmonyPCDeveloper/ohos_cc-switch','main','tauri'),
 ('qterminal-ohos','https://gitcode.com/ohos-qt/qterminal','main','qt'),
 ('xi-qt-ohos','https://gitcode.com/ohos-qt/xi-qt','main','qt'),
 ('strawberry-ohos','https://gitcode.com/ohos-qt/strawberry','main','qt'),
 ('letos-ohos','https://gitcode.com/m0_52900670/letos','main','qt'),
 ('rnoh-keep-awake','https://gitcode.com/oh-react-native/react-native-keep-awake','main','react-native'),
 ('rnoh-splash-screen','https://gitcode.com/ywp791322/rntpc_react-native-splash-screen','master','react-native'),
 ('electron-shell','https://github.com/fellow99/ElectronShell','main','electron'),
 ('psychopy-ohos','https://github.com/Aik358/psychopy-studio-harmonyos','main','electron'),
 ('coil-ohos','https://github.com/crowforkotlin/coil','ohos','kotlin-multiplatform'),
 ('flutter-add2app','https://github.com/0xZOne/ohos-flutter-add2app','main','flutter'),
 ('flutter-record-ohos','https://github.com/bruce3x/flutter_record_ohos','bruce/ohos-startstream-fix','flutter'),
 ('flutter-app-links','https://github.com/HarmonyCandies/app_links_ohos','main','flutter'),
 ('tauri-plugins-ohos','https://github.com/SJMC-Dev/tauri-plugins-workspace-ohos-experimental','feat/open-harmony','tauri'),
 ('qtc-harmony-plugin','https://github.com/liys-online/qtc-for-harmonyos','main','qt'),
]

def fetch(c):
 sid,url,ref,framework=c
 p=RESEARCH/(sid+'.git')
 r=dict(id=sid,url=url,selected_ref=ref,framework=framework,local_path=str(p))
 try:
  if not p.exists():p.mkdir();git(p,'init','--bare')
  git(p,'fetch','--depth=1','--filter=blob:none','--no-tags','--no-recurse-submodules',url,'refs/heads/'+ref,timeout=180)
  r['observed_commit']=git(p,'rev-parse','FETCH_HEAD')
  paths=git(p,'ls-tree','-r','--name-only',r['observed_commit']).splitlines()
  (HERE/(sid+'-inventory.txt')).write_text('\n'.join(paths)+'\n')
  readmes=[v for v in paths if '/' not in v and (v.lower().startswith(('readme','license','copying')))]
  r['screened_paths']=readmes
  for f in readmes:
   data=git(p,'show',r['observed_commit']+':'+f,timeout=120)
   (HERE/(sid+'-'+f.replace('/','_'))).write_text(data+'\n')
  r['outcome']='fetched'
 except Exception as e:r.update(outcome='incomplete',error=str(e))
 return r

if __name__=='__main__':
 rows=[]
 with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
  for f in concurrent.futures.as_completed([pool.submit(fetch,c) for c in CANDIDATES]):
   r=f.result();rows.append(r);dump('candidate-fetch.json',rows)
   print(r['id'],r['outcome'],r.get('observed_commit'),r.get('error',''),flush=True)

import json,subprocess,hashlib,shutil,re
from curate_lib import *
exec(open('curate_update.py').read().split('\n',1)[1])
exec(open('curate_discovery.py').read().split('\n',1)[1])
exec(open('curate_electron.py').read().split('\n',1)[1])
dump(HERE/'curation-manifest.json',dict(added=ADDED,extended=EXTENDED))
print(len(ADDED),ADDED,EXTENDED)

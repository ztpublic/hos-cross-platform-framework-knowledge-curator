"""Display pinned source files for human/agent review; no source execution."""
import json
from pathlib import Path
import subprocess
import sys

HERE=Path(__file__).resolve().parent
ROWS={r['id']:r for name in ['ref-audit.json','candidate-fetch.json','candidate-wave2.json','candidate-wave3.json','candidate-wave4.json'] for r in json.loads((HERE/name).read_text())}

if __name__=='__main__':
 r=ROWS[sys.argv[1]]
 for path in sys.argv[2:]:
  print('\nFILE',r['id'],path,flush=True)
  p=subprocess.run(['git','-C',r['local_path'],'show',r['observed_commit']+':'+path],capture_output=True)
  print(p.stdout.decode(errors='replace') if p.returncode==0 else p.stderr.decode(errors='replace'))

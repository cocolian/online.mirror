#!/usr/bin/env python3
from pathlib import Path
import concurrent.futures,hashlib,json,subprocess,sys,time
root=Path(__file__).resolve().parents[1]
build=root/'build/source-19b4efae7fb8'
cache=build/'source/engine/external/tarballs'
tmp=cache/'prefetch-tmp';tmp.mkdir(exist_ok=True)
items=json.loads((build/'fetch-list.json').read_text())
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for c in iter(lambda:f.read(1024*1024),b''):h.update(c)
 return h.hexdigest()
def fetch(item):
 name=item['url'].rsplit('/',1)[1];target=cache/name
 if target.exists() and sha(target)==item['sha256']: return 'CACHED '+name
 partial=tmp/name
 for attempt in range(3):
  result=subprocess.run(['curl','--fail','--location','--connect-timeout','20','--max-time','1200','--retry','2','--continue-at','-','--silent','--show-error','--output',str(partial),item['url']],capture_output=True,text=True)
  if result.returncode==0 and sha(partial)==item['sha256']:
   partial.replace(target)
   return 'FETCHED '+name+' '+str(target.stat().st_size)
  if result.returncode==0:
   partial.rename(tmp/(name+'.bad-'+str(time.time_ns())))
  time.sleep(1)
 raise RuntimeError(name+' '+result.stderr[-300:])
failed=[]
with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
 futures={pool.submit(fetch,item):item for item in items}
 for i,f in enumerate(concurrent.futures.as_completed(futures),1):
  try: print(str(i)+'/'+str(len(items))+' '+f.result(),flush=True)
  except Exception as e:failed.append(str(e));print('FAILED '+str(e),flush=True)
if failed: raise SystemExit(1)
print('ALL_DEPENDENCIES_VERIFIED',flush=True)

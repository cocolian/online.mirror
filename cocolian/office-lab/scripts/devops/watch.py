#!/usr/bin/env python3
"""Outbound main-branch watcher; no public webhook or GitHub write token."""
import argparse,fcntl,hashlib,json,os,subprocess,time,traceback
from pathlib import Path
REPOS={n:f'git@github.com:cocolian/{n}.git' for n in ('cocolian','cocolian-office','online.mirror')}
ROOT=Path(os.environ.get('DEVOPS_STATE',Path.home()/'.local/share/cocolian-devops'))
def command(args,**kw):
 return subprocess.check_output(args,text=True,timeout=kw.pop('timeout',180),**kw).strip()
def write_json(path,value):
 temp=path.with_suffix('.tmp');temp.write_text(json.dumps(value,indent=2,ensure_ascii=False)+'\n');temp.replace(path)
def fingerprint(heads):return hashlib.sha256(json.dumps(heads,sort_keys=True).encode()).hexdigest()[:16]
def checkout(name,sha):
 path=ROOT/'checkouts'/name
 if not (path/'.git').exists():
  path.parent.mkdir(parents=True,exist_ok=True)
  command(['git','clone','--no-checkout','--filter=blob:none','--single-branch','--branch','main',REPOS[name],str(path)],timeout=1800)
 command(['git','-C',str(path),'fetch','--filter=blob:none','origin','main'],timeout=1800)
 # Only automation-owned worktrees are reset.
 command(['git','-C',str(path),'checkout','--detach','--force',sha],timeout=1800)
 return path

def main():
 parser=argparse.ArgumentParser();parser.add_argument('--force',action='store_true');args=parser.parse_args()
 ROOT.mkdir(parents=True,exist_ok=True)
 with (ROOT/'lock').open('a') as lock:
  try:fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
  except BlockingIOError:print('Another release holds the lock',flush=True);return
  statefile=ROOT/'state.json';state=json.loads(statefile.read_text()) if statefile.exists() else {}
  try:
   heads={}
   for name,remote in REPOS.items():
    lines=command(['git','ls-remote',remote,'refs/heads/main']).splitlines()
    if len(lines)!=1:raise RuntimeError(name+': main missing or ambiguous')
    heads[name]=lines[0].split()[0]
   key=fingerprint(heads);state['last_poll']=time.time()
   if not args.force and heads==state.get('last_success'):
    state['status']='idle';write_json(statefile,state);return
   if not args.force and heads!=state.get('candidate'):
    state.update(candidate=heads,first_seen=time.time(),status='debouncing');write_json(statefile,state);print('New main revisions:',heads,flush=True);return
   if not args.force and time.time()-state['first_seen']<45:write_json(statefile,state);return
   if not args.force and state.get('failed_key')==key and time.time()<state.get('retry_after',0):write_json(statefile,state);return
   run=ROOT/'runs'/(time.strftime('%Y%m%d-%H%M%S')+'-'+key);run.mkdir(parents=True);write_json(run/'commits.json',heads)
   state.update(status='running',candidate=heads,running=heads,run_dir=str(run));write_json(statefile,state)
   with (run/'build.log').open('a',buffering=1) as log:
    paths={name:str(checkout(name,sha)) for name,sha in heads.items()}
    entry=Path(paths['online.mirror'])/'cocolian/office-lab/scripts/devops/release.py'
    env=dict(os.environ,DEVOPS_RUN=str(run),DEVOPS_CHECKOUTS=json.dumps(paths),DEVOPS_COMMITS=json.dumps(heads),DEVOPS_PREVIOUS=json.dumps(state.get('last_success',{})))
    subprocess.run(['python3','-u',str(entry)],env=env,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=21600)
   state.update(last_success=heads,status='success',completed_at=time.time())
   for field in ('error','failed_key','retry_after','running'):state.pop(field,None)
   write_json(statefile,state)
   installed=Path.home()/'.local/lib/cocolian-devops/watch.py';installed.write_text((entry.parent/'watch.py').read_text())
   print('Release verified:',heads,'log:',run/'build.log',flush=True)
  except Exception as error:
   state.update(status='failed',error=str(error),failed_at=time.time(),failed_key=locals().get('key'),retry_after=time.time()+1800);state.pop('running',None);write_json(statefile,state);traceback.print_exc();raise
if __name__=='__main__':main()

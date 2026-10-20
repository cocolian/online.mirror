#!/usr/bin/env python3
"""Outbound Git polling with stable-head debounce and isolated builds."""
import fcntl,json,os,subprocess,time
from pathlib import Path
HOME=Path.home();ROOT=HOME/'.local/share/cocolian-devops';ROOT.mkdir(parents=True,exist_ok=True)
lock=(ROOT/'lock').open('w')
try:fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
except BlockingIOError:raise SystemExit(0)
def run(args,**kw):return subprocess.check_output(args,text=True,timeout=kw.pop('timeout',120),**kw).strip()
repo='git@github.com:cocolian/online.mirror.git';branch='main';statefile=ROOT/'state.json'
s=json.loads(statefile.read_text()) if statefile.exists() else {}
def save():
 p=statefile.with_suffix('.tmp');p.write_text(json.dumps(s,indent=2)+'\n');p.replace(statefile)
try:
 head=run(['git','ls-remote',repo,'refs/heads/'+branch]).split()[0]
 if not s.get('baseline'):
  s.update(baseline=head,last_success=head,status='initialized');save();print('Baseline recorded; no deployment:',head);raise SystemExit(0)
 if head==s.get('last_success'):raise SystemExit(0)
 if head!=s.get('candidate'):
  s.update(candidate=head,first_seen=time.time(),status='debouncing');save();raise SystemExit(0)
 if time.time()-s['first_seen']<60:raise SystemExit(0)
 if s.get('failed_commit')==head:raise SystemExit(0)
 checkout=ROOT/'checkout'
 if not (checkout/'.git').exists():run(['git','clone','--no-checkout','--filter=blob:none',repo,str(checkout)],timeout=600)
 run(['git','-C',str(checkout),'fetch','origin',branch],timeout=600)
 run(['git','-C',str(checkout),'checkout','--detach','--force',head])
 # A release must explicitly provide its complete build/test/publish/deploy entrypoint.
 entry=checkout/'cocolian/office-lab/devops/release.sh'
 if not entry.is_file():raise RuntimeError('Release blocked: missing cocolian/office-lab/devops/release.sh; no cluster changes made')
 s.update(status='running',running=head);save()
 env=dict(os.environ,RELEASE_COMMIT=head,RELEASE_NAMESPACE='office-lab',RELEASE_STATE_DIR=str(ROOT),RELEASE_PREVIOUS_COMMIT=s['last_success'])
 subprocess.run(['bash',str(entry)],cwd=checkout,env=env,check=True,timeout=14400)
 s.update(last_success=head,status='success');s.pop('failed_commit',None);save()
 print('Release successful:',head)
except Exception as e:
 s.update(status='failed',error=str(e),failed_commit=s.get('candidate'),failed_at=time.time());save();print(str(e),flush=True);raise

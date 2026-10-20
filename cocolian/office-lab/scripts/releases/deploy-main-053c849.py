"""Guarded office-lab image-only deployment, rollback all touched deployments on failure."""
import json,subprocess
from pathlib import Path
root=Path('/home/codex/projects/cocolian-office');out=root/'artifacts/main-053c849-20261020'
def k(*a):return subprocess.check_output(['kubectl',*a],text=True)
old={x['metadata']['name']:x for x in json.loads((out/'deployments-before.json').read_text())['items']}
targets={'collabora':'collabora-source','collabora-wordmac':'collabora-wordmac','libreoffice':'libreoffice-source'}
images={name:'res.cocolian.cn/cocolian/'+component+'@'+(out/(component+'.digest')).read_text().strip() for name,component in targets.items()}
# All original templates must remain unchanged since preflight.
for name in targets:
 current=json.loads(k('-n','office-lab','get','deployment',name,'-o','json'))
 assert current['spec']['template']==old[name]['spec']['template'], 'Concurrent change: '+name
changed=[]
try:
 for name,image in images.items():
  before=old[name]['spec']['template'];container=before['spec']['containers'][0]['name']
  assert before['spec']['containers'][0]['imagePullPolicy']=='IfNotPresent'
  patch=[{'op':'test','path':'/spec/template','value':before},{'op':'replace','path':'/spec/template/spec/containers/0/image','value':image}]
  k('-n','office-lab','patch','deployment',name,'--type=json','-p',json.dumps(patch));changed.append(name)
  k('-n','office-lab','rollout','status','deployment/'+name,'--timeout=180s')
  print('Ready:',name,image,flush=True)
 # Run HTTP checks from existing portal container, without exposing credentials.
 code='import urllib.request,json; urls=["http://collabora:9980/hosting/discovery","http://collabora-wordmac:9980/word-mac/hosting/discovery","http://libreoffice:3000/health","http://127.0.0.1:8000/health"];print(json.dumps({u:urllib.request.urlopen(u,timeout=15).status for u in urls}))'
 print(k('-n','office-lab','exec','deployment/wopi','-c','wopi','--','python3','-c',code),flush=True)
 subprocess.run(['python3','/tmp/verify-conversion.py'],check=True,timeout=150)
 def stable(items):
  return {(p['metadata']['namespace'],p['metadata']['name']):[p['metadata']['uid'],p['status'].get('phase'),p['status'].get('containerStatuses')] for p in items if not (p['metadata']['namespace']=='office-lab' and any(p['metadata']['name'].startswith(n+'-') for n in targets))}
 before=stable(json.loads((out/'pods-before.json').read_text())['items']);after=stable(json.loads(k('get','pods','-A','-o','json'))['items']);assert before==after,'Protected pods changed'
 (out/'release.json').write_text(json.dumps({'source_commit':'053c8491238','images':images,'protected_pods_unchanged':len(before),'status':'verified'},indent=2)+'\n')
 print('VERIFIED: protected pods unchanged:',len(before),flush=True)
except BaseException:
 for name in reversed(changed):
  current=json.loads(k('-n','office-lab','get','deployment',name,'-o','json'))
  assert current['spec']['template']['spec']['containers'][0]['image']==images[name], 'Concurrent edit during rollback'
  k('-n','office-lab','patch','deployment',name,'--type=json','-p',json.dumps([{'op':'replace','path':'/spec/template','value':old[name]['spec']['template']}]))
  k('-n','office-lab','rollout','status','deployment/'+name,'--timeout=180s')
  print('Rolled back:',name,flush=True)
 raise

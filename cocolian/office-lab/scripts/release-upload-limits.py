import json,subprocess,copy
from pathlib import Path
r=Path('/home/codex/projects/cocolian-office/artifacts/upload-limits-20261017');before=json.loads((r/'before.json').read_text())['items'];changed=[]
def get(kind,name):return json.loads(subprocess.check_output(['kubectl','-n','office-lab','get',kind,name,'-o','json']))
def patch(kind,name,operations,rollback):
 (r/(name+'-'+kind+'-rollback.json')).write_text(json.dumps(rollback,indent=2)+'\n')
 subprocess.run(['kubectl','-n','office-lab','patch',kind,name,'--type=json','-p',json.dumps(operations)],check=True);changed.append((kind,name))
def deployment(name,image):
 old=next(x['spec']['template'] for x in before if x['kind']=='Deployment' and x['metadata']['name']==name);assert get('deployment',name)['spec']['template']==old
 new=copy.deepcopy(old);container=next(x for x in new['spec']['containers'] if x['name']==name);container['image']=image
 if name=='wopi':container['env']=[x for x in container['env'] if x['name']!='KNOWLEDGE_MAX_BYTES']+[{'name':'KNOWLEDGE_MAX_BYTES','value':str(2*1024**3)}]
 patch('deployment',name,[{'op':'test','path':'/spec/template','value':old},{'op':'replace','path':'/spec/template','value':new}],[{'op':'test','path':'/spec/template','value':new},{'op':'replace','path':'/spec/template','value':old}])
 subprocess.run(['kubectl','-n','office-lab','rollout','status','deployment/'+name,'--timeout=180s'],check=True)
try:
 deployment('libreoffice','res.cocolian.cn/cocolian/libreoffice-source@sha256:7d559908865f12aac5f71977c58c6c9c8a06d52a26a9a84db5c7bd05a8faf10c')
 old=next(x['data']['nginx.conf'] for x in before if x['kind']=='ConfigMap' and x['metadata']['name']=='lab-source');new=(r/'office-gateway.conf').read_text();assert get('configmap','lab-source')['data']['nginx.conf']==old
 patch('configmap','lab-source',[{'op':'test','path':'/data/nginx.conf','value':old},{'op':'replace','path':'/data/nginx.conf','value':new}],[{'op':'test','path':'/data/nginx.conf','value':new},{'op':'replace','path':'/data/nginx.conf','value':old}])
 path='/metadata/annotations/nginx.ingress.kubernetes.io~1proxy-body-size';assert get('ingress','office')['metadata']['annotations']['nginx.ingress.kubernetes.io/proxy-body-size']=='20m'
 patch('ingress','office',[{'op':'test','path':path,'value':'20m'},{'op':'replace','path':path,'value':'1g'}],[{'op':'test','path':path,'value':'1g'},{'op':'replace','path':path,'value':'20m'}])
 deployment('wopi','res.cocolian.cn/cocolian/office-portal@sha256:6783d7dd24aa30a3f18fc20d9085b5aad9e71666e04c7a8d71d18fd0f9aa91c4')
 subprocess.run(['kubectl','-n','office-lab','exec','deploy/wopi','-c','gateway','--','nginx','-t','-c','/config/nginx.conf'],check=True)
except Exception:
 for kind,name in reversed(changed):subprocess.run(['kubectl','-n','office-lab','patch',kind,name,'--type=json','--patch-file',str(r/(name+'-'+kind+'-rollback.json'))],check=True)
 raise

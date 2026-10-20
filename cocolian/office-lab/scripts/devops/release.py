#!/usr/bin/env python3
"""Image-only, immutable office-lab release. Run only through the serialized watcher."""
import base64,copy,fcntl,hashlib,json,os,shutil,subprocess,sys,time
from pathlib import Path
HOME=Path.home();CACHE=HOME/'projects/cocolian-office/build/source-19b4efae7fb8'
AUTH=HOME/'.config/cocolian-devops/registry-auth.json'
BUILDER='cocolian-office-builder:debian13'
TARGETS={'collabora':'collabora-source','collabora-wordmac':'collabora-wordmac','libreoffice':'libreoffice-source','wopi':'office-portal'}

def run(args,**kw):
 print('+',' '.join(map(str,args[:8])),flush=True)
 timeout=kw.pop('timeout',600)
 if args[0] in ('docker','skopeo') and args[1] in ('build','run','save','load','copy'):
  subprocess.run(list(map(str,args)),timeout=timeout,check=True,**kw);return ''
 try:return subprocess.check_output(list(map(str,args)),timeout=timeout,**kw).decode().strip()
 except subprocess.CalledProcessError as error:
  print(error.output.decode(errors='replace'),flush=True);raise
def k(*args,**kw):return run(['kubectl',*args],**kw)
def dump(path,obj):path.write_text(json.dumps(obj,indent=2,ensure_ascii=False)+'\n')
def git(repo,*args):return run(['git','-C',repo,*args])
def source_changes(repo,previous,current):
 if not previous:return ['engine/','browser/']
 return git(repo,'diff','--name-only',previous,current).splitlines()
def needs_engine(paths):return any(p.startswith('engine/') for p in paths)
def needs_online(paths):return any(not p.startswith(('engine/','cocolian/','.github/')) and not p.endswith(('.md','.txt','.png','.pdf')) for p in paths)

def sync_source(repo):
 # Preserve generated files and unchanged mtimes; remove only formerly tracked files.
 manifest=CACHE/'devops-source-files.json';source=CACHE/'source'
 names=git(repo,'ls-files','-z').split('\x00');names=[n for n in names if n and not n.startswith(('cocolian/','.github/'))]
 old=json.loads(manifest.read_text()) if manifest.exists() else []
 for name in set(old)-set(names):
  p=source/name
  if p.is_file() or p.is_symlink():p.unlink()
 copied=0
 for name in names:
  src=repo/name;dst=source/name
  if not src.exists() and not src.is_symlink():raise RuntimeError('Missing tracked source: '+name)
  if src.is_dir():continue
  equal=False
  if src.is_symlink():equal=dst.is_symlink() and os.readlink(src)==os.readlink(dst)
  elif dst.is_file() and not dst.is_symlink() and src.stat().st_size==dst.stat().st_size:
   equal=hashlib.file_digest(src.open('rb'),'sha256').digest()==hashlib.file_digest(dst.open('rb'),'sha256').digest()
  if equal:continue
  dst.parent.mkdir(parents=True,exist_ok=True)
  if dst.is_symlink() or (src.is_symlink() and dst.exists()):dst.unlink()
  if src.is_symlink():dst.symlink_to(os.readlink(src))
  else:shutil.copy2(src,dst)
  copied+=1
 dump(manifest,names);print('Tracked source files synchronized:',copied,flush=True)

def compile_source(repo,engine,online):
 # This cache is shared with manual builds: never run two make processes in it.
 processes=run(['ps','-eo','comm,args'])
 if any(line.split(None,1)[0] in ('make','gmake','ninja') for line in processes.splitlines() if line.split()):
  raise RuntimeError('An existing make/ninja process is running; retry after it completes')
 sync_source(repo)
 commands=['set -euo pipefail','export CCACHE_DIR=/workspace/cache/ccache','export CCACHE_MAXSIZE=8G']
 if engine:
  commands += ['cd /workspace/engine','make -j3 build',
   'make -j3 CppunitTest_sw_layoutwriter5 CPPUNIT_TEST_NAME="testWordImportTypography testWordTableGridProportional testTdf144450 testTdf167583"']
 if online:commands += ['cd /workspace/source','make -j3','DESTDIR=/workspace/stage make install']
 run(['docker','run','--rm','--name','cocolian-devops-build','--network','host','--cpus','3','--memory','12g','--user','1000:1000',
      '-v',str(CACHE)+':/workspace','-v',str(HOME/'projects/cocolian-office/artifacts/render-fidelity-20261019/qa-fonts')+':/usr/local/share/fonts/fidelity-test:ro',
      BUILDER,'bash','-lc','\n'.join(commands)],timeout=18000)

def registry_base(image,component,out):
 assert image.startswith('res.cocolian.cn/cocolian/') and '@sha256:' in image
 tag='cocolian/devops-base-'+component+':'+image.split(':')[-1][:16]
 if subprocess.run(['docker','image','inspect',tag],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode:
  archive=out/(component+'-base.tar')
  run(['skopeo','copy','--authfile',AUTH,'docker://'+image,'docker-archive:'+str(archive)+':'+tag],timeout=900)
  run(['docker','load','-i',archive],timeout=900);archive.unlink()
 return tag

def publish(tag,component,out):
 archive=out/(component+'.docker.tar');oci=out/(component+'.oci.tar')
 try:
  run(['docker','save','-o',archive,tag],timeout=900)
  run(['skopeo','copy','docker-archive:'+str(archive),'oci-archive:'+str(oci)],timeout=900)
  destination='docker://res.cocolian.cn/cocolian/'+component+':'+tag.split(':')[-1]
  expected=run(['skopeo','inspect','--format','{{.Digest}}','oci-archive:'+str(oci)])
  run(['skopeo','copy','--preserve-digests','--authfile',AUTH,'oci-archive:'+str(oci),destination],timeout=1200)
  actual=run(['skopeo','inspect','--authfile',AUTH,'--format','{{.Digest}}',destination])
  assert expected==actual,'Published digest mismatch'
  return 'res.cocolian.cn/cocolian/'+component+'@'+actual
 finally:
  archive.unlink(missing_ok=True);oci.unlink(missing_ok=True)

def build_images(paths,heads,old,out,engine,online):
 context=out/'context';context.mkdir();outer=paths['cocolian-office']/'deploy/source-build'
 shutil.copytree(paths['cocolian']/'apps/office-portal',context/'portal',ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
 # The portal is syntax checked and its focused tests run before packaging.
 for source in (context/'portal').glob('*.py'):compile(source.read_text(),str(source),'exec')
 tests=list((paths['cocolian']/'apps/office-portal').glob('test_*.py'))
 for test in tests:run([sys.executable,test],cwd=test.parent)
 for name in ('word-mac-fontconfig.xml','editor-aids.css','editor-aids.xcd','source-conversion-service.py'):
  shutil.copy2(outer/name,context/name)
 if engine:shutil.copytree(CACHE/'engine/instdir',context/'engine',symlinks=True)
 if online:shutil.copytree(CACHE/'stage/usr',context/'usr',symlinks=True)
 tag='auto-'+time.strftime('%Y%m%d%H%M%S')+'-'+heads['online.mirror'][:8]
 images={}
 for deployment,component in TARGETS.items():
  before=old[deployment]['spec']['template']['spec']['containers'][0]['image']
  base=registry_base(before,component,out)
  lines=['ARG BASE','FROM ${BASE}','USER root']
  if deployment=='wopi':lines += ['COPY portal/ /app/','USER 1000:1000']
  else:
   if engine:lines += ['COPY engine/ /opt/collaboraoffice/']
   if deployment=='libreoffice':lines += ['COPY source-conversion-service.py /usr/local/bin/source-conversion-service.py']
   else:
    if online:lines += ['COPY usr/ /usr/']
    lines += ['COPY editor-aids.css /usr/share/coolwsd/browser/dist/editor-aids-20261020.css',
     'COPY editor-aids.xcd /opt/collaboraoffice/share/registry/cocolian-editor-aids.xcd',
     '''RUN if ! grep -q 'editor-aids-20261020.css' /usr/share/coolwsd/browser/dist/cool.html; then sed -i 's|</head>|<link rel="stylesheet" href="%SERVICE_ROOT%/browser/%VERSION%/editor-aids-20261020.css"></head>|' /usr/share/coolwsd/browser/dist/cool.html; fi''']
    if deployment=='collabora-wordmac':lines += ['COPY word-mac-fontconfig.xml /etc/fonts/word-mac-fontconfig.xml']
    lines += ['RUN fc-cache -f && coolwsd-systemplate-setup /opt/cool/systemplate /opt/collaboraoffice && chown -R 1001:1001 /opt/cool/systemplate']
   lines += ['USER 1001:1001']
  for name,sha in heads.items():lines += [f'LABEL cn.cocolian.source.{name}="{sha}"']
  recipe=context/(component+'.Dockerfile');recipe.write_text('\n'.join(lines)+'\n')
  local='cocolian/'+component+':'+tag
  run(['docker','build','--network','none','--build-arg','BASE='+base,'-f',recipe,'-t',local,context],timeout=1800)
  images[deployment]=publish(local,component,out)
 return images

def stable_pods(items):
 return {(p['metadata']['namespace'],p['metadata']['name']):(p['metadata']['uid'],[(c['name'],c.get('imageID'),c['ready'],c['restartCount']) for c in p['status'].get('containerStatuses',[])]) for p in items if p['metadata']['namespace']!='office-lab'}

def candidate(old,image,name):
 # No app labels: candidate cannot enter any production Service selector.
 podname='devops-candidate-'+name
 spec=copy.deepcopy(old['spec']['template']['spec'])
 if name=='wopi':spec['containers']=spec['containers'][:1];spec['volumes']=[v for v in spec['volumes'] if v['name']=='data']
 c=spec['containers'][0];c['image']=image;c['imagePullPolicy']='IfNotPresent'
 c['resources']={'requests':{'cpu':'100m','memory':'128Mi','ephemeral-storage':'128Mi'},'limits':{'cpu':'1','memory':'1Gi','ephemeral-storage':'512Mi'}}
 spec['restartPolicy']='Never';spec['terminationGracePeriodSeconds']=5
 pod={'apiVersion':'v1','kind':'Pod','metadata':{'name':podname,'namespace':'office-lab','labels':{'devops.cocolian.cn/candidate':'true'}},'spec':spec}
 created=False
 try:
  k('-n','office-lab','create','-f','-',input=json.dumps(pod).encode());created=True
  k('-n','office-lab','wait','--for=condition=Ready','pod/'+podname,'--timeout=240s',timeout=260)
  if name=='collabora-wordmac':
   font=k('-n','office-lab','exec',podname,'--','fc-match','FangSong_GB2312')
   assert 'FangSong_GB2312' in font and 'YaHei' not in font,'Genuine FangSong font not selected'
  ip=json.loads(k('-n','office-lab','get','pod',podname,'-o','json'))['status']['podIP']
  script=Path(__file__).with_name('verify.py').read_bytes()
  k('-n','office-lab','exec','-i','deployment/wopi','-c','wopi','--','python3','-','--candidate',name,ip,input=script,timeout=120)
 finally:
  # Delete only the uniquely named disposable candidate created by this pipeline.
  if created:k('-n','office-lab','delete','pod',podname,'--ignore-not-found','--wait=true','--timeout=60s',timeout=75)

def patch_image(name,before,image):
 return [{'op':'test','path':'/spec/template','value':before},
         {'op':'replace','path':'/spec/template/spec/containers/0/image','value':image},
         {'op':'replace','path':'/spec/template/spec/containers/0/imagePullPolicy','value':'IfNotPresent'}]

def deploy(old,images,out,verify):
 changed=[]
 try:
  # Every image must have pulled and started on the cluster node before touching Deployments.
  for name,image in images.items():candidate(old[name],image,name)
  for name,image in images.items():
   k('-n','office-lab','patch','deployment',name,'--type=json','-p',json.dumps(patch_image(name,old[name]['spec']['template'],image)))
   changed.append(name);k('-n','office-lab','rollout','status','deployment/'+name,'--timeout=240s',timeout=260)
  verify()
 except BaseException:
  failures=[]
  for name in reversed(changed):
   try:
    current=json.loads(k('-n','office-lab','get','deployment',name,'-o','json'))['spec']['template']
    expected=copy.deepcopy(old[name]['spec']['template']);expected['spec']['containers'][0]['image']=images[name];expected['spec']['containers'][0]['imagePullPolicy']='IfNotPresent'
    if current!=expected:raise RuntimeError('Concurrent template modification; refusing to overwrite '+name)
    patch=[{'op':'test','path':'/spec/template','value':expected},{'op':'replace','path':'/spec/template','value':old[name]['spec']['template']}]
    k('-n','office-lab','patch','deployment',name,'--type=json','-p',json.dumps(patch))
    k('-n','office-lab','rollout','status','deployment/'+name,'--timeout=240s',timeout=260)
   except Exception as error:failures.append(str(error))
  dump(out/'rollback.json',{'attempted':changed,'errors':failures})
  if failures:print('ROLLBACK REQUIRES ATTENTION:',failures,flush=True)
  raise

def main():
 assert AUTH.is_file() and AUTH.stat().st_mode & 0o077==0,'Registry auth missing or insecure permissions'
 out=Path(os.environ['DEVOPS_RUN']);paths={n:Path(p) for n,p in json.loads(os.environ['DEVOPS_CHECKOUTS']).items()};heads=json.loads(os.environ['DEVOPS_COMMITS'])
 # Reject unexpected namespace overrides; this pipeline never applies arbitrary manifests.
 assert os.environ.get('RELEASE_NAMESPACE','office-lab')=='office-lab'
 if shutil.disk_usage(HOME).free<15*1024**3:raise RuntimeError('Less than 15 GiB available; no deployment attempted')
 old={d['metadata']['name']:d for d in json.loads(k('-n','office-lab','get','deployments','-o','json'))['items']}
 assert set(TARGETS)<=set(old)
 dump(out/'deployments-before.json',{'items':list(old.values())})
 before=json.loads(k('get','pods','-A','-o','json'));dump(out/'pods-before.json',before)
 config=json.loads((HOME/'.config/cocolian-devops/config.json').read_text())
 built=CACHE/'devops-built.json';last=json.loads(built.read_text()) if built.exists() else {'online.mirror':config['engine_baseline']}
 previous=json.loads(os.environ.get('DEVOPS_PREVIOUS','{}'))
 changes=source_changes(paths['online.mirror'],previous.get('online.mirror',config['engine_baseline']),heads['online.mirror'])
 engine=needs_engine(changes);online=needs_online(changes)
 cache_changes=source_changes(paths['online.mirror'],last['online.mirror'],heads['online.mirror'])
 with (CACHE/'devops-build.lock').open('a') as lock:
  fcntl.flock(lock,fcntl.LOCK_EX)
  if needs_engine(cache_changes) or needs_online(cache_changes):compile_source(paths['online.mirror'],needs_engine(cache_changes),needs_online(cache_changes))
  dump(built,{'online.mirror':heads['online.mirror']})
  images=build_images(paths,heads,old,out,engine,online)
 dump(out/'images.json',images)
 def verify():
  script=Path(__file__).with_name('verify.py').read_bytes()
  result=k('-n','office-lab','exec','-i','deployment/wopi','-c','wopi','--','python3','-',input=script,timeout=240)
  dump(out/'checks.json',json.loads(result))
  after=json.loads(k('get','pods','-A','-o','json'));dump(out/'pods-after.json',after)
  assert stable_pods(before['items'])==stable_pods(after['items']),'Other namespace pods changed during release'
 deploy(old,images,out,verify)
 dump(out/'release.json',{'status':'verified','commits':heads,'images':images,'engine_recompiled':engine,'online_recompiled':online,'protected_pods':len(stable_pods(before['items']))})
 print('RELEASE VERIFIED',images,flush=True)
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Image-only, immutable office-lab release. Run only through the serialized watcher."""
import base64,copy,fcntl,hashlib,json,os,shutil,subprocess,sys,tempfile,time
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
def needs_online(paths):return any(not p.startswith(('engine/','cocolian/','.github/')) and not p.endswith('.md') for p in paths)

def sync_source(repo):
 # Preserve generated files and unchanged mtimes; remove only formerly tracked files.
 manifest=CACHE/'devops-source-files.json';source=CACHE/'source'
 names=git(repo,'ls-files','-z').split('\x00');names=[n for n in names if n and not n.startswith(('cocolian/','.github/'))]
 if manifest.exists():old=json.loads(manifest.read_text())
 else:
  baseline=json.loads((CACHE/'source-lock.json').read_text())['commit']
  old=git(repo,'ls-tree','-r','--name-only',baseline).splitlines()
  old=[n for n in old if not n.startswith(('cocolian/','.github/'))]
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
   with src.open('rb') as source_stream,dst.open('rb') as target_stream:
    equal=hashlib.file_digest(source_stream,'sha256').digest()==hashlib.file_digest(target_stream,'sha256').digest()
  if equal:
   if not src.is_symlink():os.chmod(dst,src.stat().st_mode & 0o777)
   continue
  dst.parent.mkdir(parents=True,exist_ok=True)
  if dst.is_symlink() or (src.is_symlink() and dst.exists()):dst.unlink()
  if src.is_symlink():dst.symlink_to(os.readlink(src))
  else:
   shutil.copy2(src,dst)
   # Checkout timestamps may predate cached objects. Changed inputs must rebuild.
   os.utime(dst,None)
  copied+=1
 dump(manifest,names);print('Tracked source files synchronized:',copied,flush=True)

def compile_source(repo,engine,online,office_config):
 # This cache is shared with manual builds: never run two make processes in it.
 processes=run(['ps','-eo','comm,args'])
 if any(line.split(None,1)[0] in ('make','gmake','ninja') for line in processes.splitlines() if line.split()):
  raise RuntimeError('An existing make/ninja process is running; retry after it completes')
 sync_source(repo)
 commands=['set -euo pipefail','export CCACHE_DIR=/workspace/cache/ccache','export CCACHE_MAXSIZE=8G']
 if engine:
  commands += ['cd /workspace/engine','make -j3 build',
   'make -j3 CppunitTest_sw_layoutwriter5 CPPUNIT_TEST_NAME="testWordMacSmallBodyGridProportional testWordMacAutoFitPageWidth testWordMacUrlBreakUnits testWordMacRowCellSpacing testWordMacWordWrap testWordMacInlineImageGridLeading testWordMacFirstCharacterIndentFont testWordMacGridOverflowSetting testWordMacLegacyPercentTablePadding testWordMacNumberingStyleIndent testWordMacAutoFitLongUrl testWordMacAutoFitAsciiToken testWordMacAutoFitDefaultFont testWordMacAutoFitNoWrap testWordMacCellNoWrap testWordMacFixedLayoutNoAutoFit testWordMacScriptSpacingSettings testWordMacFixedGridBaseline testWordMacFractionalTableWidth testWordMacMissingNumberingLevelIndex testWordMacContinuousFirstSectionGrid testWordMacLegacyTableBorders testWordMacPunctuationCompression testWordMacEmptyParagraphStyle testWordMacSmallTableGridProportional testWordManualPageBreakOverridesParagraphDefault testWordImportTypography testWordTableGridProportional testWordBodyGridProportional testWordMacMissingParagraphDefaults testTdf144450 testTdf167583"',
   'cp workdir/CppunitTest/sw_layoutwriter5.test.log /workspace/devops-layout-tests.log']
 if online:commands += ['cd /workspace/source','make -j3','DESTDIR=/workspace/stage make install']
 run(['docker','run','--rm','--name','cocolian-devops-build','--network','host','--cpus','3','--memory','12g','--user','1000:1000',
      '-v',str(CACHE)+':/workspace','-v',str(HOME/'projects/cocolian-office/artifacts/render-fidelity-20261019/qa-fonts')+':/usr/local/share/fonts/fidelity-test:ro',
      BUILDER,'bash','-lc','\n'.join(commands)],timeout=18000)

 if engine:
  # Only this real-font test allows fonts outside the bundled QA set. The
  # private font hashes are also gated before image creation and deployment.
  run(['docker','run','--rm','--name','cocolian-devops-gb-font-test','--network','none',
       '--cpus','3','--memory','12g','--user','1000:1000','-v',str(CACHE)+':/workspace',
       '-v',str(HOME/'.local/share/cocolian-devops/fonts/office')+':/usr/local/share/fonts/word-mac:ro',
       '-v',str(office_config/'deploy/source-build/word-mac-fontconfig.xml')+':/etc/fonts/word-mac-fontconfig.xml:ro',
       '-e','FONTCONFIG_FILE=/etc/fonts/word-mac-fontconfig.xml','-e','NON_APPLICATION_FONT_USE=allow',
       '-e','CCACHE_DIR=/workspace/cache/ccache',BUILDER,'bash','-lc',
       'cd /workspace/engine && make -j3 CppunitTest_sw_layoutwriter5 NON_APPLICATION_FONT_USE=allow CPPUNIT_TEST_NAME="testWordMacCellOpeningPunctuationBudget testWordMacClosingUnitPunctuationRun testWordMacInlineImageExactGrid testWordMacCoveredRowMinimum testWordMacBodyPairedDelimiters testWordMacFixedPercentColumns testWordMacLegacyPairedParentheses testWordMacTableHangingPrefix testWordMacMinimumRowFragments testWordMacClosingUnitTrailingCjk testWordMacHangingPunctuationPaint testWordMacTableNoScriptGapCompression testWordMacOversizedFixedTableWidth testWordMacTableWidowControl testWordMacCjkHangingParenthesis testWordMacNaturalLinePitch testWordMacAutoFitMissingColumnPreferences testWordMacCjkHangingColon testWordMacEmptyRowContinuation testWordMacAutoFitCjkBreakUnits testWordMacCachedGridBottom testWordMacPdfFallbackBold testWordMacStrictGridTablePunctuation testWordMacCenteredGridHeading testWordMacGridFloatingTableTitle testWordMacTableHalfHanging testWordMacAutomaticSuperscript testWordMacGbBoldFractionalFit testWordMacResolvedFontGrid testWordMacClosingUnitBudget testWordMacPercentageClosingUnit testWordMacLeftAlignedLineBudget testWordMacClassicBoldAdvance testWordMacCjkBaseline testWordMacMultiplePunctuationBudget testWordMacMixedScriptLineBudget testWordMacFlexibleScriptGap testWordMacGbBoldTableWrap testWordMacGridBottomPadding testWordMacDengXianPitch testWordMacMissingThemeFonts testWordMacPunctuationWrap testWordMacAutomaticScriptSpacing testWordMacGridPunctuationCompression testWordMacYaHeiGridHeight testWordMacWesternPunctuationHint" '
       '&& cp workdir/CppunitTest/sw_layoutwriter5.test.log /workspace/devops-gb-font-tests.log'],timeout=1200)

def registry_transport():
 config=json.loads((HOME/'.config/cocolian-devops/config.json').read_text())
 endpoint=config.get('registry_endpoint','res.cocolian.cn')
 assert endpoint in ('res.cocolian.cn','127.0.0.1:15000'),'Unexpected registry destination'
 # TLS is bypassed only inside the authenticated SSH loopback tunnel.
 return endpoint,(['--tls-verify=false'] if endpoint=='127.0.0.1:15000' else []),(['--src-tls-verify=false'] if endpoint=='127.0.0.1:15000' else []),(['--dest-tls-verify=false'] if endpoint=='127.0.0.1:15000' else [])

def reuse_local_base(image,component,tag):
 endpoint,inspect_tls,_,_=registry_transport()
 remote=json.loads(run(['skopeo','inspect','--config','--authfile',AUTH,*inspect_tls,
  'docker://'+image.replace('res.cocolian.cn',endpoint,1)]))
 for local in run(['docker','image','ls','cocolian/'+component,'--format','{{.Repository}}:{{.Tag}}']).splitlines():
  if local.endswith(':<none>'):continue
  candidate=json.loads(run(['docker','image','inspect',local]))[0]
  # A registry/archive import may change the manifest while preserving the
  # config and every uncompressed layer. Verify both before reusing it.
  if (remote.get('rootfs',{}).get('diff_ids')==candidate.get('RootFS',{}).get('Layers')
   and remote.get('config')==candidate.get('Config')
   and remote.get('os')==candidate.get('Os')
   and remote.get('architecture')==candidate.get('Architecture')):
   run(['docker','tag',local,tag]);return True
 return False

def registry_base(image,component,out):
 assert image.startswith('res.cocolian.cn/cocolian/') and '@sha256:' in image
 tag='cocolian/devops-base-'+component+':'+image.split(':')[-1][:16]
 if subprocess.run(['docker','image','inspect',tag],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode:
  if reuse_local_base(image,component,tag):return tag
  archive=out/(component+'-base.tar')
  endpoint,_,src,_=registry_transport()
  try:
   run(['skopeo','copy','--authfile',AUTH,*src,'docker://'+image.replace('res.cocolian.cn',endpoint,1),'docker-archive:'+str(archive)+':'+tag],timeout=900)
   run(['docker','load','-i',archive],timeout=900)
  finally:archive.unlink(missing_ok=True)
 return tag

def publish(tag,component,out):
 # Keep one Docker archive and avoid a second complete OCI archive. Copy
 # straight to the registry, then verify its digest
 # and the config/uncompressed-layer identities against the source image.
 source=json.loads(run(['docker','image','inspect',tag]))[0]
 endpoint,inspect_tls,_,dest_tls=registry_transport()
 destination='docker://'+endpoint+'/cocolian/'+component+':'+tag.split(':')[-1]
 with tempfile.TemporaryDirectory(prefix='publish-',dir=out) as directory:
  digestfile=Path(directory)/'digest'
  archive=Path(directory)/(component+'.docker.tar')
  run(['docker','save','-o',archive,tag],timeout=900)
  run(['skopeo','copy','--authfile',AUTH,*dest_tls,'--format','oci',
   '--digestfile',digestfile,'docker-archive:'+str(archive),destination],timeout=1200)
  expected=digestfile.read_text().strip()
 actual=run(['skopeo','inspect','--authfile',AUTH,*inspect_tls,'--format','{{.Digest}}',destination])
 assert expected==actual,'Published digest mismatch'
 remote=json.loads(run(['skopeo','inspect','--config','--authfile',AUTH,*inspect_tls,destination]))
 assert (remote['rootfs']['diff_ids']==source['RootFS']['Layers']
  and remote['config']==source['Config'] and remote['os']==source['Os']
  and remote['architecture']==source['Architecture']),'Published image configuration mismatch'
 current=json.loads(run(['docker','image','inspect',tag]))[0]
 assert current['Id']==source['Id'],'Source image changed during publishing'
 return 'res.cocolian.cn/cocolian/'+component+'@'+actual

def select_targets(heads,previous):
 selected=set()
 if heads.get('cocolian')!=previous.get('cocolian'):selected.add('wopi')
 if any(heads.get(n)!=previous.get(n) for n in ('cocolian-office','online.mirror')):
  selected.update(('collabora','collabora-wordmac','libreoffice'))
 # A manual --force run with identical heads rebuilds all modules.
 return {n:c for n,c in TARGETS.items() if n in (selected or set(TARGETS))}

def stage_office_fonts(destination,source=None,manifest=None):
 # Font binaries stay in private server storage; Git pins only their hashes.
 source=source or HOME/'.local/share/cocolian-devops/fonts/office'
 manifest=manifest or json.loads(Path(__file__).with_name('office-fonts-manifest.json').read_text())
 destination.mkdir()
 for item in manifest['fonts']:
  name=item['file']
  if Path(name).name!=name:raise RuntimeError('Invalid font filename')
  font=source/name
  if not font.is_file():raise RuntimeError('Private Office font missing: '+name)
  with font.open('rb') as stream:actual=hashlib.file_digest(stream,'sha256').hexdigest()
  if actual!=item['sha256']:raise RuntimeError('Private Office font hash mismatch: '+name)
  shutil.copy2(font,destination/name)
 return len(manifest['fonts'])

def stage_runtime_tree(source,destination):
 # Set COPY input permissions before layering, avoiding a recursive chmod
 # that copies the complete installed engine into another image layer.
 shutil.copytree(source,destination,symlinks=True)
 for path in [destination,*destination.rglob('*')]:
  if path.is_symlink():continue
  mode=path.stat().st_mode & 0o7777
  if path.is_dir() or mode & 0o111:mode |= 0o111
  path.chmod(mode | 0o444)

def build_images(paths,heads,old,out,engine,online,targets):
 context=out/'context';context.mkdir();outer=paths['cocolian-office']/'deploy/source-build'
 if 'wopi' in targets:
  shutil.copytree(paths['cocolian']/'apps/office-portal',context/'portal',ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
  for source in (context/'portal').glob('*.py'):compile(source.read_text(),str(source),'exec')
  for test in (paths['cocolian']/'apps/office-portal').glob('test_*.py'):run([sys.executable,test],cwd=test.parent)
 if set(targets)-{'wopi'}:
  stage_office_fonts(context/'office-fonts')
  for name in ('word-mac-fontconfig.xml','editor-aids.css','editor-aids.xcd','source-conversion-service.py'):
   shutil.copy2(outer/name,context/name)
 if engine:stage_runtime_tree(CACHE/'engine/instdir',context/'engine')
 if online:stage_runtime_tree(CACHE/'stage/usr',context/'usr')
 tag='auto-'+time.strftime('%Y%m%d%H%M%S')+'-'+heads['online.mirror'][:8]
 images={}
 for deployment,component in targets.items():
  before=old[deployment]['spec']['template']['spec']['containers'][0]['image']
  base=registry_base(before,component,out)
  lines=['ARG BASE','FROM ${BASE}','USER root']
  if deployment=='wopi':lines += ['COPY portal/ /app/','RUN chmod -R a+rX /app','USER 1000:1000']
  else:
   lines += ['COPY --chmod=644 office-fonts/ /usr/local/share/fonts/word-mac/',
    'COPY --chmod=644 word-mac-fontconfig.xml /etc/fonts/word-mac-fontconfig.xml',
    'ENV FONTCONFIG_FILE=/etc/fonts/word-mac-fontconfig.xml COCOLIAN_WORD_MAC_METRICS=1 COCOLIAN_WORD_MAC_CLASSIC_METRICS=1 COCOLIAN_WORD_MAC_AUTOFIT=1 COCOLIAN_WORD_MAC_GRID_BOTTOM=1 COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS=1 COCOLIAN_WORD_MAC_LINE_BUDGET=1',
    'RUN chmod -R a+rX /usr/local/share/fonts/word-mac && fc-cache -f']
   if engine:lines += ['COPY engine/ /opt/collaboraoffice/']
   if deployment=='libreoffice':lines += ['COPY --chmod=755 source-conversion-service.py /usr/local/bin/source-conversion-service.py']
   else:
    if online:lines += ['COPY usr/ /usr/']
    lines += ['COPY --chmod=644 editor-aids.css /usr/share/coolwsd/browser/dist/editor-aids-20261020.css',
     'COPY --chmod=644 editor-aids.xcd /opt/collaboraoffice/share/registry/cocolian-editor-aids.xcd',
     '''RUN if ! grep -q 'editor-aids-20261020.css' /usr/share/coolwsd/browser/dist/cool.html; then sed -i 's|</head>|<link rel="stylesheet" href="%SERVICE_ROOT%/browser/%VERSION%/editor-aids-20261020.css"></head>|' /usr/share/coolwsd/browser/dist/cool.html; fi''']
    lines += ['RUN fc-cache -f && coolwsd-systemplate-setup /opt/cool/systemplate /opt/collaboraoffice && chown -R 1001:1001 /opt/cool/systemplate']
   lines += ['USER 1001:1001']
  revision=heads['cocolian'] if deployment=='wopi' else heads['online.mirror']
  lines += [f'LABEL org.opencontainers.image.revision="{revision}"']
  for name,sha in heads.items():lines += [f'LABEL cn.cocolian.source.{name}="{sha}"']
  recipe=context/(component+'.Dockerfile');recipe.write_text('\n'.join(lines)+'\n')
  local='cocolian/'+component+':'+tag
  run(['docker','build','--network','none','--build-arg','BASE='+base,'-f',recipe,'-t',local,context],timeout=1800)
  images[deployment]=publish(local,component,out)
 return images

def stable_pods(items,selected=None):
 return {(p['metadata']['namespace'],p['metadata']['name']):(p['metadata']['uid'],[(c['name'],c.get('imageID'),c['ready'],c['restartCount']) for c in p['status'].get('containerStatuses',[])]) for p in items if p['metadata']['namespace']!='office-lab' or (selected is not None and p['metadata'].get('labels',{}).get('app') in set(TARGETS)-set(selected))}

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
  if name!='wopi':
   for family in ('FangSong_GB2312','KaiTi_GB2312','SimSun','Microsoft YaHei','FangSong','STZhongsong','Times New Roman','Tahoma','Courier New'):
    matched=k('-n','office-lab','exec',podname,'--','fc-match','-f','%{family[0]}',family)
    assert matched==family,'Unexpected font substitution: '+family+' -> '+matched
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
 for test in ('test_release.py','test_registry_proxy.py'):run([sys.executable,Path(__file__).with_name(test)])
 out=Path(os.environ['DEVOPS_RUN']);paths={n:Path(p) for n,p in json.loads(os.environ['DEVOPS_CHECKOUTS']).items()};heads=json.loads(os.environ['DEVOPS_COMMITS'])
 # Reject unexpected namespace overrides; this pipeline never applies arbitrary manifests.
 assert os.environ.get('RELEASE_NAMESPACE','office-lab')=='office-lab'
 if shutil.disk_usage(HOME).free<15*1024**3:raise RuntimeError('Less than 15 GiB available; no deployment attempted')
 old={d['metadata']['name']:d for d in json.loads(k('-n','office-lab','get','deployments','-o','json'))['items']}
 assert set(TARGETS)<=set(old)
 dump(out/'deployments-before.json',{'items':list(old.values())})
 before=json.loads(k('get','pods','-A','-o','json'));dump(out/'pods-before.json',before)
 config=json.loads((HOME/'.config/cocolian-devops/config.json').read_text())
 built=CACHE/'devops-built.json'
 previous=json.loads(os.environ.get('DEVOPS_PREVIOUS','{}'))
 targets=select_targets(heads,previous)
 dump(out/'plan.json',{'changed_repositories':[n for n in heads if heads[n]!=previous.get(n)],'deployments':list(targets),'unchanged_deployments':sorted(set(TARGETS)-set(targets))})
 changes=source_changes(paths['online.mirror'],previous.get('online.mirror',config['engine_baseline']),heads['online.mirror'])
 engine=needs_engine(changes);online=needs_online(changes)
 with (CACHE/'devops-build.lock').open('a') as lock:
  fcntl.flock(lock,fcntl.LOCK_EX)
  # A private build can invalidate provenance while this release waits for
  # the lock. Read it only after acquiring exclusive access to the cache.
  last=json.loads(built.read_text()) if built.exists() else {'online.mirror':config['engine_baseline']}
  cache_changes=source_changes(paths['online.mirror'],last['online.mirror'],heads['online.mirror'])
  if set(targets)-{'wopi'} and (needs_engine(cache_changes) or needs_online(cache_changes)):compile_source(paths['online.mirror'],needs_engine(cache_changes),needs_online(cache_changes),paths['cocolian-office'])
  if set(targets)-{'wopi'}:dump(built,{'online.mirror':heads['online.mirror']})
  images=build_images(paths,heads,old,out,engine,online,targets)
 dump(out/'images.json',images)
 def verify():
  script=Path(__file__).with_name('verify.py').read_bytes()
  result=k('-n','office-lab','exec','-i','deployment/wopi','-c','wopi','--','python3','-',input=script,timeout=240)
  dump(out/'checks.json',json.loads(result))
  after=json.loads(k('get','pods','-A','-o','json'));dump(out/'pods-after.json',after)
  assert stable_pods(before['items'],targets)==stable_pods(after['items'],targets),'Unselected modules or other namespace pods changed during release'
  current={d['metadata']['name']:d for d in json.loads(k('-n','office-lab','get','deployments','-o','json'))['items']}
  for name in set(TARGETS)-set(targets):
   assert current[name]['spec']['template']==old[name]['spec']['template'],'Unselected deployment template changed: '+name
 deploy(old,images,out,verify)
 dump(out/'release.json',{'status':'verified','commits':heads,'images':images,'engine_recompiled':engine,'online_recompiled':online,'deployed_modules':list(targets),'unchanged_modules':sorted(set(TARGETS)-set(targets)),'protected_pods':len(stable_pods(before['items'],targets))})
 print('RELEASE VERIFIED',images,flush=True)
if __name__=='__main__':main()

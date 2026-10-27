import copy,importlib.util,json,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
spec=importlib.util.spec_from_file_location('release',Path(__file__).with_name('release.py'));r=importlib.util.module_from_spec(spec);spec.loader.exec_module(r)
class ReleaseTests(unittest.TestCase):
 def test_runtime_copy_permissions_and_symlink_isolation(self):
  with tempfile.TemporaryDirectory() as directory:
   root=Path(directory);source=root/'source';source.mkdir(mode=0o700)
   (source/'data').write_text('resource');(source/'data').chmod(0o600)
   (source/'program').write_text('executable');(source/'program').chmod(0o700)
   outside=root/'private-source';outside.write_text('private');outside.chmod(0o600)
   (source/'link').symlink_to(outside)
   outside_dir=root/'private-directory';outside_dir.mkdir(mode=0o700)
   (outside_dir/'secret').write_text('private');(outside_dir/'secret').chmod(0o600)
   (source/'directory-link').symlink_to(outside_dir,target_is_directory=True)
   destination=root/'context'
   r.stage_runtime_tree(source,destination)
   self.assertEqual(destination.stat().st_mode & 0o777,0o755)
   self.assertEqual((destination/'data').stat().st_mode & 0o777,0o644)
   self.assertEqual((destination/'program').stat().st_mode & 0o777,0o755)
   self.assertTrue((destination/'link').is_symlink())
   self.assertEqual(outside.stat().st_mode & 0o777,0o600)
   self.assertTrue((destination/'directory-link').is_symlink())
   self.assertEqual(outside_dir.stat().st_mode & 0o777,0o700)
   self.assertEqual((outside_dir/'secret').stat().st_mode & 0o777,0o600)
   self.assertEqual((source/'data').stat().st_mode & 0o777,0o600)
   self.assertEqual((source/'program').stat().st_mode & 0o777,0o700)
   self.assertEqual((destination/'data').read_bytes(),(source/'data').read_bytes())

 def test_independent_main_changes(self):
  baseline={n:'old' for n in ('cocolian','cocolian-office','online.mirror')}
  self.assertEqual(set(r.select_targets(dict(baseline,cocolian='new'),baseline)),{'wopi'})
  for name in ('cocolian-office','online.mirror'):
   heads=dict(baseline);heads[name]='new'
   self.assertEqual(set(r.select_targets(heads,baseline)),{'collabora','collabora-wordmac','libreoffice'})
  self.assertEqual(set(r.select_targets({n:'new' for n in baseline},baseline)),set(r.TARGETS))
 def test_change_detection(self):
  self.assertTrue(r.needs_engine(['engine/sw/source/core/text/itrform2.cxx']))
  self.assertFalse(r.needs_engine(['cocolian/office-lab/docs/release.md']))
  self.assertTrue(r.needs_online(['browser/src/control/Control.Toolbar.js']))
  self.assertTrue(r.needs_online(['browser/images/arrow.png']))
  self.assertFalse(r.needs_online(['engine/sw/a.cxx','cocolian/office-lab/scripts/devops/watch.py']))
 def test_old_checkout_timestamp_rebuilds_changed_source(self):
  import os,time
  with tempfile.TemporaryDirectory() as directory:
   root=Path(directory);repo=root/'repo';cache=root/'cache'
   repo.mkdir();(cache/'source').mkdir(parents=True)
   (repo/'changed.cxx').write_text('new source')
   os.utime(repo/'changed.cxx',(1,1))
   (cache/'source/changed.cxx').write_text('old source')
   (cache/'devops-source-files.json').write_text('["changed.cxx"]')
   object_time=time.time()-60
   os.utime(cache/'source/changed.cxx',(object_time,object_time))
   with patch.object(r,'CACHE',cache),patch.object(r,'git',return_value='changed.cxx\x00'):
    r.sync_source(repo)
   self.assertEqual((cache/'source/changed.cxx').read_text(),'new source')
   self.assertGreater((cache/'source/changed.cxx').stat().st_mtime,object_time)

 def test_cache_invalidation_while_waiting_for_lock(self):
  import os
  from types import SimpleNamespace
  with tempfile.TemporaryDirectory() as directory:
   root=Path(directory);cache=root/'cache';cache.mkdir();out=root/'run';out.mkdir()
   config=root/'.config/cocolian-devops';config.mkdir(parents=True)
   (config/'config.json').write_text(json.dumps({'engine_baseline':'base'}))
   auth=root/'auth.json';auth.write_text('{}');auth.chmod(0o600)
   built=cache/'devops-built.json';built.write_text(json.dumps({'online.mirror':'current'}))
   heads={name:'current' for name in ('cocolian','cocolian-office','online.mirror')}
   paths={name:str(root/name) for name in heads}
   env={'DEVOPS_RUN':str(out),'DEVOPS_CHECKOUTS':json.dumps(paths),
        'DEVOPS_COMMITS':json.dumps(heads),'DEVOPS_PREVIOUS':json.dumps(heads),
        'RELEASE_NAMESPACE':'office-lab'}
   deployments={'items':[{'metadata':{'name':name},'spec':{'template':{}}} for name in r.TARGETS]}
   def changes(repo,before,after):
    return ['engine/sw/source/core/text/guess.cxx'] if before=='base' else []
   def acquire_lock(stream,operation):
    # Simulate a private build invalidating metadata before releasing the lock.
    built.unlink()
   with patch.dict(os.environ,env),patch.object(r,'HOME',root),patch.object(r,'CACHE',cache), \
        patch.object(r,'AUTH',auth),patch.object(r,'run',return_value=''), \
        patch.object(r,'k',side_effect=[json.dumps(deployments),'{"items":[]}']), \
        patch.object(r.shutil,'disk_usage',return_value=SimpleNamespace(free=100*1024**3)), \
        patch.object(r.fcntl,'flock',side_effect=acquire_lock), \
        patch.object(r,'source_changes',side_effect=changes), \
        patch.object(r,'compile_source') as compile_source, \
        patch.object(r,'build_images',return_value={}),patch.object(r,'deploy'):
    r.main()
   compile_source.assert_called_once_with(Path(paths['online.mirror']),True,False,
                                          Path(paths['cocolian-office']))
   self.assertEqual(json.loads(built.read_text()),{'online.mirror':'current'})

 def test_compile_reserves_cpu_and_applies_limit_to_both_test_profiles(self):
  for cpus,expected in [(1,1),(4,3),(6,5),(16,5)]:
   commands=[]
   def command(args,**kwargs):
    commands.append(args)
    return ''
   with patch.object(r.os,'cpu_count',return_value=cpus),patch.object(r,'sync_source'),patch.object(r,'run',side_effect=command):
    r.compile_source(Path('/source'),True,True,Path('/office'))
   builds=[a for a in commands if a[:2]==['docker','run']]
   self.assertEqual(len(builds),2)
   for args in builds:
    self.assertEqual(args[args.index('--cpus')+1],str(expected))
    self.assertEqual(args[args.index('--memory')+1],'12g')
    self.assertIn('make -j'+str(expected),args[-1])

 def test_reuse_requires_identical_layers_and_config(self):
  source={'rootfs':{'diff_ids':['sha256:layer']},'config':{'User':'1001'},'os':'linux','architecture':'amd64'}
  local={'RootFS':{'Layers':['sha256:layer']},'Config':{'User':'1001'},'Os':'linux','Architecture':'amd64'}
  for mismatch in (None,'RootFS','Config'):
   candidate=copy.deepcopy(local)
   if mismatch=='RootFS':candidate['RootFS']['Layers']=['sha256:wrong']
   if mismatch=='Config':candidate['Config']['User']='0'
   def command(args,**kw):
    if args[:2]==['skopeo','inspect']:return json.dumps(source)
    if args[:3]==['docker','image','ls']:return 'cocolian/test:old'
    if args[:3]==['docker','image','inspect']:return json.dumps([candidate])
    if args[:2]==['docker','tag']:return ''
    raise AssertionError(args)
   with patch.object(r,'run',side_effect=command)as run,patch.object(r,'registry_transport',return_value=('127.0.0.1:15000',[],[],[])):
    self.assertEqual(r.reuse_local_base('res.cocolian.cn/cocolian/test@sha256:digest','test','verified'),mismatch is None)
    tags=[x.args[0]for x in run.call_args_list if x.args[0][:2]==['docker','tag']]
    self.assertEqual(len(tags),1 if mismatch is None else 0)

 def test_publish_streams_and_checks_immutable_digest(self):
  digest='sha256:'+'a'*64
  source={'Id':'immutable','RootFS':{'Layers':['layer']},'Config':{'User':'1001'},'Os':'linux','Architecture':'amd64'}
  for mismatch in (None,'digest','config'):
   with tempfile.TemporaryDirectory()as root:
    directories=[]
    def command(args,**kw):
     if args[:3]==['docker','image','inspect']:return json.dumps([source])
     if args[:2]==['docker','save']:
      Path(args[3]).write_bytes(b'exported image');return ''
     if args[:2]==['skopeo','copy']:
      self.assertTrue(args[-2].startswith('docker-archive:'));self.assertTrue(args[-2].endswith('/test.docker.tar'))
      self.assertEqual(args[-1],'docker://127.0.0.1:15000/cocolian/test:tag')
      path=Path(args[args.index('--digestfile')+1]);directories.append(path.parent);path.write_text(digest);return ''
     if args[:2]==['skopeo','inspect']:
      if '--config'in args:
       remote={'rootfs':{'diff_ids':['layer']},'config':{'User':'0'if mismatch=='config'else'1001'},'os':'linux','architecture':'amd64'}
       return json.dumps(remote)
      return digest if mismatch!='digest'else'sha256:'+'b'*64
     raise AssertionError(args)
    with patch.object(r,'registry_transport',return_value=('127.0.0.1:15000',[],[],[])),patch.object(r,'run',side_effect=command):
     if mismatch is None:self.assertEqual(r.publish('cocolian/test:tag','test',Path(root)),'res.cocolian.cn/cocolian/test@'+digest)
     else:
      with self.assertRaisesRegex(AssertionError,'mismatch'):r.publish('cocolian/test:tag','test',Path(root))
    self.assertTrue(directories);self.assertTrue(all(not x.exists()for x in directories))
    self.assertEqual(list(Path(root).iterdir()),[])

 def test_native_publish_checks_digest_and_image_identity(self):
  digest='sha256:'+'a'*64
  source={'Id':'immutable','RootFS':{'Layers':['layer']},'Config':{'User':'1001'},'Os':'linux','Architecture':'amd64'}
  for mismatch in (None,'digest','config','push'):
   with tempfile.TemporaryDirectory() as directory:
    root=Path(directory);auth=root/'auth.json';auth.write_text(json.dumps({'auths':{'res.cocolian.cn':{'auth':'dTpw'}}}))
    def command(args,**kw):
     if args[:3]==['docker','image','inspect']:return json.dumps([source])
     if args[:2]==['docker','tag'] or args[:3]==['docker','image','rm']:return ''
     if args[:2]==['docker','--config']:
      self.assertEqual(args[3],'push');self.assertTrue((Path(args[2])/'config.json').exists())
      return 'digest: '+digest if mismatch!='push' else 'no digest'
     if args[:2]==['skopeo','inspect']:
      if '--config' in args:return json.dumps({'rootfs':{'diff_ids':['layer']},'config':{'User':'0' if mismatch=='config' else '1001'},'os':'linux','architecture':'amd64'})
      return digest if mismatch!='digest' else 'sha256:'+'b'*64
     raise AssertionError(args)
    with patch.object(r,'AUTH',auth),patch.object(r,'registry_transport',return_value=('127.0.0.1:15444',[],[],[])),patch.object(r,'run',side_effect=command) as runner:
     if mismatch is None:self.assertEqual(r.publish('cocolian/test:tag','test',root),'res.cocolian.cn/cocolian/test@'+digest)
     else:
      with self.assertRaises((AssertionError,RuntimeError)):r.publish('cocolian/test:tag','test',root)
     self.assertTrue(any(c.args[0][:3]==['docker','image','rm'] for c in runner.call_args_list))
    self.assertEqual([p.name for p in root.iterdir()],['auth.json'])

 def test_private_font_hash_gate(self):
  import hashlib
  with tempfile.TemporaryDirectory() as directory:
   root=Path(directory);source=root/'private';source.mkdir();(source/'font.ttf').write_bytes(b'approved font')
   manifest={'fonts':[{'file':'font.ttf','sha256':hashlib.sha256(b'approved font').hexdigest()}]}
   self.assertEqual(r.stage_office_fonts(root/'valid',source,manifest),1)
   (source/'font.ttf').write_bytes(b'changed font')
   with self.assertRaisesRegex(RuntimeError,'hash mismatch'):r.stage_office_fonts(root/'changed',source,manifest)
   (source/'font.ttf').unlink()
   with self.assertRaisesRegex(RuntimeError,'missing'):r.stage_office_fonts(root/'missing',source,manifest)

 def test_failure_restores_templates(self):
  template={'spec':{'containers':[{'image':'old','imagePullPolicy':'IfNotPresent'}]}}
  old={name:{'spec':{'template':copy.deepcopy(template)}} for name in ('one','two')}
  current={name:copy.deepcopy(template) for name in old};restored=[]
  def kubectl(*args,**kw):
   name=args[4] if args[2]=='patch' else None
   if args[2]=='patch':
    ops=json.loads(args[-1])
    if name=='two':raise RuntimeError('simulated deployment failure')
    self.assertEqual(ops[0]['value'],current[name])
    if ops[1]['path']=='/spec/template':current[name]=ops[1]['value'];restored.append(name)
    else:current[name]['spec']['containers'][0]['image']=ops[1]['value']
   if args[2]=='get':return json.dumps({'spec':{'template':current[args[4]]}})
   return ''
  with tempfile.TemporaryDirectory() as directory,patch.object(r,'candidate'),patch.object(r,'k',side_effect=kubectl):
   with self.assertRaisesRegex(RuntimeError,'simulated'):r.deploy(old,{'one':'new-one','two':'new-two'},Path(directory),lambda:None)
   self.assertEqual(restored,['one']);self.assertEqual(current['one'],template)
 def test_rollback_refuses_concurrent_change(self):
  template={'spec':{'containers':[{'image':'old','imagePullPolicy':'IfNotPresent'}]}}
  old={'one':{'spec':{'template':template}}};calls=[]
  def kubectl(*args,**kw):
   calls.append(args)
   if args[2]=='get':return json.dumps({'spec':{'template':{'spec':{'containers':[{'image':'someone-else'}]}}}})
   return ''
  def fail():raise RuntimeError('health failed')
  with tempfile.TemporaryDirectory() as directory,patch.object(r,'candidate'),patch.object(r,'k',side_effect=kubectl):
   with self.assertRaisesRegex(RuntimeError,'health'):r.deploy(old,{'one':'new'},Path(directory),fail)
   result=json.loads((Path(directory)/'rollback.json').read_text());self.assertIn('Concurrent',result['errors'][0])
   self.assertEqual(sum(c[2]=='patch' for c in calls),1)
if __name__=='__main__':unittest.main()

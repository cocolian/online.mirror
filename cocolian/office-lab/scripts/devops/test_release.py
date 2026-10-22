import copy,importlib.util,json,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
spec=importlib.util.spec_from_file_location('release',Path(__file__).with_name('release.py'));r=importlib.util.module_from_spec(spec);spec.loader.exec_module(r)
class ReleaseTests(unittest.TestCase):
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

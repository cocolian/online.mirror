#!/usr/bin/env python3
"""Normalize one OCI image archive for the restricted host importer."""
import hashlib,json,re,sys,tarfile,io
source,destination,image=sys.argv[1:]
if not re.fullmatch(r'docker\.io/cocolian/(collabora-source|libreoffice-source):[A-Za-z0-9][A-Za-z0-9_.-]{0,127}',image):raise SystemExit('Unexpected image name')
with tarfile.open(source) as src:
 members={}
 for m in src.getmembers():
  if m.isdir():continue
  if not m.isfile() or m.name in members:raise ValueError('Unsupported archive member')
  if m.name not in ('index.json','oci-layout') and not re.fullmatch(r'blobs/sha256/[a-f0-9]{64}',m.name):raise ValueError('Unsupported OCI path')
  members[m.name]=m
  if m.name.startswith('blobs/'):
   h=hashlib.sha256()
   with src.extractfile(m) as f:
    for chunk in iter(lambda:f.read(1024*1024),b''):h.update(chunk)
   if h.hexdigest()!=m.name.rsplit('/',1)[1]:raise ValueError('Blob hash mismatch')
 def read(path):
  if members[path].size>2*1024*1024:raise ValueError('JSON too large')
  return json.load(src.extractfile(members[path]))
 def blob(d):
  if not re.fullmatch(r'sha256:[a-f0-9]{64}',d):raise ValueError('Invalid digest')
  return 'blobs/sha256/'+d.split(':')[1]
 index=read('index.json')
 if len(index['manifests'])!=1:raise ValueError('Expected exactly one image manifest; disable provenance export')
 descriptor=index['manifests'][0]
 if descriptor['mediaType']!='application/vnd.oci.image.manifest.v1+json':raise ValueError('Expected flat OCI manifest')
 manifest=read(blob(descriptor['digest']));config=read(blob(manifest['config']['digest']))
 if config['architecture']!='amd64' or config['os']!='linux':raise ValueError('Expected linux/amd64')
 if config.get('config',{}).get('User')!='1001:1001':raise ValueError('Expected unprivileged runtime user')
 for layer in manifest['layers']:
  if blob(layer['digest']) not in members:raise ValueError('Missing layer')
 descriptor.setdefault('annotations',{})['io.containerd.image.name']=image
 descriptor['annotations']['org.opencontainers.image.ref.name']=image
 index_data=json.dumps(index,separators=(',',':')).encode()
 with tarfile.open(destination,'x') as out:
  for name,m in members.items():
   if name=='index.json':
    header=tarfile.TarInfo(name);header.size=len(index_data);header.mode=0o644
    out.addfile(header,io.BytesIO(index_data))
   else:out.addfile(m,src.extractfile(m))
 print(json.dumps({'image':image,'digest':descriptor['digest'],'deployment_image':image.rsplit(':',1)[0]+'@'+descriptor['digest']}))

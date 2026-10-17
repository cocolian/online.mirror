import urllib.request,json,hashlib,os,base64,time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
out=Path('/home/codex/projects/cocolian-office/artifacts/fonts-20261017');out.mkdir(exist_ok=True);fonts=out/'fonts';fonts.mkdir(exist_ok=True);manifest=[];locked=json.loads((out/'fonts-lock.json').read_text()) if (out/'fonts-lock.json').exists() else []
for family in ['sans','serif']:
 repo='adobe-fonts/source-han-'+family
 req=urllib.request.Request('https://api.github.com/repos/'+repo+'/commits/release',headers={'User-Agent':'cocolian-font-fetch'})
 commit=next((x['commit'] for x in locked if '/'+repo+'/' in x['url']),None) or json.load(urllib.request.urlopen(req,timeout=60))['sha']
 base='https://raw.githubusercontent.com/'+repo+'/'+commit+'/'
 lic=urllib.request.urlopen(base+'LICENSE.txt',timeout=60).read();(fonts/(family+'-LICENSE.txt')).write_bytes(lic)
 for weight in ['Regular','Bold']:
  name='SourceHan'+family.title()+'SC-'+weight+'.otf';url=base+'OTF/SimplifiedChinese/'+name
  meta=json.load(urllib.request.urlopen(urllib.request.Request('https://api.github.com/repos/'+repo+'/contents/OTF/SimplifiedChinese/'+name+'?ref='+commit,headers={'User-Agent':'cocolian-font-fetch'}),timeout=60))
  def chunk(start):
   end=min(start+1024*1024,meta['size'])-1
   for attempt in range(4):
    try:
     req=urllib.request.Request(url,headers={'Range':f'bytes={start}-{end}'})
     response=urllib.request.urlopen(req,timeout=60);value=response.read()
     assert response.status==206 and len(value)==end-start+1
     return value
    except Exception:
     if attempt==3:raise
     time.sleep(1)
  with ThreadPoolExecutor(max_workers=8) as pool:data=b''.join(pool.map(chunk,range(0,meta['size'],1024*1024)))
  assert data[:4]==b'OTTO' and hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()==meta['sha']
  expected=next((x['sha256'] for x in locked if x['file']==name),None)
  assert expected is None or hashlib.sha256(data).hexdigest()==expected
  (fonts/name).write_bytes(data);print(name,len(data),flush=True)
  manifest.append(dict(file=name,url=url,commit=commit,bytes=len(data),sha256=hashlib.sha256(data).hexdigest(),license=family+'-LICENSE.txt'))
(out/'fonts-lock.json').write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps(manifest))

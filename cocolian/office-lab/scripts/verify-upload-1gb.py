"""Exercise the exact 1 GiB HTTP limit on a disposable local WOPI host."""
import os, json, tempfile, shutil, subprocess, secrets, time, http.client, urllib.parse, hashlib, threading
from pathlib import Path
source=Path('/home/codex/projects/cocolian/apps/office-portal');work=Path(tempfile.mkdtemp(prefix='office-upload-1gb-'));data=work/'data';data.mkdir();key=secrets.token_urlsafe(32)
for p in source.iterdir():
 if p.is_file():shutil.copy2(p,work/p.name)
s=(work/'server.py').read_text().replace("Path('/data')","Path(os.environ['SOURCE_TEST_DATA'])").replace("('0.0.0.0',8000)","('127.0.0.1',18081)");(work/'server.py').write_text(s)
log=(work/'host.log').open('w');host=subprocess.Popen(['python3',str(work/'server.py')],env=dict(os.environ,LAB_KEY=key,SOURCE_TEST_DATA=str(data)),stdout=log,stderr=log);result={};peak=[0];stop=threading.Event()
def sample():
 while not stop.wait(.02):
  try:
   for line in Path('/proc/'+str(host.pid)+'/status').read_text().splitlines():
    if line.startswith('VmRSS:'):peak[0]=max(peak[0],int(line.split()[1]))
  except FileNotFoundError:pass
class Zeroes:
 def __init__(self,length):self.remaining=length
 def read(self,size):
  size=min(size,self.remaining);self.remaining-=size;return b'\0'*size

def request(method,path,length=0,body=None):
 c=http.client.HTTPConnection('127.0.0.1',18081,timeout=120)
 c.request(method,path,body=body,headers={'Authorization':'Bearer '+key,'Content-Type':'application/octet-stream','Content-Length':str(length)})
 return c,c.getresponse()
try:
 time.sleep(.5);thread=threading.Thread(target=sample);thread.start();limit=1024**3
 for length in [0,limit+1]:
  c,r=request('POST','/api/files?name=boundary.txt',length);assert r.status==413,(length,r.status);r.read();c.close()
 started=time.monotonic();c,r=request('POST','/api/files?name=boundary.txt',limit,Zeroes(limit));assert r.status==201,r.read();record=json.loads(r.read());c.close();result['exact_1gb_upload']=record['size']==limit;assert result['exact_1gb_upload'];path=data/'knowledge-base'/record['id']/'boundary.txt';assert path.stat().st_size==limit
 c,r=request('HEAD','/download?file='+record['id']);assert r.status==200 and int(r.getheader('Content-Length'))==limit;r.read();c.close()
 c,r=request('GET','/download?file='+record['id']);assert r.status==200;count=0
 while chunk:=r.read(1024*1024):count+=len(chunk)
 assert count==limit;c.close();result['streamed_download']=True
 token=hashlib.sha256((key+'Alice').encode()).hexdigest();c,r=request('POST','/wopi/files/'+record['id']+'/contents?access_token='+token,limit,Zeroes(limit));assert r.status==200,r.read();r.read();c.close();result['streamed_wopi_save']=True
 assert not list((data/'knowledge-base').glob('.upload-*'));assert not list(path.parent.glob('.save-*'))
 result.update(oversize_rejected=True,zero_rejected=True,peak_server_rss_kib=peak[0],elapsed_seconds=round(time.monotonic()-started,2));assert peak[0]<128*1024,result
finally:
 stop.set();host.terminate();host.wait(timeout=10);log.close();shutil.rmtree(work)
print(json.dumps(result))

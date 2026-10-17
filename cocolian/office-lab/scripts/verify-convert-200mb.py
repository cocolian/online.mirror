"""Exact 200 MiB multipart boundary test against an isolated source converter."""
import http.client,io,tempfile,zipfile,time,json
from pathlib import Path
limit=200*1024*1024
fixture=Path('/home/codex/projects/cocolian-office/tests/fixtures/source-build-smoke.docx')
class Parts:
 def __init__(self,*parts):self.parts=list(parts)
 def read(self,size):
  while self.parts:
   value=self.parts[0].read(size)
   if value:return value
   self.parts.pop(0)
  return b''
def request(payload,filename='boundary.docx',extra=b'',crlf=True):
 prefix=('---office-boundary\r\nContent-Disposition: form-data; name="files"; filename="'+filename+'"\r\nContent-Type: application/vnd.openxmlformats-officedocument.wordprocessingml.document\r\n\r\n').encode()
 suffix=b'\r\n---office-boundary--'+(b'\r\n' if crlf else b'')
 with payload.open('rb') as stream:
  c=http.client.HTTPConnection('127.0.0.1',13000,timeout=360)
  c.request('POST','/forms/libreoffice/convert',body=Parts(io.BytesIO(prefix),stream,io.BytesIO(extra),io.BytesIO(suffix)),headers={'Content-Type':'multipart/form-data; boundary=-office-boundary','Content-Length':str(len(prefix)+payload.stat().st_size+len(extra)+len(suffix))})
  response=c.getresponse();body=response.read();status=response.status;c.close();return status,body
with tempfile.TemporaryDirectory(prefix='office-convert-limit-') as temp:
 path=Path(temp)/'boundary.docx'
 def write(padding):
  with zipfile.ZipFile(fixture) as original,zipfile.ZipFile(path,'w',compression=zipfile.ZIP_STORED) as out:
   for name in original.namelist():out.writestr(name,original.read(name))
   info=zipfile.ZipInfo('padding.bin');info.file_size=padding
   with out.open(info,'w') as target:
    while padding:
     n=min(padding,1024*1024);target.write(b'\0'*n);padding-=n
 write(0);overhead=path.stat().st_size;write(limit-overhead);assert path.stat().st_size==limit
 started=time.monotonic();status,body=request(path);assert status==200,(status,body[:300]);assert body.startswith(b'%PDF-')
 result={'exact_200mb_docx':True,'pdf_bytes':len(body),'elapsed_seconds':round(time.monotonic()-started,2)}
 status,body=request(path,extra=b'0');assert status==413,(status,body[:200]);result['one_byte_over_rejected']=True
 status,body=request(fixture,crlf=False);assert status==200 and body.startswith(b'%PDF-');result['closing_without_crlf']=True
 status,body=request(fixture,filename='test.txt');assert status==415;result['non_docx_rejected']=True
 print(json.dumps(result))

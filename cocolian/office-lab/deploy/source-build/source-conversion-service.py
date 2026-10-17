#!/usr/bin/python3
"""Single-worker DOCX-to-PDF adapter with bounded, streamed request handling."""
import json,os,subprocess,tempfile,threading,shutil
from email.parser import BytesParser
from email.policy import default
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path

ENGINE='/opt/collaboraoffice/program/soffice'
LIMIT=200*1024*1024
SLOT=threading.BoundedSemaphore(1)

class UploadError(Exception):
 def __init__(self,status,message):self.status=status;self.message=message

def receive_docx(reader,length,content_type,path):
 header=BytesParser(policy=default).parsebytes(('Content-Type: '+content_type+'\r\n\r\n').encode())
 boundary=header.get_boundary()
 if not boundary or len(boundary)>70 or not boundary.isascii() or '\r' in boundary or '\n' in boundary:raise UploadError(400,'Invalid multipart boundary')
 boundary=boundary.encode();opening=b'--'+boundary+b'\r\n';remaining=length
 def line():
  nonlocal remaining
  value=reader.readline(min(remaining+1,16385));remaining-=len(value)
  if not value or remaining<0 or len(value)>16384:raise UploadError(400,'Invalid multipart headers')
  return value
 if line()!=opening:raise UploadError(400,'Invalid multipart opening')
 headers=b''
 while True:
  value=line();headers+=value
  if len(headers)>16384:raise UploadError(400,'Multipart headers too large')
  if value==b'\r\n':break
 part=BytesParser(policy=default).parsebytes(headers)
 if part.get_param('name',header='content-disposition')!='files':raise UploadError(400,'Only one files field is supported')
 if not (part.get_filename() or '').lower().endswith('.docx'):raise UploadError(415,'Only DOCX is supported')
 if part.get('Content-Transfer-Encoding','binary').lower() not in ('binary','8bit'):raise UploadError(415,'Encoded uploads are not supported')
 marker=b'\r\n--'+boundary;closing=marker+b'--';tail_size=len(closing)+2;tail=b'';size=0
 with path.open('xb') as output:
  def write(value):
   nonlocal size
   size+=len(value)
   if size>LIMIT:raise UploadError(413,'DOCX exceeds 200 MiB')
   output.write(value)
  while remaining:
   chunk=reader.read(min(1024*1024,remaining))
   if not chunk:raise UploadError(400,'Incomplete upload')
   remaining-=len(chunk);buffer=tail+chunk;cut=max(0,len(buffer)-tail_size)
   if marker in buffer and buffer.find(marker)<cut:raise UploadError(400,'Only one files field is supported')
   write(buffer[:cut]);tail=buffer[cut:]
  if tail.endswith(closing+b'\r\n'):write(tail[:-len(closing)-2])
  elif tail.endswith(closing):write(tail[:-len(closing)])
  else:raise UploadError(400,'Invalid multipart closing')
 if not size:raise UploadError(400,'Empty DOCX')
 with path.open('rb') as stream:
  if stream.read(2)!=b'PK':raise UploadError(400,'Expected a DOCX ZIP document')

class Handler(BaseHTTPRequestHandler):
 def reply(self,status,body=b'',kind='application/json'):
  self.send_response(status);self.send_header('Content-Type',kind);self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body)
 def error(self,status,message):self.reply(status,json.dumps({'error':message}).encode())
 def do_GET(self):
  if self.path=='/health':return self.reply(200,b'{"status":"up","engine":"source-cli","api":"docx-to-pdf-subset"}')
  return self.error(404,'Not found')
 def do_POST(self):
  if self.path!='/forms/libreoffice/convert':return self.error(404,'Not found')
  try:length=int(self.headers.get('Content-Length','0'))
  except ValueError:return self.error(400,'Invalid content length')
  if length<=0 or length>LIMIT+64*1024:return self.error(413,'DOCX maximum is 200 MiB')
  content_type=self.headers.get('Content-Type','')
  if not content_type.startswith('multipart/form-data;'):return self.error(415,'Expected multipart/form-data')
  if not SLOT.acquire(timeout=5):return self.error(503,'Conversion service is busy')
  try:
   self.connection.settimeout(600)
   with tempfile.TemporaryDirectory(prefix='source-convert-') as directory:
    work=Path(directory);source=work/'input.docx'
    try:receive_docx(self.rfile,length,content_type,source)
    except UploadError as error:return self.error(error.status,error.message)
    except OSError:return self.error(507,'Upload incomplete or insufficient storage')
    command=[ENGINE,'-env:UserInstallation='+(work/'profile').as_uri(),'--headless','--convert-to','pdf','--outdir',str(work),str(source)]
    try:result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=300,env=dict(os.environ,SAL_USE_VCLPLUGIN='svp'))
    except subprocess.TimeoutExpired:return self.error(504,'Source engine conversion timed out')
    output=work/'input.pdf'
    if result.returncode or not output.is_file():return self.error(500,'Source engine failed to convert this document')
    if output.stat().st_size>LIMIT:return self.error(413,'Generated PDF exceeds 200 MiB')
    with output.open('rb') as stream:
     if stream.read(5)!=b'%PDF-':return self.error(500,'Invalid source engine PDF output')
     stream.seek(0);self.send_response(200);self.send_header('Content-Type','application/pdf');self.send_header('Content-Length',str(output.stat().st_size));self.end_headers();shutil.copyfileobj(stream,self.wfile,1024*1024)
  finally:SLOT.release()
 def log_message(self,fmt,*args):print(json.dumps({'method':self.command,'path':self.path.split('?')[0],'status':args[1] if len(args)>1 else ''}),flush=True)
if __name__=='__main__':ThreadingHTTPServer(('0.0.0.0',3000),Handler).serve_forever()

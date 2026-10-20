import hashlib,http.client,importlib.util,threading,unittest
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
spec=importlib.util.spec_from_file_location('proxy',Path(__file__).with_name('registry_proxy.py'));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class Upstream(BaseHTTPRequestHandler):
 def do_POST(self):
  data=b''
  if self.headers.get('Transfer-Encoding')=='chunked':
   while True:
    size=int(self.rfile.readline().strip(),16)
    if not size:self.rfile.readline();break
    data+=self.rfile.read(size);self.rfile.read(2)
  else:data=self.rfile.read(int(self.headers.get('Content-Length',0)))
  result=hashlib.sha256(data).hexdigest().encode();self.send_response(202);self.send_header('Location','http://res.cocolian.cn/v2/test/blobs/uploads/owned?state=opaque');self.send_header('Content-Length',str(len(result)));self.end_headers();self.wfile.write(result)
 def log_message(self,*args):pass
class ProxyTests(unittest.TestCase):
 def test_streamed_upload_and_absolute_location(self):
  upstream=ThreadingHTTPServer(('127.0.0.1',0),Upstream)
  class Handler(m.Proxy):
   def log_message(self,*args):pass
  Handler.upstream=upstream.server_address
  proxy=ThreadingHTTPServer(('127.0.0.1',0),Handler)
  for server in (upstream,proxy):threading.Thread(target=server.serve_forever,daemon=True).start()
  try:
   for chunked in (False,True):
    blocks=[b'a'*1048576,b'b'*1048576];body=iter(blocks) if chunked else b''.join(blocks)
    c=http.client.HTTPConnection(*proxy.server_address,timeout=10);c.request('POST','/v2/test/blobs/uploads/',body=body,encode_chunked=chunked)
    r=c.getresponse();self.assertEqual(r.status,202);self.assertEqual(r.getheader('Location'),'http://127.0.0.1:15000/v2/test/blobs/uploads/owned?state=opaque');self.assertEqual(r.read(),hashlib.sha256(b''.join(blocks)).hexdigest().encode());c.close()
  finally:
   for server in (proxy,upstream):server.shutdown();server.server_close()
if __name__=='__main__':unittest.main()

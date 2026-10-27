#!/usr/bin/env python3
"""TLS endpoint for the existing loopback registry SSH transport.
Listen only on 127.0.0.1; retain the original SSH tunnel and public TLS.
Docker trusts this service's private certificate only for 127.0.0.1:15444.
"""
import ssl
import http.client
import urllib.parse
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer

class Proxy(BaseHTTPRequestHandler):
 protocol_version='HTTP/1.1'
 upstream=('127.0.0.1',15000)
 location_root='https://127.0.0.1:15444'
 def relay(self):
  connection=http.client.HTTPConnection(*self.upstream,timeout=600)
  try:
   connection.putrequest(self.command,self.path,skip_host=True,skip_accept_encoding=True)
   for name,value in self.headers.items():
    if name.lower() not in ('host','connection','proxy-connection','expect'):
     connection.putheader(name,value)
   connection.putheader('Host','res.cocolian.cn');connection.putheader('Connection','close');connection.endheaders()
   if 'chunked' in self.headers.get('Transfer-Encoding','').lower():
    while True:
     line=self.rfile.readline(8192)
     if not line or len(line)>=8192:raise ValueError('Invalid chunk header')
     size=int(line.split(b';',1)[0].strip(),16);connection.send(line)
     remaining=size+2 if size else 0
     while remaining:
      block=self.rfile.read(min(1024*1024,remaining))
      if not block:raise ValueError('Truncated upload')
      connection.send(block);remaining-=len(block)
     if not size:
      while True:
       trailer=self.rfile.readline(8192);connection.send(trailer)
       if trailer==b'\r\n':break
       if not trailer or len(trailer)>=8192:raise ValueError('Invalid chunk trailer')
      break
   else:
    remaining=int(self.headers.get('Content-Length','0'))
    while remaining:
     block=self.rfile.read(min(1024*1024,remaining))
     if not block:raise ValueError('Truncated upload')
     connection.send(block);remaining-=len(block)
   response=connection.getresponse();self.send_response(response.status,response.reason)
   for name,value in response.getheaders():
    if name.lower() in ('transfer-encoding','connection','keep-alive','server','date'):continue
    if name.lower()=='location':
     parsed=urllib.parse.urlsplit(value)
     if parsed.netloc not in ('res.cocolian.cn','res.cocolian.cn:80','127.0.0.1:15000','10.43.173.55','10.43.173.55:80','127.0.0.1:15444') and parsed.netloc:
      raise ValueError('Unexpected registry redirect destination')
     value=self.location_root+urllib.parse.urlunsplit(('','',parsed.path,parsed.query,''))
    self.send_header(name,value)
   self.send_header('Connection','close');self.end_headers();self.close_connection=True
   if self.command!='HEAD':
    while True:
     block=response.read(1024*1024)
     if not block:break
     self.wfile.write(block)
  finally:connection.close()
 do_GET=do_HEAD=do_POST=do_PATCH=do_PUT=do_DELETE=relay
 def log_message(self,fmt,*args):
  # Never log Authorization, query strings, or Registry signed upload states.
  print(self.command,self.path.split('?')[0],args[1] if len(args)>1 else '',flush=True)
if __name__=='__main__':
 server=ThreadingHTTPServer(('127.0.0.1',15444),Proxy)
 context=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
 context.load_cert_chain('/home/codex/.config/cocolian-devops/local-registry-tls/cert.pem','/home/codex/.config/cocolian-devops/local-registry-tls/key.pem')
 server.socket=context.wrap_socket(server.socket,server_side=True)
 server.serve_forever()

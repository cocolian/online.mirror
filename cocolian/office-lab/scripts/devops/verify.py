#!/usr/bin/env python3
"""Synthetic HTTP/WOPI/conversion smoke checks; never edits customer documents."""
import hashlib,http.cookiejar,io,json,os,re,sys,urllib.parse as P,urllib.request as U,uuid,zipfile
from pathlib import Path

def fixture(text):
 b=io.BytesIO()
 with zipfile.ZipFile(b,'w',zipfile.ZIP_DEFLATED) as z:
  z.writestr('[Content_Types].xml','''<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="xml" ContentType="application/xml"/><Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/></Types>''')
  z.writestr('_rels/.rels','''<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/></Relationships>''')
  z.writestr('word/document.xml','''<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:eastAsia="仿宋_GB2312"/></w:rPr><w:t>'''+text+'''</w:t></w:r></w:p><w:sectPr/></w:body></w:document>''')
 return b.getvalue()

def convert(base,route,field,raw):
 boundary='devops'+uuid.uuid4().hex
 body=('--'+boundary+'\r\nContent-Disposition: form-data; name="'+field+'"; filename="devops-smoke.docx"\r\nContent-Type: application/vnd.openxmlformats-officedocument.wordprocessingml.document\r\n\r\n').encode()+raw+('\r\n--'+boundary+'--\r\n').encode()
 with U.urlopen(U.Request(base+route,data=body,headers={'Content-Type':'multipart/form-data; boundary='+boundary}),timeout=90) as r:
  value=r.read();assert r.status==200 and value.startswith(b'%PDF-') and len(value)>1000
 return {'http':200,'pdf_bytes':len(value)}

def main():
 raw=fixture('Cocolian DevOps 自动发布验证，仿宋字体。')
 if len(sys.argv)>1 and sys.argv[1]=='--candidate':
  name,ip=sys.argv[2:4]
  if name=='wopi':assert U.urlopen('http://'+ip+':8000/health',timeout=15).status==200
  else:
   port=3000 if name=='libreoffice' else 9980
   prefix='/word-mac' if name=='collabora-wordmac' else ''
   route='/forms/libreoffice/convert' if name=='libreoffice' else prefix+'/cool/convert-to/pdf'
   print(json.dumps(convert('http://'+ip+':'+str(port),route,'files' if name=='libreoffice' else 'data',raw)))
  return
 checks={};key=os.environ['LAB_KEY'].strip();base='http://127.0.0.1:8000'
 jar=http.cookiejar.CookieJar();client=U.build_opener(U.HTTPCookieProcessor(jar))
 with client.open(U.Request(base+'/login',data=P.urlencode({'key':key}).encode()),timeout=15) as r:
  assert r.status==200 and '文档工作台' in r.read().decode();checks['login']=True
 for url in ['http://collabora:9980/hosting/discovery','http://collabora-wordmac:9980/word-mac/hosting/discovery','http://libreoffice:3000/health','http://127.0.0.1:8080/health']:
  assert U.urlopen(url,timeout=15).status==200
 checks['discovery_and_health']=True
 for fid,prefix in [('1','/browser/'),('215a82d508cc45729ae9cbfa595a6927','/word-mac/browser/')]:
  html=client.open(base+'/edit?user=Bob&file='+fid,timeout=20).read().decode()
  assert 'action="'+prefix in html
  # Check the actual browser entry through the gateway, not just discovery.
  action=__import__('html').unescape(re.search(r'action="([^"]+cool.html[^\"]*)"',html).group(1))
  assert client.open('http://127.0.0.1:8080'+action,timeout=20).status==200
 checks['editor_entry_and_profile_routes']=True
 for name,url,route,field in [('editor','http://collabora:9980','/cool/convert-to/pdf','data'),('wordmac','http://collabora-wordmac:9980','/word-mac/cool/convert-to/pdf','data'),('converter','http://libreoffice:3000','/forms/libreoffice/convert','files')]:checks[name]=convert(url,route,field,raw)
 def request(path,data=None,headers=None):return U.urlopen(U.Request(base+path,data=data,headers=headers or {}),timeout=20)
 fid=None;name='devops-smoke-'+uuid.uuid4().hex+'.docx'
 try:
  with request('/api/files?name='+name,raw,{'Authorization':'Bearer '+key,'Content-Type':'application/octet-stream'}) as r:fid=json.load(r)['id']
  assert re.fullmatch('[0-9a-f]{32}',fid)
  token=hashlib.sha256((key+'Bob').encode()).hexdigest();path='/wopi/files/'+fid+'?access_token='+token;lock='devops-'+uuid.uuid4().hex
  assert request(path,b'',{'X-WOPI-Override':'LOCK','X-WOPI-Lock':lock}).status==200
  updated=fixture('Cocolian DevOps WOPI save verified.')
  assert request('/wopi/files/'+fid+'/contents?access_token='+token,updated,{'X-WOPI-Lock':lock}).status==200
  assert request('/wopi/files/'+fid+'/contents?access_token='+token).read()==updated
  assert request(path,b'',{'X-WOPI-Override':'UNLOCK','X-WOPI-Lock':lock}).status==200
  checks['wopi_upload_lock_save_download_unlock']=True
 finally:
  if fid:
   directory=Path(os.environ.get('KNOWLEDGE_DIR','/data/knowledge-base'))/fid;p=directory/name
   # Only this invocation's UUID-named synthetic document may be removed.
   if p.is_file() and not p.is_symlink():p.unlink();directory.rmdir()
 print(json.dumps(checks))
if __name__=='__main__':main()

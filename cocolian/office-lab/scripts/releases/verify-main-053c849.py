"""Verify conversion against loopback candidate; no live documents."""
import urllib.request,time,json
from pathlib import Path
root=Path('/home/codex/projects/cocolian-office');out=root/'artifacts/main-053c849-20261020'
for _ in range(30):
 try:urllib.request.urlopen('http://127.0.0.1:13000/health',timeout=1);break
 except Exception:time.sleep(1)
else:raise RuntimeError('Converter not ready')
results=[]
for file in [root/'tests/fixtures/source-build-smoke.docx',root/'src/online.mirror/engine/sw/qa/extras/layout/data/word-import-typography.docx',root/'src/online.mirror/engine/sw/qa/extras/layout/data/word-table-grid-proportional.docx']:
 boundary='Main053c849';body=('--'+boundary+'\r\nContent-Disposition: form-data; name="files"; filename="'+file.name+'"\r\nContent-Type: application/vnd.openxmlformats-officedocument.wordprocessingml.document\r\n\r\n').encode()+file.read_bytes()+('\r\n--'+boundary+'--\r\n').encode()
 req=urllib.request.Request('http://127.0.0.1:13000/forms/libreoffice/convert',data=body,headers={'Content-Type':'multipart/form-data; boundary='+boundary})
 with urllib.request.urlopen(req,timeout=90) as r:
  pdf=r.read();assert r.status==200 and pdf.startswith(b'%PDF-');(out/(file.stem+'.pdf')).write_bytes(pdf);results.append({'file':file.name,'status':r.status,'bytes':len(pdf)})
(out/'candidate-conversion.json').write_text(json.dumps(results,indent=2)+'\n');print(json.dumps(results))

import zipfile,subprocess,json,urllib.request
from pathlib import Path
root=Path('/home/codex/projects/cocolian-office');out=root/'artifacts/fonts-ui-20261018'
paras=[]
for family in ['Sans','Serif','FangSong_GB2312','KaiTi_GB2312','SimHei','Liberation Serif','WenQuanYi Zen Hei']:
 for bold in [False,True]:
  font='Source Han '+family+' SC' if family in ['Sans','Serif'] else family;style='<w:b/>' if bold else ''
  paras.append(f'<w:p><w:r><w:rPr><w:rFonts w:ascii="{font}" w:hAnsi="{font}" w:eastAsia="{font}"/>{style}</w:rPr><w:t>思源中文字体测试，公文排版 ABC 123 {family} {bold}</w:t></w:r></w:p>')
xml='<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body>'+''.join(paras)+'<w:sectPr><w:pgSz w:w="11906" w:h="16838"/></w:sectPr></w:body></w:document>'
with zipfile.ZipFile(root/'tests/fixtures/source-build-smoke.docx') as z,zipfile.ZipFile(out/'fonts-test.docx','w') as dest:
 for name in z.namelist():dest.writestr(name,xml if name=='word/document.xml' else z.read(name))
boundary='SourceHanFontsSmoke';body=('--'+boundary+'\r\nContent-Disposition: form-data; name="files"; filename="fonts.docx"\r\nContent-Type: application/vnd.openxmlformats-officedocument.wordprocessingml.document\r\n\r\n').encode()+(out/'fonts-test.docx').read_bytes()+('\r\n--'+boundary+'--\r\n').encode()
resp=urllib.request.urlopen(urllib.request.Request('http://127.0.0.1:13000/forms/libreoffice/convert',data=body,headers={'Content-Type':'multipart/form-data; boundary='+boundary}),timeout=90)
pdf=resp.read();assert resp.status==200 and pdf.startswith(b'%PDF-');(out/'fonts-test.pdf').write_bytes(pdf)
fonts=subprocess.check_output(['pdffonts',str(out/'fonts-test.pdf')]).decode()
assert all(x in fonts for x in ['SourceHanSansSC-Regular','SourceHanSansSC-Bold','SourceHanSerifSC-Regular','SourceHanSerifSC-Bold','FangSong_GB2312','KaiTi_GB2312','SimHei']),fonts
(out/'pdf-fonts.txt').write_text(fonts);print(fonts)

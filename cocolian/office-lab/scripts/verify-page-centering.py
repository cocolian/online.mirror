"""Isolated mixed-orientation Writer layout and save regression."""
import os,json,secrets,subprocess,time,shutil,zipfile
from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path('/home/codex/projects/cocolian-office');out=root/'artifacts/page-center-20261017';out.mkdir(exist_ok=True);data=out/('data-'+secrets.token_hex(4));data.mkdir();source=Path('/home/codex/projects/cocolian/apps/office-portal');key=secrets.token_urlsafe(32)
for p in source.iterdir():
 if p.is_file():shutil.copy2(p,out/p.name)
s=(source/'server.py').read_text().replace("Path('/data')","Path(os.environ['SOURCE_TEST_DATA'])").replace('http://collabora:9980','http://127.0.0.1:19980').replace("src=url.path+","src=urllib.parse.urlunsplit((url.scheme,url.netloc,url.path,'',''))+").replace('http://wopi:8080','http://127.0.0.1:18080').replace("('0.0.0.0',8000)","('127.0.0.1',18080)");(out/'host.py').write_text(s);(out/'editor.json').write_text(json.dumps({'language':'zh-CN','ui_mode':'tabbed','use_saved_ui_state':False}))
def section(w,h):return f'<w:sectPr><w:type w:val="nextPage"/><w:pgSz w:w="{w}" w:h="{h}"/><w:pgMar w:top="1440" w:right="1440" w:bottom="1440" w:left="1440"/></w:sectPr>'
xml='<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:r><w:t>PORTRAIT_ONE</w:t></w:r></w:p><w:p><w:pPr>'+section(11906,16838)+'</w:pPr></w:p><w:p><w:r><w:t>LANDSCAPE_TWO</w:t></w:r></w:p><w:p><w:pPr>'+section(16838,11906)+'</w:pPr></w:p><w:p><w:r><w:t>PORTRAIT_THREE</w:t></w:r></w:p>'+section(11906,16838)+'</w:body></w:document>'
with zipfile.ZipFile(root/'tests/fixtures/source-build-smoke.docx') as original,zipfile.ZipFile(data/'collaboration.docx','w') as archive:
 for name in original.namelist():archive.writestr(name,xml if name=='word/document.xml' else original.read(name))
env=dict(os.environ,LAB_KEY=key,SOURCE_TEST_DATA=str(data),PUBLIC_ORIGIN='http://127.0.0.1:18080',NO_PROXY='127.0.0.1,localhost',no_proxy='127.0.0.1,localhost');log=(out/'host.log').open('w');host=subprocess.Popen(['python3',str(out/'host.py')],env=env,stdout=log,stderr=log);result={}
try:
 time.sleep(.5)
 with sync_playwright() as p:
  b=p.chromium.launch(channel='chrome',headless=True,chromium_sandbox=True);c=b.new_context(device_scale_factor=float(os.environ.get('CENTER_DPR','1')),viewport={'width':1800,'height':1000});assert c.request.post('http://127.0.0.1:18080/login',form={'key':key}).ok
  page=c.new_page();page.goto('http://127.0.0.1:18080/edit?user=Alice');f=page.frame_locator('iframe[name=editor]');f.locator('#document-canvas').wait_for(state='visible',timeout=90000)
  try:f.get_by_role('button',name='跳过',exact=True).click(timeout=1500)
  except Exception:pass
  frame=page.frames[1];frame.wait_for_function('app.file.writer.pageRectangleList.length>=3')
  print('geometry',frame.evaluate('({pages:app.file.writer.pageRectangleList,zoom:app.map.getZoom(),file:app.activeDocument.fileSize.pX})'),flush=True)
  def check(index):
   frame.evaluate('i=>{const l=app.activeDocument.activeLayout;l.scrollTo(l.viewedRectangle.pX1,app.file.writer.pageRectangleList[i][1]*app.twipsToPixels)}',index);page.wait_for_timeout(250)
   state=frame.evaluate('''i=>{const l=app.activeDocument.activeLayout,p=app.file.writer.pageRectangleList[i],w=p[2]*app.twipsToPixels,left=p[0]*app.twipsToPixels-l.viewedRectangle.pX1;return {page:i,width:w,frame:l.frameSize.pX,left,delta:left+w/2-l.frameSize.pX/2}}''',index)
   if state['width']<=state['frame']:assert abs(state['delta'])<=2,state
   return state
  result['initial']=[check(i) for i in [0,1,2,0]];page.screenshot(path=str(out/'centered.png'))
  z=frame.evaluate('app.map.getZoom()')
  for delta in [-1,1]:
   frame.evaluate('v=>app.zoomControl.zoomTo(v,undefined,false)',z+delta);page.wait_for_timeout(400);result['zoom_'+str(delta)]=[check(i) for i in [0,1,2]]
  check(1);frame.evaluate('app.activeDocument.activeLayout.scroll(100,0)');result['return_from_horizontal_pan']=check(2)
  frame.evaluate('v=>app.zoomControl.zoomTo(v,undefined,false)',z);page.wait_for_timeout(400)
  frame.evaluate('app.map.uiManager.showStyleListDeck()');f.locator('.StyleListPanel').wait_for();page.wait_for_timeout(400);result['sidebar']=[check(i) for i in [0,1,2]]
  f.locator('.StyleListPanel .ui-panel-close-button').click();frame.evaluate("app.map.sendUnoCommand('.uno:SidebarHide')");frame.wait_for_function('!app.map.sidebar.isVisible()');page.wait_for_timeout(400);result['sidebar_closed']=check(0)
  assert result['sidebar_closed']['frame']>result['sidebar'][0]['frame']
  page.screenshot(path=str(out/'centered-no-sidebar.png'))
  for width in [1440,1024,1800]:
   page.set_viewport_size({'width':width,'height':1000});page.wait_for_timeout(300);result['width_'+str(width)]=[check(i) for i in [0,1,2]]
  check(0)
  position=frame.evaluate('''()=>{const l=app.activeDocument.activeLayout,p=app.file.writer.pageRectangleList[0],pt={pX:(p[0]+1800)*app.twipsToPixels,pY:(p[1]+1600)*app.twipsToPixels};return {x:l.documentToViewX(pt)/app.dpiScale,y:l.documentToViewY(pt)/app.dpiScale}}''')
  f.locator('#document-canvas').click(position=position);marker='CENTER_SAVE_'+secrets.token_hex(4);page.keyboard.insert_text(marker);page.keyboard.press('Control+s')
  for _ in range(60):
   with zipfile.ZipFile(data/'collaboration.docx') as archive:document=archive.read('word/document.xml').decode()
   if marker in document:break
   time.sleep(1)
  else:raise AssertionError('Click and WOPI save failed')
  result['click_and_wopi_save']=True;b.close()
finally:
 host.terminate();host.wait(timeout=10);log.close();(out/('verification-dpr'+os.environ.get('CENTER_DPR','1')+'.json')).write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))

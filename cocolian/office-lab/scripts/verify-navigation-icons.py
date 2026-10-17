"""Isolated mixed-orientation Writer layout and save regression."""
import os,json,secrets,subprocess,time,shutil,zipfile
from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path('/home/codex/projects/cocolian-office');out=root/'artifacts/nav-icons-20261017';out.mkdir(exist_ok=True);data=out/('data-'+secrets.token_hex(4));data.mkdir();source=Path('/home/codex/projects/cocolian/apps/office-portal');key=secrets.token_urlsafe(32)
for p in source.iterdir():
 if p.is_file():shutil.copy2(p,out/p.name)
s=(source/'server.py').read_text().replace("Path('/data')","Path(os.environ['SOURCE_TEST_DATA'])").replace('http://collabora:9980','http://127.0.0.1:19980').replace("src=url.path+","src=urllib.parse.urlunsplit((url.scheme,url.netloc,url.path,'',''))+").replace('http://wopi:8080','http://127.0.0.1:18080').replace("('0.0.0.0',8000)","('127.0.0.1',18080)");(out/'host.py').write_text(s);(out/'editor.json').write_text(json.dumps({'language':'zh-CN','ui_mode':'tabbed','use_saved_ui_state':False}))
def section(w,h):return f'<w:sectPr><w:type w:val="nextPage"/><w:pgSz w:w="{w}" w:h="{h}"/><w:pgMar w:top="1440" w:right="1440" w:bottom="1440" w:left="1440"/></w:sectPr>'
xml='<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body>'+''.join('<w:p><w:pPr><w:outlineLvl w:val="0"/></w:pPr><w:r><w:t>'+t+'</w:t></w:r></w:p><w:p><w:r><w:t>Navigation search sample content.</w:t></w:r></w:p>' for t in ['第一章 测试标题','第二章 导航验证'])+section(11906,16838)+'</w:body></w:document>'

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
  frame=page.frames[1]
  frame.evaluate("app.socket.sendMessage('uno .uno:Navigator?Navigator:bool=true')")
  panel=f.locator('.office-writer-navigation');panel.wait_for(timeout=20000)
  tabs=panel.locator('[role=tab]');assert tabs.count()==2
  assert not panel.locator('.navigation-title').is_visible()
  for tab in tabs.all():
   assert tab.inner_text()==''
   assert tab.get_attribute('aria-label')
   assert tab.locator('svg').count()==1
  assert panel.locator('.navigation-header [role=tab]').count()==2
  f.locator('#tab-quick-find').click();assert f.locator('#tab-quick-find').get_attribute('aria-selected')=='true'
  f.locator('#tab-navigator').click();assert f.locator('#tab-navigator').get_attribute('aria-selected')=='true'
  f.locator('#tab-navigator').focus();page.keyboard.press('ArrowRight');page.wait_for_timeout(200)
  result['keyboard_focus']=frame.evaluate('document.activeElement.id')
  f.locator('#tab-navigator').click()
  search=f.locator('#navigator-search-input');search.fill('Navigation');search.press('Enter');page.wait_for_timeout(1500)
  result['search_tab_selected']=f.locator('#tab-quick-find').get_attribute('aria-selected')
  page.screenshot(path=str(out/'navigation-search.png'))
  search.fill('');search.press('Enter');f.locator('#tab-navigator').click();page.wait_for_timeout(400)
  result['outline_text']=panel.locator('#NavigatorPanel').inner_text()
  assert '第一章' in result['outline_text']
  panel.get_by_text('第二章 导航验证',exact=True).dblclick();page.wait_for_timeout(300)
  page.screenshot(path=str(out/'navigation-outline.png'))
  panel.locator('.close-navigation-button').click();panel.wait_for(state='hidden',timeout=10000)
  result['icon_tabs_search_outline_close']=True
  f.locator('#document-canvas').click(position={'x':400,'y':300});marker='NAV_SAVE_'+secrets.token_hex(4);page.keyboard.insert_text(marker);page.keyboard.press('Control+s')
  for _ in range(60):
   with zipfile.ZipFile(data/'collaboration.docx') as archive:document=archive.read('word/document.xml').decode()
   if marker in document:break
   time.sleep(1)
  else:raise AssertionError('Click and WOPI save failed')
  result['click_and_wopi_save']=True;b.close()
finally:
 host.terminate();host.wait(timeout=10);log.close();(out/'verification.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))

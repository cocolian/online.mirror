"""Isolated mixed-orientation Writer layout and save regression."""
import os,json,secrets,subprocess,time,shutil,zipfile
from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path('/home/codex/projects/cocolian-office');out=root/'artifacts/font-groups-20261018';out.mkdir(exist_ok=True);data=out/('data-'+secrets.token_hex(4));data.mkdir();source=Path('/home/codex/projects/cocolian/apps/office-portal');key=secrets.token_urlsafe(32)
for p in source.iterdir():
 if p.is_file():shutil.copy2(p,out/p.name)
s=(source/'server.py').read_text().replace("Path('/data')","Path(os.environ['SOURCE_TEST_DATA'])").replace('http://collabora:9980','http://127.0.0.1:19980').replace("src=url.path+","src=urllib.parse.urlunsplit((url.scheme,url.netloc,url.path,'',''))+").replace('http://wopi:8080','http://127.0.0.1:18080').replace("('0.0.0.0',8000)","('127.0.0.1',18080)");(out/'host.py').write_text(s);(out/'editor.json').write_text(json.dumps({'language':'zh-CN','ui_mode':os.environ.get('FONT_UI_MODE','tabbed'),'use_saved_ui_state':False}))
def section(w,h):return f'<w:sectPr><w:type w:val="nextPage"/><w:pgSz w:w="{w}" w:h="{h}"/><w:pgMar w:top="1440" w:right="1440" w:bottom="1440" w:left="1440"/></w:sectPr>'
xml='<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body>'+''.join('<w:p><w:pPr><w:outlineLvl w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Source Han Serif SC" w:eastAsia="Source Han Serif SC" w:hAnsi="Source Han Serif SC"/></w:rPr><w:t>'+t+'</w:t></w:r></w:p><w:p><w:r><w:t>Navigation search sample content.</w:t></w:r></w:p>' for t in ['第一章 测试标题','第二章 导航验证'])+section(11906,16838)+'</w:body></w:document>'

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
  if os.environ.get('FONT_UI_MODE')=='compact':
   frame.evaluate("app.map.uiManager.onChangeUIMode({mode:'classic'})")
   frame.wait_for_function("app.map.uiManager.getCurrentMode()==='classic'")
  f.locator('#fontnamecombobox .ui-combobox-button').first.click()
  fontRows=f.locator('.office-font-entry');fontRows.first.wait_for()
  names=fontRows.all_inner_texts();result['font_names']=names
  assert names[:2]==['仿宋_GB2312','楷体_GB2312'],names[:6]
  groups=f.locator('.office-font-group');result['groups']=groups.all_inner_texts()
  assert len(result['groups'])>=9,result['groups']
  ids=fontRows.evaluate_all('rows=>rows.map(e=>Number(e.dataset.fontGroup))');assert ids==sorted(ids)
  assert any('C059（世纪' in x for x in names) and any('P052（帕拉' in x for x in names) and any('Z003（西文' in x for x in names)
  assert any(x in names for x in ['仿宋_GB2312','FangSong_GB2312'])
  assert any(x in names for x in ['楷体_GB2312','KaiTi_GB2312'])
  assert any(x in names for x in ['黑体','SimHei'])
  assert any('Liberation' in x for x in names)
  assert any('WenQuanYi' in x or '文泉驿' in x for x in names)
  assert fontRows.locator('img').count()==0
  assert fontRows.first.evaluate('e=>getComputedStyle(e).fontSize')=='13px'
  page.wait_for_timeout(350)
  assert fontRows.first.bounding_box()['height']<=30
  result['ui_mode']=frame.evaluate('app.map.uiManager.getCurrentMode()')
  expectedMode='classic' if os.environ.get('FONT_UI_MODE')=='compact' else 'notebookbar'
  assert result['ui_mode']==expectedMode,result['ui_mode']
  page.screenshot(path=str(out/'font-list.png'))
  filterInput=f.locator('.ui-combobox-search-input')
  for term in ['仿宋','FangSong','世纪','C059']:
   filterInput.fill(term);visible=f.locator('.office-font-entry:not(.hidden)');assert visible.count()==1
   assert f.locator('.office-font-group:not(.hidden)').count()==1
  filterInput.fill('NotAFont999');assert f.locator('.office-font-entry:not(.hidden)').count()==0 and f.locator('.office-font-group:not(.hidden)').count()==0
  filterInput.fill('楷体');filterInput.press('ArrowDown');page.keyboard.press('Enter')
  frame.wait_for_function("['KaiTi_GB2312','楷体_GB2312'].includes(app.map.stateChangeHandler.getItemValue('.uno:CharFontName'))")
  result['bilingual_filter_and_keyboard']=True
  for choices in [['仿宋_GB2312','FangSong_GB2312'],['楷体_GB2312','KaiTi_GB2312'],['黑体','SimHei']]:
   name=next(x for x in choices if x in names)
   f.locator('#fontnamecombobox .ui-combobox-button').first.click()
   f.locator('.office-font-entry').filter(has_text=name).first.click()
   page.wait_for_timeout(250)
   assert f.locator('#fontnamecombobox input').first.input_value() in choices
   frame.wait_for_function("v=>v.includes(app.map.stateChangeHandler.getItemValue('.uno:CharFontName'))",arg=choices,timeout=10000)
  result['font_selection']=True
  fontInput=f.locator('#fontnamecombobox input').first
  fontInput.fill('C059（世纪教科书风格）');fontInput.press('Enter')
  frame.wait_for_function("app.map.stateChangeHandler.getItemValue('.uno:CharFontName')==='C059'")
  fontInput.fill('黑体');fontInput.press('Enter')
  frame.wait_for_function("app.map.stateChangeHandler.getItemValue('.uno:CharFontName')==='SimHei'")
  assert not frame.evaluate("document.body.classList.contains('current-font-missing')")
  result['typed_alias_resolves_to_engine_font']=True
  frame.evaluate("app.socket.sendMessage('uno .uno:Navigator?Navigator:bool=true')")
  panel=f.locator('.office-writer-navigation');panel.wait_for(timeout=20000)
  tabs=panel.locator('[role=tab]');assert tabs.count()==2
  assert not panel.locator('.navigation-title').is_visible()
  for tab in tabs.all():
   assert tab.inner_text()==''
   assert tab.get_attribute('aria-label')
   assert tab.locator('svg').count()==1
  assert panel.locator('.navigation-header [role=tab]').count()==2
  result['icon_metrics']=panel.locator('#tab-navigator').evaluate('''e=>{const s=getComputedStyle(e.querySelector('svg'));return {width:e.getBoundingClientRect().width,iconWidth:s.width,stroke:s.strokeWidth,header:getComputedStyle(e.closest('.navigation-header')).height}}''')
  assert result['icon_metrics']=={'width':44,'iconWidth':'20px','stroke':'1.25px','header':'36px'},result['icon_metrics']
  assert not f.locator('#navigator-search-input').is_visible()
  f.locator('#tab-quick-find').click();assert f.locator('#tab-quick-find').get_attribute('aria-selected')=='true'
  f.locator('#tab-navigator').click();assert f.locator('#tab-navigator').get_attribute('aria-selected')=='true'
  f.locator('#tab-navigator').focus();page.keyboard.press('ArrowRight');page.wait_for_timeout(200)
  result['keyboard_focus']=frame.evaluate('document.activeElement.id')
  f.locator('#tab-navigator').click()
  f.locator('#tab-quick-find').click()
  search=f.locator('#navigator-search-input');assert search.is_visible();search.fill('Navigation');search.press('Enter');page.wait_for_timeout(1500)
  result['search_tab_selected']=f.locator('#tab-quick-find').get_attribute('aria-selected')
  page.screenshot(path=str(out/'navigation-search.png'))
  search.fill('');search.press('Enter');f.locator('#tab-navigator').click();page.wait_for_timeout(400)
  assert not search.is_visible()
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

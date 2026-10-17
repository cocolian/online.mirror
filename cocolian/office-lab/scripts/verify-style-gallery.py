import os,json,secrets,subprocess,time,shutil,socket
from pathlib import Path
from playwright.sync_api import sync_playwright
r=Path('/home/codex/projects/cocolian-office');out=r/'artifacts/style-gallery-20261017';out.mkdir(exist_ok=True)
key=secrets.token_urlsafe(32);result={}
with sync_playwright() as p:
 b=p.chromium.launch(channel='chrome',headless=True,chromium_sandbox=True)
 for mode in ['tabbed']:
  d=out/mode;d.mkdir(exist_ok=True);(d/'host.py').write_text((Path('/home/codex/projects/cocolian/apps/office-portal/server.py')).read_text().replace("Path('/data')","Path(os.environ['SOURCE_TEST_DATA'])").replace('http://collabora:9980','http://127.0.0.1:19980').replace("src=url.path+","src=urllib.parse.urlunsplit((url.scheme,url.netloc,url.path,'',''))+").replace('http://wopi:8080','http://127.0.0.1:18080').replace("('0.0.0.0',8000)","('127.0.0.1',18080)"));shutil.copy2(r/'tests/fixtures/source-build-smoke.docx',d/'collaboration.docx');(d/'editor.json').write_text(json.dumps({'language':'zh-CN','ui_mode':mode,'use_saved_ui_state':False}))
  env=dict(os.environ,LAB_KEY=key,SOURCE_TEST_DATA=str(d),NO_PROXY='127.0.0.1,localhost',no_proxy='127.0.0.1,localhost');log=(d/'host.log').open('w');host=subprocess.Popen(['python3',str(d/'host.py')],env=env,stdout=log,stderr=log)
  try:
   for _ in range(60):
    try:
     with socket.create_connection(('127.0.0.1',18080),timeout=.2):break
    except OSError:time.sleep(.1)
   c=b.new_context(viewport={'width':1800,'height':1000});assert c.request.post('http://127.0.0.1:18080/login',form={'key':key}).ok
   page=c.new_page();page.goto('http://127.0.0.1:18080/edit?user=Alice');f=page.frame_locator('iframe[name=editor]');f.locator('#document-canvas').wait_for(state='visible',timeout=90000)
   try:f.get_by_role('button',name='跳过',exact=True).click(timeout=1500)
   except Exception:pass
   frame=page.frames[1];indicator=frame.locator('#office-net-status');print('initial',frame.evaluate("()=>({indicator:!!document.getElementById('office-net-status'),nav:!!document.querySelector('.main-nav'),timer:app.socket._netTimer,connected:app.socket.connected(),loaded:app.socket._map._docLoaded})"),flush=True);page.screenshot(path=str(out/'initial.png'));indicator.wait_for(state='visible',timeout=15000)
   frame.evaluate("app.map.uiManager.showStyleListDeck()")
   frame.locator('.StyleListPanel #treeview').wait_for(timeout=15000)
   page.wait_for_timeout(2500)
   (out/'sidebar-dom.html').write_text(frame.locator('.StyleListPanel').evaluate('(e)=>e.outerHTML'))
   page.screenshot(path=str(out/'preview.png'))
   frame.locator('#stylesview_2').click()
   page.wait_for_timeout(750)
   frame.wait_for_function("app.map.stateChangeHandler.getItemValue('.uno:StyleApply')==='标题 1'")
   expander=frame.locator('.StyleListPanel #treeview_1 .ui-treeview-expander')
   if frame.locator('.StyleListPanel #treeview_1').get_attribute('aria-expanded')=='false':expander.click()
   frame.locator('.StyleListPanel #treeview_4').dblclick()
   frame.wait_for_function("app.map.stateChangeHandler.getItemValue('.uno:StyleApply')==='标题 2'")
   frame.locator('.StyleListPanel #filter-input').select_option('3')
   page.wait_for_timeout(750)
   assert frame.locator('.StyleListPanel #filter-input').input_value()=='3'
   rows=frame.locator('.StyleListPanel .ui-treeview-entry:visible')
   assert rows.count()>0
   row=rows.first.bounding_box();assert 32<=row['height']<=40,row
   checkbox=frame.get_by_role('checkbox',name='显示预览',exact=True)
   checkbox.uncheck();page.wait_for_timeout(500);assert not checkbox.is_checked()
   checkbox.check();page.wait_for_timeout(500);assert checkbox.is_checked()
   result[mode]={'gallery_applies_heading1':True,'sidebar_applies_heading2':True,'hierarchy_expands':True,'used_filter':True,'preview_toggle':True}
   for width in [1440,1024]:
    page.set_viewport_size({'width':width,'height':1000});page.wait_for_timeout(400)
    page.screenshot(path=str(out/('styles-'+str(width)+'.png')))
   c.close()
  finally:host.terminate();host.wait(timeout=10);log.close()
 b.close()
(out/'monitor-tests.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n');print(json.dumps(result,ensure_ascii=False))

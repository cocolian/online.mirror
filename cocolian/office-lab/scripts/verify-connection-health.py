import os,json,secrets,subprocess,time,shutil,socket
from pathlib import Path
from playwright.sync_api import sync_playwright
r=Path('/home/codex/projects/cocolian-office');out=r/'artifacts/net-status-20261017';out.mkdir(exist_ok=True)
fixture_doc=r/'tests/fixtures/source-build-smoke.docx'
if not fixture_doc.exists():fixture_doc=Path(__file__).resolve().parents[1]/'tests/fixtures/network-smoke.docx'
key=secrets.token_urlsafe(32);result={}
with sync_playwright() as p:
 b=p.chromium.launch(channel='chrome',headless=True,chromium_sandbox=True)
 for mode in ['tabbed','compact']:
  d=out/mode;d.mkdir(exist_ok=True);(d/'host.py').write_text((Path('/home/codex/projects/cocolian/apps/office-portal/server.py')).read_text().replace("Path('/data')","Path(os.environ['SOURCE_TEST_DATA'])").replace('http://collabora:9980','http://127.0.0.1:19980').replace("src=url.path+","src=urllib.parse.urlunsplit((url.scheme,url.netloc,url.path,'',''))+").replace('http://wopi:8080','http://127.0.0.1:18080').replace("('0.0.0.0',8000)","('127.0.0.1',18080)"));shutil.copy2(fixture_doc,d/'collaboration.docx');(d/'editor.json').write_text(json.dumps({'language':'zh-CN','ui_mode':mode,'use_saved_ui_state':False}))
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
   frame.wait_for_function("document.getElementById('office-net-status').dataset.level==='good'",timeout=30000)
   actual=frame.evaluate('app.socket._netSamples.map(x=>x.ms)');assert len(actual)>=3 and all(0<x<1000 for x in actual),actual
   page.screenshot(path=str(out/(mode+'-good.png')),clip={'x':0,'y':0,'width':1800,'height':140})
   indicator.click();assert frame.locator('#office-net-details').is_visible();assert '最近 30 秒' in frame.locator('#office-net-details').inner_text()
   result[mode]={'actual_ping_rtts_ms':actual,'good_label':indicator.inner_text()}
   if mode=='tabbed':
    frame.evaluate('app.socket.enableMessageDelay(250)')
    frame.wait_for_function("document.getElementById('office-net-status').dataset.level==='poor'",timeout=30000)
    delayed=frame.evaluate('app.socket._netSamples.slice(-3).map(x=>x.ms)')
    assert all(450<x<2000 for x in delayed),delayed
    result[mode]['injected_250ms_each_direction_rtts']=delayed
    frame.evaluate('app.socket.disableMessageDelay()')
    frame.wait_for_function("document.getElementById('office-net-status').dataset.level==='good'",timeout=25000)
   frame.evaluate('clearInterval(app.socket._netTimer)')
   def samples(values):
    return frame.evaluate('''values=>{const s=app.socket;for(const ms of values){s._netPending={id:'test-'+(++s._netSequence),sent:performance.now()-ms};s._netPong('pong rendercount=0 netprobe='+s._netPending.id)}return document.getElementById('office-net-status').dataset.level}''',values)
   assert samples([700,800])=='good','Transient samples changed state'
   assert samples([900])=='poor','Persistent high RTT not shown'
   page.screenshot(path=str(out/(mode+'-poor.png')),clip={'x':0,'y':0,'width':1800,'height':250})
   assert samples([40,45])=='poor','Recovered too soon'
   assert samples([50])=='good'
   assert samples([200,500,250])=='slow','Alternating high latency was missed'
   # Save feedback hooks stay independent of latency.
   for method,state in [('showModifiedStatus','modified'),('showSavingStatus','saving'),('showSavedStatus','saved'),('showSaveFailedStatus','failed')]:
    frame.evaluate('(name)=>app.socket._map.saveState[name]()',method)
    assert indicator.get_attribute('data-save-state')==state,(method,indicator.get_attribute('data-save-state'))
   frame.evaluate("()=>{app.socket._netPending={id:'expired',sent:performance.now()-16000};const send=app.socket._doSend;app.socket._doSend=()=>{};app.socket._netTick();app.socket._doSend=send;}")
   assert indicator.get_attribute('data-level')=='waiting';assert '保存状态待确认' in indicator.get_attribute('title')
   pending=frame.evaluate('app.socket._netPending.id');frame.evaluate("app.socket._netPong('pong rendercount=0 netprobe=stale')");assert frame.evaluate('app.socket._netPending.id')==pending
   assert samples([30,35,40])=='good'
   # Verify actual close notification, without touching any production connection.
   frame.evaluate('app.socket.socket.close()');frame.wait_for_function("['offline','paused'].includes(document.getElementById('office-net-status').dataset.level)",timeout=5000)
   result[mode].update({'latency_hysteresis':True,'save_hooks':True,'timeout_and_stale_reply':True,'disconnect':indicator.inner_text()})
   c.close()
  finally:host.terminate();host.wait(timeout=10);log.close()
 b.close()
(out/'monitor-tests.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n');print(json.dumps(result,ensure_ascii=False))

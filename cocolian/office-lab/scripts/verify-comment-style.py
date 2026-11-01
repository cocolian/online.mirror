"""Exercise native Writer comments on disposable documents, never customer files.

LAB_KEY supplies the existing portal login. --candidate points to the local
comment-style work directory before the native browser bundle is released.
"""
import argparse, io, json, os, time, zipfile
from pathlib import Path
from playwright.sync_api import sync_playwright, expect

parser=argparse.ArgumentParser()
parser.add_argument('--base',default='https://office.cocolian.cn')
parser.add_argument('--candidate',type=Path)
parser.add_argument('--output',type=Path,default=Path('comment-style-validation'))
args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
key=os.environ['LAB_KEY'];base=args.base.rstrip('/')
buf=io.BytesIO()
with zipfile.ZipFile(buf,'w') as z:
 z.writestr('[Content_Types].xml','<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="xml" ContentType="application/xml"/><Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/></Types>')
 z.writestr('_rels/.rels','<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/></Relationships>')
 z.writestr('word/document.xml','<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:r><w:t>OpenWen comment review.</w:t></w:r></w:p><w:p><w:r><w:t>This paragraph keeps the original document content.</w:t></w:r></w:p><w:sectPr/></w:body></w:document>')

with sync_playwright() as p:
 browser=p.chromium.launch(channel='chrome',headless=True)
 context=browser.new_context(viewport={'width':1440,'height':1000})
 assert context.request.post(base+'/login',form={'key':key}).ok
 upload=context.request.post(base+'/api/files?name=comment-style-'+str(int(time.time()))+'.docx',data=buf.getvalue(),headers={'Content-Type':'application/octet-stream'})
 assert upload.status==201
 fid=upload.json()['id'];page=context.new_page()
 page.goto(base+'/edit?user=Alice&file='+fid)
 frame=page.frame(name='editor')
 frame.wait_for_function('!!(window.app?.map?._docLayer && window.app?.definitions?.Comment)',timeout=90000)
 page.wait_for_timeout(2500)
 if frame.get_by_text('跳过',exact=True).count():frame.get_by_text('跳过',exact=True).click()
 if args.candidate:
  methods=json.loads((args.candidate/'candidate-methods.json').read_text())
  frame.evaluate('''methods=>{
   const Comment=app.definitions.Comment,p=Comment.prototype;
   for(const [name,source] of Object.entries(methods))p[name]=new Function('Comment','return '+source)(Comment);
   for(const name of ['createContainerAndWrapper','updateEditability','updateResolvedField']){
    const original=p[name];p[name]=function(...a){const result=original.apply(this,a);if(name==='createContainerAndWrapper'&&app.map._docLayer._docType==='text'&&!this.sectionProperties.data.trackchange)this.sectionProperties.container.classList.add('writer-comment');if(name!=='createContainerAndWrapper')this.updateResolveButton();return result;};
   }
   const old=app.map.uiManager.showCommand;app.map.uiManager.showCommand=function(command,show){old.call(this,command,show);if(command==='.uno:ResolveComment')document.querySelectorAll('.cool-annotation-resolve').forEach(e=>e.classList.toggle('hidden-by-command',!show));};
  }''',methods)
  css=(args.candidate/'online/browser/css/cool.css').read_text()
  css=css[css.index('/* Writer margin comments'):css.index('.cool-annotation-child-line {')]
  page.route('**/comment-style-test.css',lambda route:route.fulfill(body=css,content_type='text/css'))
  frame.add_style_tag(url=base+'/comment-style-test.css')
 frame.evaluate("app.map.sendUnoCommand('.uno:InsertAnnotation',{Text:{type:'string',value:'请检查本段措辞。第二行批注意见保留。'}})")
 card=frame.locator('.writer-comment').first
 expect(card).to_be_visible(timeout=15000)
 ident=card.get_attribute('id').removeprefix('comment-container-')
 date=card.locator('.cool-annotation-date');actions=card.locator('.cool-annotation-menubar')
 tick=card.locator('.cool-annotation-resolve');menu=card.locator('.cool-annotation-menu')
 assert card.locator('.cool-annotation-menu-edit').count()==0
 expect(card.locator('.avatar-img')).to_be_hidden()
 assert tick.count()==1 and menu.count()==1
 frame.evaluate('document.activeElement?.blur()');page.mouse.move(200,200)
 expect(date).to_have_css('opacity','0');expect(actions).to_have_css('opacity','0')
 height=card.bounding_box()['height']
 page.screenshot(path=str(args.output/'comments-idle.png'))
 card.hover();expect(date).to_have_css('opacity','1');expect(actions).to_have_css('opacity','1')
 assert abs(card.bounding_box()['height']-height)<1
 page.screenshot(path=str(args.output/'comments-hover.png'))
 page.mouse.move(200,200);tick.focus();expect(actions).to_have_css('opacity','1')
 menu.focus();menu.press('Enter')
 # Dropdown IDs are generated by the shared native menu implementation.
 entries=frame.locator('.ui-combobox-entry')
 expect(entries.filter(has_text='修改')).to_be_visible(timeout=5000)
 expect(menu).to_have_attribute('aria-expanded','true')
 entries.filter(has_text='修改').hover();expect(actions).to_have_css('opacity','1')
 page.wait_for_timeout(250);page.screenshot(path=str(args.output/'comments-menu.png'))
 entries.filter(has_text='修改').click()
 expect(menu).to_have_attribute('aria-expanded','false')
 editor=frame.locator('#annotation-modify-textarea-'+ident)
 expect(editor).to_be_visible();editor.fill('修改后的批注正文。')
 frame.locator('#annotation-save-'+ident).click()
 expect(card.locator('.cool-annotation-content')).to_contain_text('修改后的批注正文。')
 # The direct button goes through the native UNO command, and can reopen.
 tick.focus();tick.press('Enter')
 frame.wait_for_function("id=>app.sectionContainer.getSectionWithName(app.CSections.CommentList.name).sectionProperties.commentList.find(c=>String(c.sectionProperties.data.id)===id)?.sectionProperties.data.resolved==='true'",arg=ident)
 frame.evaluate("app.map.sendUnoCommand('.uno:ShowResolvedAnnotations')")
 expect(card).to_be_visible(timeout=10000)
 expect(tick).to_have_attribute('aria-pressed','true')
 tick.focus();tick.press('Space')
 frame.wait_for_function("id=>app.sectionContainer.getSectionWithName(app.CSections.CommentList.name).sectionProperties.commentList.find(c=>String(c.sectionProperties.data.id)===id)?.sectionProperties.data.resolved==='false'",arg=ident)
 expect(tick).to_have_attribute('aria-pressed','false')
 # Host command suppression and Viewing mode must not expose a live shortcut.
 frame.evaluate("app.map.uiManager.showCommand('.uno:ResolveComment',false)")
 expect(tick).to_be_hidden()
 frame.evaluate("app.map.uiManager.showCommand('.uno:ResolveComment',true)")
 expect(tick).to_be_visible()
 frame.evaluate("app.map.setPermission('readonly')")
 expect(actions).to_be_hidden()
 frame.evaluate("app.map.setPermission('edit')")
 card.hover();expect(actions).to_be_visible()
 # Reply and delete remain in the menu instead of separate toolbar icons.
 menu.click();entries.filter(has_text='回复').first.click()
 reply=frame.locator('#annotation-reply-textarea-'+ident)
 expect(reply).to_be_visible();reply.fill('回复内容保留。')
 frame.locator('#annotation-reply-'+ident).click()
 expect(frame.locator('.writer-comment .cool-annotation-content').filter(has_text='回复内容保留。')).to_be_visible()
 frame.evaluate('app.map.save(false,false)');page.wait_for_timeout(1800)
 saved=context.request.get(base+'/download?file='+fid)
 with zipfile.ZipFile(io.BytesIO(saved.body())) as z:
  assert 'word/comments.xml' in z.namelist()
  comments=z.read('word/comments.xml').decode()
  assert '修改后的批注正文。' in comments and '回复内容保留。' in comments
 child=frame.locator('.writer-comment').filter(has=frame.locator('.cool-annotation-content').filter(has_text='回复内容保留。')).last
 child_id=child.get_attribute('id')
 child.hover();child.locator('.cool-annotation-menu').click()
 frame.get_by_text('删除',exact=True).click()
 expect(frame.locator('#'+child_id)).to_have_count(0)
 # Touch users do not depend on a hover that their device cannot produce.
 print('PASS: native dropdown keeps actions visible; edit, reply and remove are retained')
 page.reload();frame=page.frame(name='editor');frame.wait_for_function('window.app?.officeAIReview',timeout=90000)
 if args.candidate:
  print('PASS: candidate hover, stable geometry, keyboard resolve/reopen, menu editing/reply, permissions and WOPI save')
 else:
  expect(frame.locator('.writer-comment .cool-annotation-content').filter(has_text='修改后的批注正文。')).to_be_visible(timeout=15000)
  print('PASS: published comment UI persists through reload')
  touch=browser.new_context(viewport={'width':1440,'height':1000},has_touch=True)
  assert touch.request.post(base+'/login',form={'key':key}).ok
  tp=touch.new_page();tp.goto(base+'/edit?user=Bob&file='+fid)
  tf=tp.frame(name='editor');tf.wait_for_function('window.app?.officeAIReview',timeout=90000)
  if tf.get_by_text('跳过',exact=True).count():tf.get_by_text('跳过',exact=True).click()
  tc=tf.locator('.writer-comment').first;expect(tc).to_be_visible(timeout=15000)
  assert tf.evaluate('matchMedia("(hover: none)").matches')
  tf.evaluate('document.activeElement?.blur()')
  expect(tc.locator('.cool-annotation-date')).to_have_css('opacity','1')
  expect(tc.locator('.cool-annotation-menubar')).to_have_css('opacity','1')
  expect(tc.locator('.cool-annotation-resolve')).to_be_visible()
  assert tc.locator('.cool-annotation-menu-edit').count()==0
  touch.close();print('PASS: touch and a second author retain accessible comment actions')
 (args.output/'result.json').write_text(json.dumps({'file':fid,'base':base,'candidate':bool(args.candidate),'verified':True},indent=2))
 context.close();browser.close()

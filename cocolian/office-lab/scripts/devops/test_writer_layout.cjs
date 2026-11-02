/* Exercise the real Writer placement and initial navigation methods. */
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const root = path.resolve(__dirname, '../../../..');
const ts = require(require.resolve('typescript', {paths: [path.join(root, 'browser'), ...module.paths]}));

function setup({dpi=2, mobile=false, preview=false, comments=true, frame=2400, page=1600, pages=[]}={}) {
 const commands=[];
 const commentSection={commentsHiddenOrNotPresent:()=>!comments, calculateAvailableSpace:()=>Math.floor((frame-page)/2),
  sectionProperties:{commentWidth:500}};
 const app={dpiScale:dpi, pixelsToTwips:1, twipsToPixels:1, CSections:{CommentList:{name:'comments'}},
  file:{writer:{pageRectangleList:pages}}, activeDocument:{fileSize:{pX:page,pY:10000}},
  sectionContainer:{getSectionWithName:()=>commentSection}, socket:{sendMessage:c=>commands.push(c)}};
 const context={app,console,Util:{ensureValue:()=>{}},ViewLayoutBase:class {},
  window:{mode:{isSmallScreenDevice:()=>mobile,isDesktop:()=>!mobile,isInteractivePreview:()=>preview},L:{Control:class {}}}};
 vm.createContext(context);
 const source=['app/ViewLayoutWriter.ts','control/Control.UIManager.ts'].map(p=>fs.readFileSync(path.join(root,'browser/src',p),'utf8')).join('\n');
 vm.runInContext(ts.transpileModule(source,{compilerOptions:{target:ts.ScriptTarget.ES2020}}).outputText+
  '\nthis.Writer=ViewLayoutWriter;this.Manager=UIManager;',context);
 const layout=Object.create(context.Writer.prototype);
 Object.assign(layout,{frameSize:{pX:frame,pY:1000},scrollProperties:{viewX:0,viewY:0},commentMarginDirty:true});
 const manager=Object.create(context.Manager.prototype);
 Object.assign(manager,{map:{isText:()=>true},getBooleanDocTypePref:()=>false});
 return {app,layout,manager,commands};
}

test('desktop page stays beside navigation across viewport, zoom and comment changes',()=>{
 for(const dpi of [1,2]) for(const frame of [1200,1600,2400,3400]) for(const page of [800,1100,1800]) for(const comments of [false,true]) {
  const {layout}=setup({dpi,frame,page,comments});
  assert.equal(layout.getCenteringOffset()[0],Math.min(16*dpi,Math.max(0,frame-page)));
  layout.scrollProperties.viewY=6000;
  assert.equal(layout.getCenteringOffset()[0],Math.min(16*dpi,Math.max(0,frame-page)), 'scrolling must not move the page sideways');
 }
});

test('mixed page sizes align the visible page and discard a fitting page residual pan',()=>{
 const {layout}=setup({comments:false,frame:1800,page:2200,pages:[[100,0,1400,2000],[0,2000,2200,1400]]});
 layout.scrollProperties.viewX=120;
 assert.equal(layout.getCenteringOffset()[0]-120+100,32);
 layout.scrollProperties.viewY=2100;
 assert.equal(layout.getCenteringOffset()[0],0);
 // A landscape page is still horizontally pannable when wider than the frame.
 assert.equal(layout.scrollProperties.viewX-layout.getCenteringOffset()[0],120);
});

test('small-screen and interactive previews retain their centered placement',()=>{
 for(const mode of [{mobile:true},{preview:true}]) {
  const {layout}=setup({...mode,comments:false,frame:2400,page:1600});
  assert.equal(layout.getCenteringOffset()[0],400);
 }
});

test('Writer opens navigation even with a saved closed preference; other documents retain it',()=>{
 const {manager,commands}=setup();
 manager.initializeNavigator();manager.initializeNavigator();
 assert.deepEqual(commands,['uno .uno:Navigator?Navigator:bool=true','uno .uno:Navigator?Navigator:bool=true']);
 manager.map.isText=()=>false;commands.length=0;manager.initializeNavigator();assert.equal(commands.length,0);
 manager.getBooleanDocTypePref=()=>true;manager.initializeNavigator();assert.equal(commands.length,1);
 for(const mode of [{mobile:true},{preview:true}]) {const s=setup(mode);s.manager.initializeNavigator();assert.equal(s.commands.length,0);}
});

/* Regression checks for opening Writer with an existing comment margin. */
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(path.resolve(__dirname, '../../../../browser/src/layer/tile/CanvasTileLayer.js'), 'utf8');

function setup({mobile=false, frame=1800, hasComments=true}={}) {
 const extend = properties => { function Layer() {} Layer.prototype=properties; return Layer; };
 let chosen=null;
 const app={dpiScale:2, CSections:{CommentList:{name:'comments'}}, tile:{size:{x:3840}},
  sectionContainer:{getSectionWithName:()=>({sectionProperties:{commentWidth:500}})},
  activeDocument:{partHasComments:hasComments, getScaleZoom:ratio=>10+Math.log(ratio)/Math.log(1.2),
   fileSize:{x:12000}, activeLayout:{frameSize:{pX:frame,cX:frame/2,cY:400},applyZoom:zoom=>{chosen=zoom;}}}};
 const window={L:{Class:{extend},Layer:{extend}},tileSize:256,
  mode:{isSmallScreenDevice:()=>mobile,isTablet:()=>false,isInteractivePreview:()=>false},prefs:{get:()=> 'true'}};
 const context={app,window,RenderManager:{tileSize:256},cool:{Point:class {constructor(x,y){this.x=x;this.y=y;}}},console};
 vm.runInNewContext(source,context);
 const layer=Object.create(window.L.CanvasTileLayer.prototype);
 Object.assign(layer,{isWriter:()=>true,isImpress:()=>false,isCalc:()=>false,isDraw:()=>false,
  _isMultiPageView:()=>false,_documentWidthTwips:12000,_partHasComments:false,
  _getDocumentContainerSize:()=>[2000,800],_viewReset:()=>{},_resetClientVisArea:()=>{},_requestNewTiles:()=>{}});
 return {app,layer,get chosen(){return chosen;}};
}

test('initial zoom uses the document frame, latest comments and a gutter',()=>{
 const state=setup();state.layer.setInitialZoom({});
 const expected=Math.floor(10+Math.log((1800-500-32)/1600)/Math.log(1.2));
 assert.equal(state.chosen,expected);
 assert.equal(state.layer._includedCommentsInFirstFit,true);
});

test('a wide desktop opens at no more than 100 percent',()=>{
 const state=setup({frame:5000});state.layer.setInitialZoom({});assert.equal(state.chosen,10);
});

test('a narrow viewport shrinks instead of putting comments outside the viewport',()=>{
 const state=setup({frame:1450});state.layer.setInitialZoom({});
 const pageWidth=1600*Math.pow(1.2,state.chosen-10);
 assert.ok(pageWidth+500+32<=1450);
});

test('mobile does not reserve a desktop comment column',()=>{
 const state=setup({mobile:true});
 const zoom=state.layer._writerDynamicZoom(800,1600,true);
 assert.ok(Math.abs(zoom-(10+Math.log(.5)/Math.log(1.2)))<1e-9);
});

test('an ordinary post-zoom event preserves a manually selected zoom',()=>{
 const state=setup();state.layer._firstFitDone=true;state.layer._includedCommentsInFirstFit=true;
 state.layer._map={_zoom:12,uiManager:{getStartCompareChanges:()=>false}};
 state.layer._fitWidthZoom();assert.equal(state.chosen,null);assert.equal(state.layer._map._zoom,12);
});

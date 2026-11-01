/* Run the actual TypeScript classes against deterministic document geometry. */
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const root = path.resolve(__dirname, '../../../..');
const ts = require(require.resolve('typescript', {paths: [path.join(root, 'browser'), ...module.paths]}));

function setup() {
 const tasks = [], bounds = {left:250, right:1512, top:135, bottom:675, height:540};
 class Point {
  constructor(x,y) {this.x=x;this.y=y;this.vX=x;this.vY=y;}
  toArray() {return [this.x,this.y];} vToArray() {return [this.vX,this.vY];}
 }
 const app = {definitions:{},dpiScale:2,pixelsToTwips:1,twipsToPixels:1,CSections:{},
  map:{_docLayer:{_docType:'text'},_viewInfoByUserName:{}},
  sectionContainer:{getCanvasBoundingClientRect:()=>({x:0})},
  activeDocument:{fileSize:{cX:1000},activeLayout:{type:'ViewLayoutWriter',viewedRectangle:{cX1:0,pX1:0,pY1:0,pY2:1080},
   ensureViewSizeCoversComments:()=>{},scrollTo:(x,y)=>{app.scrolls.push([x,y]);}}},
  layoutingService:{appendLayoutingTask:fn=>tasks.push(fn)},scrolls:[]};
 const document = {documentElement:{dir:'ltr'},getElementById:id=>id==='document-canvas'
  ? {style:{width:'1512'}} : {getBoundingClientRect:()=>bounds}};
 const context = {app,document,console,CanvasSectionObject:class {},DOMRect:class {},DOMPurify:{addHook:()=>{}},_:s=>s,
  $:element=>({hasClass:name=>element.classList.contains(name),addClass:name=>element.classList.toggle(name,true)}),
  window:{L:{Map:{include:()=>{}},DomUtil:{hasClass:(e,n)=>e.classList.contains(n),removeClass:(e,n)=>e.classList.toggle(n,false)}},mode:{isSmallScreenDevice:()=>false}},
  cool:{CanvasSectionObject:class {},SimplePoint:Point}};
 vm.createContext(context);
 const source = ['CommentSection.ts','CommentListSection.ts'].map(file=>
  fs.readFileSync(path.join(root,'browser/src/canvas/sections',file),'utf8')).join('\n');
 vm.runInContext(ts.transpileModule(source,{compilerOptions:{target:ts.ScriptTarget.ES2020}}).outputText,context);
 const Comment = context.cool.Comment, List = context.cool.CommentSection;
 const list = Object.create(List.prototype);
 Object.assign(list,{myTopLeft:[2000,20],isCollapsed:false,changesImportSignature:null,scrollRequest:0,idIndexMap:new Map(),
  sectionProperties:{commentList:[],show:true,showResolved:false,selectedComment:null,expandedComment:null,
   marginY:40,commentWidth:500,canvasContainerBounds:bounds,deflectionOfSelectedComment:160},
  getComment:id=>list.sectionProperties.commentList.find(c=>c.sectionProperties.data.id===id),
  getIndexOf:id=>list.sectionProperties.commentList.findIndex(c=>c.sectionProperties.data.id===id),
  update:()=>{},orderCommentList:()=>{},checkSize:()=>{},listenForClickOutsideComment:()=>{},
  setThreadPopup:()=>{},resetCommentsSize:()=>{},resizeLastComment:()=>{},resizeComments:()=>{},
  updateChildLines:()=>{},calculateAvailableSpace:()=>500,showArrow:()=>{},hideArrow:()=>{}});
 app.map.getDocType=()=> 'text';list.map=app.map;
 list.containerObject={pauseDrawing:()=>{},resumeDrawing:()=>{},addSection:()=>true,removeSection:()=>{}};
 function card(id,y=0,height=86,extra={}) {
  const c=Object.create(Comment.prototype), classes=new Set();
  const style={left:'',top:'',display:'',visibility:''};
  const container={style,classList:{toggle:(name,on)=>on?classes.add(name):classes.delete(name),contains:name=>classes.has(name)},
   getBoundingClientRect:()=>({left:parseFloat(style.left)||0,top:parseFloat(style.top)||0,
    right:(parseFloat(style.left)||0)+250,bottom:(parseFloat(style.top)||0)+height,height,width:250})};
  Object.assign(c,{pendingInit:false,containerPosX:0,containerPosY:0,renderedPosY:0,positionedHeight:null,
   canvasContainerBounds:{},sectionProperties:{container,commentListSection:list,
    data:{id,parent:'0',anchorPos:[0,y],anchorSPoint:new Point(0,y),...extra}},
   isEdit:()=>false,getCommentHeight:()=>height,getCommentWidth:()=>500,show:()=>{},
   revealSearchSelection:()=>{},isContainerVisible:()=>true});
  return c;
 }
 let editing=false,created=0;
 function FakeComment(name,data) {created++;return card(data.id,0,86,data);}
 FakeComment.isAnyEdit=()=>editing;
 FakeComment.makeName=Comment.makeName;
 context.cool.Comment=FakeComment;
 return {app,list,card,Comment,List,tasks,bounds,get created(){return created;},edit:value=>{editing=value;}};
}
const redline=(index,description='删除 A')=>({index,description,author:'Alice',dateTime:'2026-11-01T08:00:00Z',type:'Delete',textRange:'100,200,50,20',comment:''});

test('periodic unchanged revision reads retain card identity and selection',()=>{
 const s=setup();s.list.importChanges([redline(7)]);
 const card=s.list.getComment('change-7');s.list.sectionProperties.selectedComment=card;
 s.list.sectionProperties.expandedComment=card;
 for(let i=0;i<4;i++)s.list.importChanges([redline(7)]);
 assert.equal(s.created,1);assert.equal(s.list.sectionProperties.selectedComment,card);
 assert.equal(s.app.scrolls.length,0);
});
test('changed revision refresh preserves a selected normal comment without stale active cards',()=>{
 const s=setup(),normal=s.card('1');s.list.sectionProperties.commentList.push(normal);
 s.list.sectionProperties.selectedComment=normal;s.list.sectionProperties.expandedComment=normal;
 s.list.importChanges([redline(7)]);s.list.importChanges([redline(7),redline(8,'插入 B')]);
 assert.equal(s.list.sectionProperties.selectedComment,normal);
 assert.deepEqual(s.list.sectionProperties.commentList.filter(c=>c.sectionProperties.container.classList.contains('annotation-active')).map(c=>c.sectionProperties.data.id),['1']);
 assert.equal(s.app.scrolls.length,0);
});
test('a changed table restores the same revision but never a reused index with different content',()=>{
 const s=setup();s.list.importChanges([redline(7)]);
 const old=s.list.getComment('change-7');s.list.sectionProperties.selectedComment=old;s.list.sectionProperties.expandedComment=old;
 s.list.importChanges([redline(7),redline(8)]);
 assert.equal(s.list.sectionProperties.selectedComment,s.list.getComment('change-7'));
 assert.notEqual(s.list.sectionProperties.selectedComment,old);
 s.list.importChanges([redline(7,'完全不同的修订')]);
 assert.equal(s.list.sectionProperties.selectedComment,null);
});
test('background metadata cannot destroy an edit in progress',()=>{
 const s=setup();s.list.importChanges([redline(7)]);s.edit(true);
 s.list.importChanges([redline(8)]);assert.equal(s.created,1);
 assert.ok(s.list.getComment('change-7'));
});
test('selectById retains the exact reply as the expanded target',()=>{
 const s=setup(),reply=s.card('reply');s.list.sectionProperties.commentList=[reply];
 let chosen;s.list.select=c=>{chosen=c;};s.list.selectById('reply');assert.equal(chosen,reply);
});
test('switching threads clears the old selection before expanding the clicked card',()=>{
 const s=setup(),a=s.card('a'),b=s.card('b'),reply=s.card('reply',0,86,{parent:'b'});
 s.list.sectionProperties.commentList=[a,b,reply];
 s.list.sectionProperties.selectedComment=a;s.list.sectionProperties.expandedComment=a;
 a.sectionProperties.container.classList.toggle('annotation-active',true);
 s.list.select(b);
 assert.equal(s.list.sectionProperties.selectedComment,b);assert.equal(s.list.sectionProperties.expandedComment,b);
 assert.equal(a.sectionProperties.container.classList.contains('annotation-active'),false);
 s.list.selectById('reply');
 assert.equal(s.list.sectionProperties.selectedComment,b);assert.equal(s.list.sectionProperties.expandedComment,reply);
});
test('mixed Writer comments and revisions keep their positions across every selection',()=>{
 const s=setup();s.list.sectionProperties.commentList=[s.card('1',0),s.card('change-7',0,123,{trackchange:true}),s.card('2',20)];
 s.list.layout();const positions=s.list.sectionProperties.commentList.map(c=>[c.containerPosX,c.containerPosY]);
 for(const selected of s.list.sectionProperties.commentList){
  s.list.sectionProperties.selectedComment=selected;s.list.layout();
  assert.deepEqual(s.list.sectionProperties.commentList.map(c=>[c.containerPosX,c.containerPosY]),positions);
 }
 for(let i=1;i<positions.length;i++)assert.ok(positions[i][1]>positions[i-1][1]);
});
test('selected Writer cards stay in their stack and are clipped below the ribbon',()=>{
 const s=setup(),c=s.card('1');s.list.sectionProperties.selectedComment=c;
 c.setContainerPos(true,s.bounds,1000,-60);
 assert.equal(c.renderedPosY,-60);assert.equal(c.sectionProperties.container.style.top,'75px');
 assert.equal(c.sectionProperties.container.style.clipPath,'inset(60px 0px 0px 0px)');
});
test('clicking a visible stacked card does not scroll back to its earlier text anchor',()=>{
 const s=setup(),c=s.card('change-7',-500,100,{trackchange:true});s.list.sectionProperties.commentList=[c];
 s.list.sectionProperties.selectedComment=c;c.setContainerPos(true,s.bounds,1000,200);
 s.list.scrollCommentIntoView(c);s.tasks.forEach(fn=>fn());assert.equal(s.app.scrolls.length,0);
});
test('rapid clicks cancel an older queued scroll target',()=>{
 const s=setup(),a=s.card('a'),b=s.card('b');s.list.sectionProperties.commentList=[a,b];
 a.setContainerPos(true,s.bounds,1000,200);b.setContainerPos(true,s.bounds,1000,700);
 s.list.scrollCommentIntoView(b);s.list.scrollCommentIntoView(a);
 s.tasks.forEach(fn=>fn());assert.equal(s.app.scrolls.length,0);
});

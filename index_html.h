#pragma once
#include <pgmspace.h>
// Web UI embedded in flash (replaces data/index.html + LittleFS upload)
static const char INDEX_HTML[] PROGMEM = R"HTMLPAGE(
<!doctype html>
<html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<title>ESP32 Thermal Camera</title>
<style>
:root{--bg:#0b0e12;--panel:#151a21;--line:#2a323d;--txt:#e6edf3;--mut:#8b98a5;--acc:#ff8a1f;--ok:#3fb950;--bad:#f85149;--cold:#4fc3f7}
*{box-sizing:border-box}
html,body{margin:0;background:var(--bg);color:var(--txt);font:14px/1.4 system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
header{display:flex;justify-content:space-between;align-items:center;padding:10px 14px;border-bottom:1px solid var(--line)}
header h1{font-size:16px;margin:0;font-weight:600}
.pill{display:inline-flex;align-items:center;gap:6px;font-size:12px;color:var(--mut)}
.dot{width:9px;height:9px;border-radius:50%;background:var(--bad)}
.dot.ok{background:var(--ok)}
main{display:grid;grid-template-columns:minmax(0,1fr) 330px;gap:12px;padding:12px;max-width:1200px;margin:0 auto}
@media(max-width:860px){main{grid-template-columns:1fr}}
.card{background:var(--panel);border:1px solid var(--line);border-radius:10px;padding:10px}
.view{position:relative;width:100%;aspect-ratio:4/3;background:#000;border-radius:8px;overflow:hidden;touch-action:none;user-select:none}
.view canvas{position:absolute;inset:0;width:100%;height:100%;display:block}
#img.px{image-rendering:pixelated}
#ov{cursor:crosshair}
.barwrap{margin-top:8px}
#bar{width:100%;height:14px;display:block;border-radius:3px;image-rendering:pixelated}
.scale{display:flex;justify-content:space-between;font-size:12px;color:var(--mut);margin-top:2px}
.stats{display:grid;grid-template-columns:repeat(2,1fr);gap:8px}
.stat{background:#0f141a;border:1px solid var(--line);border-radius:8px;padding:8px}
.stat b{display:block;font-size:11px;color:var(--mut);font-weight:500;letter-spacing:.04em;text-transform:uppercase}
.stat span{font-size:20px;font-weight:600;font-variant-numeric:tabular-nums}
.stat small{display:block;color:var(--mut);font-size:11px}
.stat.wide{grid-column:span 2}
h3{margin:12px 0 6px;font-size:12px;color:var(--mut);text-transform:uppercase;letter-spacing:.06em}
.row{display:flex;gap:8px;align-items:center;margin:6px 0;flex-wrap:wrap}
.row label{color:var(--mut);min-width:76px}
select,input[type=number],button{background:#0f141a;color:var(--txt);border:1px solid var(--line);border-radius:8px;padding:8px 10px;font-size:14px}
input[type=number]{width:80px}
button{cursor:pointer}
button.pri{background:var(--acc);color:#000;border:0;font-weight:600}
button:active{transform:scale(.98)}
.btns{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:8px}
.chk{display:flex;gap:6px;align-items:center;color:var(--txt)}
footer{padding:8px 14px 20px;color:var(--mut);font-size:12px;text-align:center}
footer a{color:var(--acc)}
.spots{font-size:13px;color:var(--mut)}
</style></head>
<body>
<header>
  <h1>ESP32-S3 &middot; MLX90640 Thermal Camera</h1>
  <div class="pill"><span class="dot" id="dotConn"></span><span id="txtConn">connecting</span>&nbsp;&middot;&nbsp;<span class="dot" id="dotSens"></span><span id="txtSens">sensor</span></div>
</header>
<main>
  <section class="card">
    <div class="view" id="view">
      <canvas id="img"></canvas>
      <canvas id="ov"></canvas>
    </div>
    <div class="barwrap">
      <canvas id="bar" width="256" height="1"></canvas>
      <div class="scale"><span id="scLo">--</span><span id="scMid"></span><span id="scHi">--</span></div>
    </div>
  </section>

  <aside class="card">
    <div class="stats">
      <div class="stat"><b>Max</b><span id="sMax">--</span></div>
      <div class="stat"><b>Min</b><span id="sMin">--</span></div>
      <div class="stat"><b>Average (768 px)</b><span id="sAvg">--</span></div>
      <div class="stat"><b>Centre (16,12)</b><span id="sCtr">--</span></div>
      <div class="stat wide"><b>Hotspot</b><span id="sHot" style="color:#ff8a1f">--</span><small id="sHotXY"></small></div>
      <div class="stat wide"><b>Coldspot</b><span id="sCold" style="color:var(--cold)">--</span><small id="sColdXY"></small></div>
      <div class="stat wide"><b>Cursor / spots (raw sensor pixels)</b><span id="sCur" style="font-size:15px">move over image, tap to add spot</span><small class="spots" id="sSpots"></small></div>
    </div>

    <h3>Display</h3>
    <div class="row"><label for="pal">Palette</label>
      <select id="pal"><option value="ironbow">Ironbow</option><option value="inferno">Inferno</option></select></div>
    <div class="row"><label for="interp">Interpolation</label>
      <select id="interp"><option value="1">Off (raw pixels)</option><option value="4">4&times;</option><option value="8" selected>8&times;</option></select></div>
    <div class="row"><label for="mode">Scale</label>
      <select id="mode"><option value="auto">Auto</option><option value="manual">Manual</option></select>
      <input type="number" id="mn" value="20" step="1"> <input type="number" id="mx" value="40" step="1"></div>
    <div class="row"><label class="chk"><input type="checkbox" id="cbCross" checked> Crosshair</label>
      <label class="chk"><input type="checkbox" id="cbMarks" checked> Hot/cold markers</label></div>

    <h3>Device</h3>
    <div class="row"><label for="rate">Sensor rate</label>
      <select id="rate"><option value="1">1 Hz</option><option value="2">2 Hz</option><option value="4" selected>4 Hz</option><option value="8">8 Hz</option></select></div>
    <div class="row"><label for="dpal">Screen palette</label>
      <select id="dpal"><option value="ironbow">Ironbow</option><option value="inferno">Inferno</option></select></div>

    <div class="btns">
      <button id="bFreeze">Freeze</button>
      <button id="bSnap" class="pri">Snapshot PNG</button>
      <button id="bCsv">Save CSV</button>
      <button id="bClear">Clear spots</button>
    </div>
    <div class="spots" style="margin-top:8px" id="fps">&nbsp;</div>
  </aside>
</main>
<footer>Temperatures are raw MLX90640 pixel values; interpolation is display-only. &middot; <a href="/setup">Wi-Fi setup</a></footer>

<script>
(()=>{
const W=32,H=24,N=768,HDR=28;
const $=id=>document.getElementById(id);
const img=$('img'),ov=$('ov'),bar=$('bar'),ictx=img.getContext('2d'),octx=ov.getContext('2d'),bctx=bar.getContext('2d');

// ---- palettes (same key colours as the firmware) ----
const KEYS={
 ironbow:[[0,0,0,0],[.2,30,0,110],[.4,150,0,130],[.55,210,30,60],[.7,245,110,0],[.85,255,210,40],[1,255,255,255]],
 inferno:[[0,0,0,4],[.1,22,11,57],[.2,66,10,104],[.3,106,23,110],[.4,147,38,103],[.5,188,55,84],[.6,221,81,58],[.7,243,120,25],[.8,252,165,10],[.9,246,215,70],[1,252,255,164]]};
function lutOf(k){const l=new Uint8Array(768);for(let i=0;i<256;i++){const t=i/255;let s=0;while(s<k.length-2&&t>k[s+1][0])s++;const a=k[s],b=k[s+1],f=(t-a[0])/(b[0]-a[0]);for(let c=0;c<3;c++)l[i*3+c]=Math.round(a[c+1]+(b[c+1]-a[c+1])*f);}return l;}
const LUT={ironbow:lutOf(KEYS.ironbow),inferno:lutOf(KEYS.inferno)};

// ---- state ----
const S={pal:'ironbow',k:8,auto:true,min:20,max:40,cross:true,marks:true,paused:false,spots:[],ptr:null,frame:null,lo:0,hi:1,rInit:false,fpsEma:0,lastT:0};
const KEY='thermalcam.v1';
try{Object.assign(S,JSON.parse(localStorage.getItem(KEY)||'{}'));S.spots=[];S.paused=false;}catch(e){}
const save=()=>{try{localStorage.setItem(KEY,JSON.stringify({pal:S.pal,k:S.k,auto:S.auto,min:S.min,max:S.max,cross:S.cross,marks:S.marks}))}catch(e){}};
const fmt=t=>t.toFixed(1)+'\u00B0C';
let imgData=null,ws=null,dirty=false;

// ---- websocket ----
function setConn(ok){$('dotConn').className='dot'+(ok?' ok':'');$('txtConn').textContent=ok?'live':'reconnecting';}
function setSens(ok){$('dotSens').className='dot'+(ok?' ok':'');$('txtSens').textContent=ok?'sensor OK':'sensor fail';}
function connect(){
  ws=new WebSocket((location.protocol==='https:'?'wss://':'ws://')+location.host+'/ws');
  ws.binaryType='arraybuffer';
  ws.onopen=()=>setConn(true);
  ws.onclose=()=>{setConn(false);setTimeout(connect,1500);};
  ws.onerror=()=>{try{ws.close()}catch(e){}};
  ws.onmessage=e=>{
    if(typeof e.data==='string'){try{const m=JSON.parse(e.data);if(m.type==='hello'){$('rate').value=m.rate;$('dpal').value=m.palette;}}catch(_){}return;}
    onFrame(e.data);
  };
}
function send(o){if(ws&&ws.readyState===1)ws.send(JSON.stringify(o));}

function onFrame(buf){
  if(buf.byteLength<HDR+N*2)return;
  const dv=new DataView(buf);
  if(dv.getUint8(0)!==1)return;
  const ok=!!(dv.getUint8(1)&1);setSens(ok);
  if(!ok||S.paused)return;
  const t=new Float32Array(N);
  for(let i=0;i<N;i++)t[i]=dv.getInt16(HDR+i*2,true)/100;
  S.frame={temps:t,min:dv.getFloat32(4,true),max:dv.getFloat32(8,true),avg:dv.getFloat32(12,true),ctr:dv.getFloat32(16,true),hot:dv.getUint16(20,true),cold:dv.getUint16(22,true)};
  const now=performance.now();
  if(S.lastT){const f=1000/(now-S.lastT);S.fpsEma=S.fpsEma?S.fpsEma*.8+f*.2:f;}
  S.lastT=now;
  if(!dirty){dirty=true;requestAnimationFrame(draw);}
}

// ---- rendering ----
function range(f){
  if(!S.auto)return[S.min,Math.max(S.max,S.min+1)];
  let mn=f.min,mx=f.max;
  if(mx-mn<5){const m=(mx+mn)/2;mn=m-2.5;mx=m+2.5;}
  if(!S.rInit){S.lo=mn;S.hi=mx;S.rInit=true;}else{S.lo+=.3*(mn-S.lo);S.hi+=.3*(mx-S.hi);}
  return[S.lo,S.hi];
}
function renderImage(f,lo,hi){
  const k=S.k,w=W*k,h=H*k;
  if(img.width!==w||img.height!==h){img.width=w;img.height=h;imgData=ictx.createImageData(w,h);}
  img.classList.toggle('px',k===1);
  const lut=LUT[S.pal],inv=255/(hi-lo),d=imgData.data,t=f.temps;
  const x0a=new Uint8Array(w),fxa=new Float32Array(w);
  for(let x=0;x<w;x++){let s=Math.max(0,(x+.5)/k-.5),x0=s|0,fx=s-x0;if(x0>=W-1){x0=W-1;fx=0;}x0a[x]=x0;fxa[x]=fx;}
  let o=0;
  for(let y=0;y<h;y++){
    let s=Math.max(0,(y+.5)/k-.5),y0=s|0,fy=s-y0;if(y0>=H-1){y0=H-1;fy=0;}
    const y1=Math.min(H-1,y0+1),r0=y0*W,r1=y1*W;
    for(let x=0;x<w;x++){
      const x0=x0a[x],x1=Math.min(W-1,x0+1),fx=fxa[x];
      const a=t[r0+x0]+(t[r0+x1]-t[r0+x0])*fx,b=t[r1+x0]+(t[r1+x1]-t[r1+x0])*fx;
      let i=((a+(b-a)*fy-lo)*inv)|0;if(i<0)i=0;else if(i>255)i=255;
      d[o++]=lut[i*3];d[o++]=lut[i*3+1];d[o++]=lut[i*3+2];d[o++]=255;
    }
  }
  ictx.putImageData(imgData,0,0);
}
function drawBar(){
  const id=bctx.createImageData(256,1),lut=LUT[S.pal];
  for(let i=0;i<256;i++){id.data[i*4]=lut[i*3];id.data[i*4+1]=lut[i*3+1];id.data[i*4+2]=lut[i*3+2];id.data[i*4+3]=255;}
  bctx.putImageData(id,0,0);
}
function label(c,txt,x,y,w,h,fs){
  c.font='600 '+fs+'px system-ui,sans-serif';
  const tw=c.measureText(txt).width+8,th=fs+6;
  let bx=x+10,by=y-th-6;
  if(bx+tw>w)bx=x-tw-10;if(bx<0)bx=2;
  if(by<0)by=y+10;if(by+th>h)by=h-th-2;
  c.fillStyle='rgba(0,0,0,.7)';c.fillRect(bx,by,tw,th);
  c.fillStyle='#fff';c.textBaseline='middle';c.fillText(txt,bx+4,by+th/2);
}
function ring(c,x,y,r,col,lw){c.strokeStyle='#000';c.lineWidth=lw+2;c.beginPath();c.arc(x,y,r,0,7);c.stroke();c.strokeStyle=col;c.lineWidth=lw;c.beginPath();c.arc(x,y,r,0,7);c.stroke();}
function drawMarkers(c,w,h,live){
  const f=S.frame;if(!f)return;
  const cw=w/W,ch=h/H,fs=Math.max(11,Math.round(w/46)),lw=Math.max(1.5,w/400),r=Math.max(6,w/70);
  const cx=i=>((i%W)+.5)*cw,cy=i=>(((i/W)|0)+.5)*ch;
  if(S.marks){
    ring(c,cx(f.hot),cy(f.hot),r,'#fff',lw);label(c,fmt(f.temps[f.hot]),cx(f.hot),cy(f.hot),w,h,fs);
    ring(c,cx(f.cold),cy(f.cold),r,'#4fc3f7',lw);label(c,fmt(f.temps[f.cold]),cx(f.cold),cy(f.cold),w,h,fs);
  }
  S.spots.forEach((s,i)=>{
    const x=(s.x+.5)*cw,y=(s.y+.5)*ch;ring(c,x,y,r*.8,'#3fb950',lw);
    c.strokeStyle='#3fb950';c.lineWidth=lw;c.beginPath();c.moveTo(x-r*1.4,y);c.lineTo(x+r*1.4,y);c.moveTo(x,y-r*1.4);c.lineTo(x,y+r*1.4);c.stroke();
    label(c,'S'+(i+1)+' '+fmt(f.temps[s.y*W+s.x]),x,y,w,h,fs);
  });
  if(live&&S.cross&&S.ptr){
    const p=S.ptr,x=p.u*w,y=p.v*h;
    c.strokeStyle='rgba(255,255,255,.65)';c.lineWidth=1;c.setLineDash([6,4]);
    c.beginPath();c.moveTo(0,y);c.lineTo(w,y);c.moveTo(x,0);c.lineTo(x,h);c.stroke();c.setLineDash([]);
    c.strokeStyle='#ffd33d';c.lineWidth=lw;c.strokeRect(p.px*cw,p.py*ch,cw,ch);
    label(c,fmt(f.temps[p.py*W+p.px])+'  ('+p.px+','+p.py+')',x,y,w,h,fs);
  }
}
function drawOverlay(){
  const cw=ov.clientWidth,ch=ov.clientHeight,dpr=window.devicePixelRatio||1;
  if(ov.width!==Math.round(cw*dpr)||ov.height!==Math.round(ch*dpr)){ov.width=Math.round(cw*dpr);ov.height=Math.round(ch*dpr);}
  octx.setTransform(1,0,0,1,0,0);octx.clearRect(0,0,ov.width,ov.height);octx.scale(dpr,dpr);
  drawMarkers(octx,cw,ch,true);
}
function updateText(f,lo,hi){
  $('sMax').textContent=fmt(f.max);$('sMin').textContent=fmt(f.min);$('sAvg').textContent=fmt(f.avg);$('sCtr').textContent=fmt(f.ctr);
  $('sHot').textContent=fmt(f.temps[f.hot]);$('sHotXY').textContent='pixel ('+(f.hot%W)+', '+((f.hot/W)|0)+')';
  $('sCold').textContent=fmt(f.temps[f.cold]);$('sColdXY').textContent='pixel ('+(f.cold%W)+', '+((f.cold/W)|0)+')';
  $('scLo').textContent=lo.toFixed(1)+'\u00B0C';$('scHi').textContent=hi.toFixed(1)+'\u00B0C';$('scMid').textContent=((lo+hi)/2).toFixed(1)+'\u00B0C';
  $('fps').textContent=S.fpsEma?S.fpsEma.toFixed(1)+' frames/s'+(S.paused?' (frozen)':''):'';
  $('sSpots').innerHTML=S.spots.map((s,i)=>'S'+(i+1)+' ('+s.x+','+s.y+'): '+fmt(f.temps[s.y*W+s.x])).join('<br>');
  if(S.ptr)$('sCur').textContent=fmt(f.temps[S.ptr.py*W+S.ptr.px])+'  @ ('+S.ptr.px+','+S.ptr.py+')';
}
function draw(){
  dirty=false;const f=S.frame;if(!f)return;
  const[lo,hi]=range(f);
  renderImage(f,lo,hi);updateText(f,lo,hi);drawOverlay();
}
function redraw(){if(S.frame){if(!dirty){dirty=true;requestAnimationFrame(()=>{dirty=false;drawOverlay();if(S.frame)updateText(S.frame,...(S.auto?[S.lo,S.hi]:[S.min,S.max]));});}}}

// ---- pointer: crosshair + spots ----
function pos(e){
  const r=ov.getBoundingClientRect(),u=Math.min(1,Math.max(0,(e.clientX-r.left)/r.width)),v=Math.min(1,Math.max(0,(e.clientY-r.top)/r.height));
  return{u,v,px:Math.min(W-1,(u*W)|0),py:Math.min(H-1,(v*H)|0)};
}
let down=null;
ov.addEventListener('pointermove',e=>{S.ptr=pos(e);redraw();});
ov.addEventListener('pointerdown',e=>{down={x:e.clientX,y:e.clientY};S.ptr=pos(e);try{ov.setPointerCapture(e.pointerId)}catch(_){}redraw();});
ov.addEventListener('pointerup',e=>{
  if(down&&Math.hypot(e.clientX-down.x,e.clientY-down.y)<8){const p=pos(e);S.spots.push({x:p.px,y:p.py});if(S.spots.length>4)S.spots.shift();}
  down=null;if(e.pointerType!=='mouse'){S.ptr=null;$('sCur').textContent='tap image to add spot';}redraw();
});
ov.addEventListener('pointerleave',e=>{if(e.pointerType==='mouse'){S.ptr=null;redraw();}});
window.addEventListener('resize',redraw);

// ---- controls ----
$('pal').onchange=e=>{S.pal=e.target.value;drawBar();save();draw();};
$('interp').onchange=e=>{S.k=+e.target.value;save();draw();};
$('mode').onchange=e=>{S.auto=e.target.value==='auto';save();draw();};
$('mn').onchange=e=>{S.min=+e.target.value;save();draw();};
$('mx').onchange=e=>{S.max=+e.target.value;save();draw();};
$('cbCross').onchange=e=>{S.cross=e.target.checked;save();redraw();};
$('cbMarks').onchange=e=>{S.marks=e.target.checked;save();redraw();};
$('rate').onchange=e=>send({cmd:'rate',hz:+e.target.value});
$('dpal').onchange=e=>send({cmd:'palette',name:e.target.value});
$('bClear').onclick=()=>{S.spots=[];$('sSpots').innerHTML='';redraw();};
$('bFreeze').onclick=e=>{S.paused=!S.paused;e.target.textContent=S.paused?'Resume':'Freeze';e.target.classList.toggle('pri',S.paused);};

function stamp(){const d=new Date(),p=n=>String(n).padStart(2,'0');return d.getFullYear()+p(d.getMonth()+1)+p(d.getDate())+'_'+p(d.getHours())+p(d.getMinutes())+p(d.getSeconds());}
function download(blob,name){const a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download=name;document.body.appendChild(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(a.href),2000);}
$('bSnap').onclick=()=>{
  const f=S.frame;if(!f)return;
  const SW=960,SH=720,FT=110,c=document.createElement('canvas');c.width=SW;c.height=SH+FT;
  const x=c.getContext('2d');x.fillStyle='#0b0e12';x.fillRect(0,0,SW,SH+FT);
  x.imageSmoothingEnabled=S.k>1;x.drawImage(img,0,0,SW,SH);
  drawMarkers(x,SW,SH,false);
  x.imageSmoothingEnabled=false;x.drawImage(bar,20,SH+10,SW-40,16);
  const lo=S.auto?S.lo:S.min,hi=S.auto?S.hi:S.max;
  x.fillStyle='#8b98a5';x.font='14px system-ui,sans-serif';x.textBaseline='top';
  x.textAlign='left';x.fillText(lo.toFixed(1)+'\u00B0C',20,SH+30);x.textAlign='right';x.fillText(hi.toFixed(1)+'\u00B0C',SW-20,SH+30);
  x.textAlign='left';x.fillStyle='#e6edf3';x.font='600 16px system-ui,sans-serif';
  x.fillText('Max '+fmt(f.max)+'   Min '+fmt(f.min)+'   Avg '+fmt(f.avg)+'   Centre '+fmt(f.ctr),20,SH+54);
  x.fillStyle='#8b98a5';x.font='13px system-ui,sans-serif';
  x.fillText('Hot ('+(f.hot%W)+','+((f.hot/W)|0)+')  Cold ('+(f.cold%W)+','+((f.cold/W)|0)+')   '+new Date().toLocaleString()+'   MLX90640 32x24, '+S.pal,20,SH+80);
  c.toBlob(b=>b&&download(b,'thermal_'+stamp()+'.png'),'image/png');
};
$('bCsv').onclick=()=>{
  const f=S.frame;if(!f)return;let s='';
  for(let y=0;y<H;y++){const r=[];for(let x=0;x<W;x++)r.push(f.temps[y*W+x].toFixed(2));s+=r.join(',')+'\n';}
  download(new Blob([s],{type:'text/csv'}),'thermal_'+stamp()+'.csv');
};

// ---- init ----
$('pal').value=S.pal;$('interp').value=S.k;$('mode').value=S.auto?'auto':'manual';$('mn').value=S.min;$('mx').value=S.max;
$('cbCross').checked=S.cross;$('cbMarks').checked=S.marks;
drawBar();connect();
})();
</script>
</body></html>
)HTMLPAGE";

// Página servida em GET / (etapa 5: criar, imprimir e configurar).
// Sem recursos externos: funciona sem internet.
#pragma once

const char PAGINA[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>EggBot</title>
<style>
:root{--primary:#3f51b5;--accent:#ff4081;--err:#c62828;--ok:#2e7d32;--bg:#eee;--text:rgba(0,0,0,.87);--muted:rgba(0,0,0,.55);--line:rgba(0,0,0,.12)}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--text);font-family:Roboto,system-ui,-apple-system,"Segoe UI",sans-serif;font-size:15px}
.bar{position:sticky;top:0;z-index:2;display:flex;align-items:center;gap:2px;height:56px;padding:0 6px;background:var(--primary);color:#fff;box-shadow:0 2px 4px rgba(0,0,0,.25)}
.bar .tab{background:none;border:0;border-bottom:2px solid transparent;border-radius:0;height:56px;color:#fff;padding:0 10px;font-weight:500;text-transform:uppercase;letter-spacing:.04em;font-size:.85rem;opacity:.65;cursor:pointer}
.bar .tab.on{opacity:1;color:var(--accent);border-bottom-color:var(--accent)}
.sp{flex:1}.net{display:flex;align-items:center;gap:6px;font-size:.8rem;white-space:nowrap;min-width:0}.net span:last-child{overflow:hidden;text-overflow:ellipsis}
@media(max-width:480px){.bar .tab{padding:0 7px;font-size:.78rem}#sNet{display:none}.frow>span:first-child{width:84px}}
main{max-width:640px;margin:0 auto;padding:24px 0}
.pane{display:flex;flex-direction:column;gap:12px}.pane[hidden]{display:none}
.card{background:#fff;border-radius:4px;padding:16px;box-shadow:0 1px 3px rgba(0,0,0,.2)}
@media(max-width:640px){main{padding:0 0 16px}.card{border-radius:0}}
h2{margin:0 0 10px;font-size:1.1rem;font-weight:500;display:flex;align-items:center;gap:8px}
h3{margin:14px 0 6px;font-size:.95rem;font-weight:500}
.dot{width:11px;height:11px;border-radius:50%;background:#bbb;flex:none}
.dot.ok{background:#43a047;box-shadow:0 0 6px #43a047}.dot.err{background:var(--err)}.dot.busy{background:#fb8c00}
canvas{display:block;width:100%;border-radius:4px;touch-action:none;cursor:crosshair;background:#f3e3d3}
#egg{touch-action:pan-y;cursor:grab;background:#cce0ff}
.leg{display:flex;gap:14px;font-size:.78rem;color:var(--muted)}.leg i{display:inline-block;width:16px;height:3px;vertical-align:middle;margin-right:4px}
input[type=range]{flex:1;min-width:0;accent-color:var(--accent)}
input[type=number]{width:96px;font:inherit;padding:6px;border:1px solid var(--line);border-radius:4px}
.tools{display:flex;flex-wrap:wrap;gap:8px;align-items:center;margin-top:8px}
button,select,label.btn{font:inherit;font-weight:500;border:0;border-radius:4px;height:36px;padding:0 12px;cursor:pointer}
button,label.btn{text-transform:uppercase;color:#fff;background:var(--primary);display:inline-flex;align-items:center}
button.alt,label.btn{background:#fff;color:var(--primary);border:1px solid var(--line)}
button.warn{background:#fff;color:var(--err);border:1px solid var(--line)}
button.go{background:var(--accent)}
button:disabled{opacity:.35;cursor:not-allowed}
select{background:#fff;border:1px solid var(--line);color:var(--text)}
.muted{color:var(--muted);font-size:.85rem}
.row{display:flex;justify-content:space-between;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid var(--line);font-size:.93rem}
.row:last-child{border:0}.row>span:first-child{color:var(--muted);flex:none}.row>span:last-child{text-align:right;word-break:break-word}
.frow{display:flex;align-items:center;gap:10px;padding:6px 0}.frow>span:first-child{width:118px;flex:none;color:var(--muted);font-size:.9rem}.frow b{width:44px;text-align:right;font-weight:500}
pre.con{margin:8px 0 0;padding:10px;max-height:200px;overflow:auto;background:#1e1e1e;color:#d4d4d4;border-radius:4px;font:12px/1.5 ui-monospace,Menlo,Consolas,monospace;white-space:pre}
.res{padding:10px 12px;border-radius:4px;margin:8px 0;font-size:.93rem;display:none}
.res.ok{display:block;background:#e8f5e9;color:#1b5e20}.res.err{display:block;background:#fdecea;color:#8a1c14}.res.info{display:block;background:#e8eaf6;color:#1a237e}
.prog{height:10px;border-radius:5px;background:rgba(0,0,0,.08);overflow:hidden;margin:4px 0 6px}
.prog i{display:block;height:100%;width:0;background:var(--accent);transition:width .4s}
.grid{display:grid;grid-template-columns:80px 1fr 1fr;gap:8px;align-items:center;margin:8px 0}
.grid4{display:grid;grid-template-columns:repeat(4,1fr);gap:8px;margin:8px 0}
.grid .k{color:var(--muted);font-size:.9rem}
.hint{font-size:.8rem;color:var(--muted);margin:8px 0 0;min-height:1em}
.console{display:flex;gap:8px;margin-top:10px}
.console input{flex:1;min-width:0;font:15px ui-monospace,Menlo,Consolas,monospace;padding:8px;border:1px solid var(--line);border-radius:4px}
ul.log{list-style:none;margin:10px 0 0;padding:8px;max-height:200px;overflow:auto;background:#1e1e1e;color:#d4d4d4;border-radius:4px;font:12px/1.55 ui-monospace,Menlo,Consolas,monospace}
ul.log .t{color:#888}.out{color:#9cdcfe}.in{color:#b5cea8}.bad{color:#f48771}
.save{position:sticky;bottom:0;display:flex;gap:8px;align-items:center;padding:10px 16px;background:#fff;box-shadow:0 -1px 3px rgba(0,0,0,.15)}
#toast{position:fixed;left:50%;bottom:20px;transform:translateX(-50%);background:#323232;color:#fff;padding:12px 16px;border-radius:4px;display:none;max-width:92vw;font-size:.9rem;z-index:5}
#svgBox{position:absolute;left:-10000px;top:0;width:800px;height:800px;overflow:hidden;visibility:hidden}
</style>
</head>
<body>
<div class="bar"><button class="tab on" data-t="criar">Criar</button><button class="tab" data-t="imprimir" id="tabPr">Imprimir</button><button class="tab" data-t="config">Config.</button><span class="sp"></span>
  <span class="net"><span class="dot" id="dNet"></span><span id="sNet">…</span></span></div>
<main>
 <div class="pane" id="t-criar">
  <section class="card">
    <h2>Desenho</h2>
    <canvas id="cv"></canvas>
    <div class="tools">
      <button class="alt" id="bL" title="Girar a vista">◀</button><span class="muted" id="viewLbl">rotação 0–1600</span><button class="alt" id="bR" title="Girar a vista">▶</button>
      <span class="sp"></span><button class="alt" id="bUndo">Desfazer</button><button class="warn" id="bClear">Limpar</button>
    </div>
    <div class="tools">
      <select id="pat"><option value="star">Estrela</option><option value="circle">Círculo</option><option value="square">Quadrado</option><option value="sine">Senoide (1 volta)</option><option value="multi">2 senoides (1 volta)</option><option value="helix">Espiral (2 voltas)</option></select><button class="alt" id="bPat">Inserir padrão</button>
      <span class="sp"></span>
      <select id="svgSize"><option value="1600">SVG: meia volta</option><option value="3150">SVG: volta inteira</option></select><label class="btn" for="svgIn">Importar SVG</label><input type="file" id="svgIn" accept=".svg,image/svg+xml" hidden>
    </div>
    <p class="muted" id="dStats" style="margin:8px 0 0">Desenhe com o dedo ou o mouse, insira um padrão ou importe um SVG (só contornos; texto precisa virar caminho no Inkscape). A área é a superfície do ovo aberta: largura = rotação (3200 passos = 1 volta), altura = braço (<span id="armLbl">−400 a 400</span> passos).</p>
  </section>

  <section class="card" id="cardEgg">
    <h2>Prévia no ovo</h2>
    <canvas id="egg"></canvas>
    <div class="tools"><span class="muted">Girar</span><input type="range" id="eRot" min="0" max="3199" step="10" value="0"><button class="alt" id="bCenter">Centralizar</button></div>
    <div class="leg" style="margin-top:6px"><span><i style="background:#7a1414"></i>traço (caneta abaixada)</span><span><i style="background:#2e9d5b"></i>deslocamento (caneta levantada)</span></div>
    <p class="muted" style="margin:6px 0 0">Mostrando <b id="eSrc">nada ainda</b>, já simplificado. Arraste para girar.</p>
  </section>

  <section class="card" id="cardFile">
    <h2>Arquivo gerado no navegador</h2>
    <div class="muted" id="fInfo">—</div>
    <pre class="con" id="fCon">(desenhe algo)</pre>
    <div class="res" id="res"></div>
    <div class="tools"><button id="bSend">Enviar para o EggBot</button><button class="alt" id="bGoPr" style="display:none">Ir para Imprimir</button></div>
  </section>
 </div>

 <div class="pane" id="t-imprimir" hidden>
  <section class="card" id="cardPr">
    <h2><span class="dot" id="dPr"></span><span id="sPr">Impressão</span></h2>
    <div class="row"><span>Arquivo no EggBot</span><span id="jName">nenhum</span></div>
    <div class="row"><span>Tamanho / pontos</span><span id="jSize">—</span></div>
    <div class="row"><span>Tempo estimado</span><span id="jEst">—</span></div>
    <div class="row"><span>Arduino</span><span id="prUno">—</span></div>
    <div class="row"><span>Braço</span><span id="prArm">—</span></div>
    <div class="prog"><i id="pBar"></i></div>
    <div class="row"><span>Progresso</span><span id="pTxt">—</span></div>
    <div class="row"><span>Decorrido / restante</span><span id="pTime">—</span></div>
    <div class="row"><span>Comando atual</span><span id="pCmd">—</span></div>
    <div class="res" id="pMsg"></div>
    <div class="tools">
      <button class="go" id="bStart">Iniciar</button><button class="alt" id="bPause">Pausar</button><button class="go" id="bResume" style="display:none">Continuar</button><button class="warn" id="bCancel">Cancelar</button>
      <span class="sp"></span><button class="warn" id="bDel">Apagar arquivo</button>
    </div>
    <p class="hint">Antes de iniciar: ovo preso e caneta no lugar. O desenho começa levando o braço da posição atual até o primeiro ponto (o centro do braço é o calibrado na aba Config.); a rotação conta a partir de onde o ovo está. No fim a caneta sobe, volta ao início e os motores desligam.</p>
  </section>
 </div>

 <div class="pane" id="t-config" hidden>
  <section class="card">
    <h2>Caneta (servo)</h2>
    <div class="frow"><span>Em cima</span><input type="range" class="cf" id="cPenUp" min="0" max="100"><b id="vPenUp">—</b><button class="alt pt" id="bTUp">Testar</button></div>
    <div class="frow"><span>Embaixo</span><input type="range" class="cf" id="cPenDown" min="0" max="100"><b id="vPenDown">—</b><button class="alt pt" id="bTDown">Testar</button></div>
    <div class="frow"><span>Tempo de subir/descer</span><input type="number" class="cf" id="cPenMs" min="100" max="2000" step="50"><span class="muted">ms</span></div>
    <p class="hint">"Testar" move o servo para a posição (grava no Arduino, mas só fica valendo no ESP32 ao salvar). 0–100 % equivale a 0–180° do servo.</p>
  </section>

  <section class="card">
    <h2>Braço</h2>
    <div class="row"><span>Posição atual</span><span id="armPos">—</span></div>
    <div class="grid4"><button class="cmd" data-c="SM,250,-50,0">− 50</button><button class="cmd" data-c="SM,100,-10,0">− 10</button><button class="cmd" data-c="SM,100,10,0">+ 10</button><button class="cmd" data-c="SM,250,50,0">+ 50</button></div>
    <div class="tools"><button class="alt" id="bZero">Definir aqui como centro</button><button class="alt" id="bToZero">Ir para o centro</button></div>
    <h3>Limites</h3>
    <div class="frow"><span>Superior</span><input type="number" class="cf" id="cArmMax" min="10" max="1600"><button class="alt" id="bMarkMax">Marcar aqui</button></div>
    <div class="frow"><span>Inferior</span><input type="number" class="cf" id="cArmMin" min="-1600" max="-10"><button class="alt" id="bMarkMin">Marcar aqui</button></div>
    <p class="hint">Calibração: leve a caneta ao meio da altura do ovo com os botões e toque em "Definir aqui como centro". Depois suba até o limite seguro e marque o superior; desça e marque o inferior. Salve. Se desligar os motores, a posição pode mudar: defina o centro de novo.</p>
  </section>

  <section class="card">
    <h2>Velocidade</h2>
    <div class="frow"><span>Desenhando</span><input type="number" class="cf" id="cVDown" min="50" max="1000" step="10"><span class="muted">passos/s (caneta abaixada)</span></div>
    <div class="frow"><span>Deslocando</span><input type="number" class="cf" id="cVUp" min="50" max="1000" step="10"><span class="muted">passos/s (caneta levantada)</span></div>
    <p class="hint">Mais devagar = traço mais firme. Mudar velocidade ou limites recalcula o arquivo que está no EggBot.</p>
  </section>
  <div class="save"><span class="muted" id="cfgState">—</span><span class="sp"></span><button class="alt" id="bCfgReset">Descartar</button><button id="bCfgSave">Salvar</button></div>

  <section class="card">
    <h2><span class="dot" id="dUno"></span><span id="sUno">Arduino: não verificado</span></h2>
    <div class="row"><span>Firmware</span><span id="ver">—</span></div>
    <div class="row"><span>Último comando</span><span id="last">—</span></div>
    <div class="row"><span>Erros</span><span id="errs">0</span></div>
    <div class="row"><span>ESP32: memória livre / flash</span><span id="sys">—</span></div>
    <div class="grid"><span class="k">Motores</span><button class="cmd" data-c="EM,1">Ligar</button><button class="alt cmd" data-c="EM,0">Desligar</button></div>
    <div class="grid"><span class="k">Caneta</span><button class="cmd" data-c="SP,0">Sobe ↑</button><button class="cmd" data-c="SP,1">Desce ↓</button></div>
    <div class="grid"><span class="k">Rotação</span><button class="cmd" data-c="SM,1000,0,-200">− 200</button><button class="cmd" data-c="SM,1000,0,200">+ 200</button></div>
    <div class="tools"><button class="alt cmd" data-c="v">Verificar Arduino</button></div>
    <div class="hint" id="busyHint"></div>
    <form class="console" id="cForm"><input id="cIn" maxlength="32" placeholder="console EBB: QP  ou  SM,500,50,0" autocomplete="off" autocapitalize="characters"><button id="cBtn">Enviar</button></form>
    <ul class="log" id="log"></ul>
  </section>
 </div>
</main>
<div id="toast"></div>
<div id="svgBox"></div>
<script>
(function(){
var $=function(i){return document.getElementById(i)},ROT=3200,VIEW=1600,MAXB=65536,AMIN=-400,AMAX=400;
var strokes=[],cur=null,view=0,file="",remote="",remoteSig="",st=null,fails=0,busy=false,tt=0,tab="criar",cfgDirty=false,cfgSig="";
function toast(m){var t=$("toast");t.textContent=m;t.style.display="block";clearTimeout(tt);tt=setTimeout(function(){t.style.display="none"},4500)}
function kb(b){return b<1024?b+" B":(b/1024).toFixed(1).replace(".",",")+" KB"}
function secs(s){var m=Math.floor(s/60),h=Math.floor(m/60);return h?h+" h "+(m%60)+" min":m?m+" min "+(s%60)+" s":s+" s"}
function fmtT(ms){var s=Math.floor(ms/1000);return("0"+Math.floor(s/60)%60).slice(-2)+":"+("0"+s%60).slice(-2)+"."+("00"+ms%1000).slice(-3)}
/* ---------- abas ---------- */
function showTab(t){tab=t;document.querySelectorAll(".bar .tab").forEach(function(b){b.className="tab"+(b.getAttribute("data-t")===t?" on":"")});
  ["criar","imprimir","config"].forEach(function(n){$("t-"+n).hidden=n!==t});
  var egg=$("cardEgg");if(t==="criar")$("t-criar").insertBefore(egg,$("cardFile"));else if(t==="imprimir")$("t-imprimir").appendChild(egg);
  try{localStorage.setItem("eggbotTab",t)}catch(e){}
  draw();setEgg()}
document.querySelectorAll(".bar .tab").forEach(function(b){b.onclick=function(){showTab(b.getAttribute("data-t"))}});
/* ---------- CRC-32 (IEEE, igual ao do ESP32) ---------- */
var T=[];for(var n=0;n<256;n++){var c=n;for(var k=0;k<8;k++)c=c&1?0xEDB88320^(c>>>1):c>>>1;T[n]=c>>>0}
function crc32(s){var b=new TextEncoder().encode(s),c=0xFFFFFFFF;for(var i=0;i<b.length;i++)c=T[(c^b[i])&255]^(c>>>8);return((c^0xFFFFFFFF)>>>0).toString(16).toUpperCase().padStart(8,"0")}
/* ---------- simplificação (Ramer–Douglas–Peucker) ---------- */
function rdp(p,e){if(p.length<3)return p.slice();var keep=new Uint8Array(p.length),s=[[0,p.length-1]];keep[0]=keep[p.length-1]=1;
  while(s.length){var ab=s.pop(),a=ab[0],b=ab[1],A=p[a],B=p[b],dx=B[0]-A[0],dy=B[1]-A[1],L=Math.hypot(dx,dy),md=0,mi=-1;
    for(var i=a+1;i<b;i++){var d=L<1e-6?Math.hypot(p[i][0]-A[0],p[i][1]-A[1]):Math.abs(dy*(p[i][0]-A[0])-dx*(p[i][1]-A[1]))/L;if(d>md){md=d;mi=i}}
    if(md>e){keep[mi]=1;s.push([a,mi],[mi,b])}}
  return p.filter(function(_,i){return keep[i]})}
/* ---------- arquivo EGGJOB 1 ---------- */
function build(name,strs){var body=[];strs.forEach(function(s){if(!s.length)return;body.push("U","L "+s[0][0]+" "+s[0][1],"D");for(var i=1;i<s.length;i++)body.push("L "+s[i][0]+" "+s[i][1])});
  if(body.length)body.push("U");var head="EGGJOB 1\nN "+name+"\nP "+body.length+"\n",txt=head+body.join("\n")+(body.length?"\n":"");return txt+"END "+crc32(txt)+"\n"}
function clampPt(q){var r=Math.max(-6400,Math.min(6400,q[0])),a=Math.max(AMIN,Math.min(AMAX,q[1]));return[r,a,r!==q[0]||a!==q[1]]}
function regen(){var out=0,simp=strokes.map(function(s){return rdp(s.map(function(q){var c=clampPt(q);if(c[2])out++;return[c[0],c[1]]}),2)}),n0=0,n1=0;
  strokes.forEach(function(s){n0+=s.length});simp.forEach(function(s){n1+=s.length});
  file=strokes.length?build("desenho",simp):"";var b=new Blob([file]).size,lines=file?file.split("\n").length-5:0;
  $("fInfo").textContent=file?strokes.length+" traço(s) · "+(n0!==n1?n0+" → "+n1+" pontos após simplificar":n1+" pontos")+" · "+kb(b)+(b>MAXB?" (acima do limite de 64 KB)":"")+(lines>8000?" (mais de 8000 linhas)":"")+(out?" · "+out+" ponto(s) ajustado(s) aos limites do braço":""):"—";
  $("fCon").textContent=file||"(desenhe algo)";$("dStats").style.display=strokes.length?"none":"";setEgg();lock()}
/* ---------- tela de desenho ---------- */
var cv=$("cv"),g=cv.getContext("2d");
function size(){var w=cv.clientWidth,h=Math.round(w*(AMAX-AMIN)/VIEW),d=window.devicePixelRatio||1;if(cv.width!==Math.round(w*d)||cv.height!==Math.round(h*d)){cv.width=Math.round(w*d);cv.height=Math.round(h*d);cv.style.height=h+"px"}return{w:w,h:h,d:d}}
function toXY(r,a,z){var dx=((r-view)%ROT+ROT)%ROT;return[dx*z.w/VIEW,(AMAX-a)*z.h/(AMAX-AMIN),dx]}
function draw(){if(!cv.clientWidth)return;var z=size(),y;g.setTransform(z.d,0,0,z.d,0,0);g.clearRect(0,0,z.w,z.h);
  g.strokeStyle="rgba(0,0,0,.08)";g.lineWidth=1;g.beginPath();for(var x=0;x<=VIEW;x+=200){g.moveTo(x*z.w/VIEW,0);g.lineTo(x*z.w/VIEW,z.h)}
  for(var a=Math.ceil(AMIN/100)*100;a<=AMAX;a+=100){y=(AMAX-a)*z.h/(AMAX-AMIN);g.moveTo(0,y);g.lineTo(z.w,y)}g.stroke();
  y=AMAX*z.h/(AMAX-AMIN);g.strokeStyle="rgba(63,81,181,.35)";g.setLineDash([6,4]);g.beginPath();g.moveTo(0,y);g.lineTo(z.w,y);g.stroke();g.setLineDash([]);
  g.strokeStyle="#7a1414";g.lineWidth=2;g.lineCap=g.lineJoin="round";
  strokes.forEach(function(s){g.beginPath();var p=null;s.forEach(function(q){var c=toXY(q[0],q[1],z);if(p&&Math.abs(c[2]-p[2])<VIEW)g.lineTo(c[0],c[1]);else g.moveTo(c[0],c[1]);p=c});if(s.length===1){var c=toXY(s[0][0],s[0][1],z);g.moveTo(c[0],c[1]);g.lineTo(c[0]+.1,c[1])}g.stroke()});
  $("viewLbl").textContent="rotação "+view+"–"+(view+VIEW)}
function pt(e){var z=size(),r=cv.getBoundingClientRect(),x=Math.max(0,Math.min(z.w,e.clientX-r.left)),y=Math.max(0,Math.min(z.h,e.clientY-r.top));return[Math.round(view+x*VIEW/z.w),Math.round(AMAX-y*(AMAX-AMIN)/z.h)]}
var last=null;
cv.addEventListener("pointerdown",function(e){cv.setPointerCapture(e.pointerId);cur=[pt(e)];last=[e.clientX,e.clientY];strokes.push(cur);draw()});
cv.addEventListener("pointermove",function(e){if(!cur||Math.hypot(e.clientX-last[0],e.clientY-last[1])<3)return;last=[e.clientX,e.clientY];cur.push(pt(e));draw()});
cv.addEventListener("pointerup",function(){if(cur){cur=null;regen()}});
$("bL").onclick=function(){view=Math.max(0,view-400);draw()};$("bR").onclick=function(){view=Math.min(ROT,view+400);draw()};
$("bUndo").onclick=function(){strokes.pop();draw();regen()};$("bClear").onclick=function(){strokes=[];draw();regen()};
window.addEventListener("resize",function(){draw();drawEgg()});
/* ---------- padrões prontos (fórmulas de preplotter/patterns.py, ajustadas aos limites do braço) ---------- */
function pattern(k){var c=(AMIN+AMAX)/2,h=(AMAX-AMIN)/2,cx=view+800,s=[],i,t,N;
  function P(r,a){s.push([Math.round(r),Math.round(a)])}
  if(k==="sine"||k==="multi"){N=k==="sine"?64:128;var per=k==="sine"?1:2;for(i=0;i<N;i++){t=i/(N-1);P(view+t*ROT,c+.8*h*Math.sin(2*Math.PI*t*per))}}
  else if(k==="helix"){N=256;for(i=0;i<N;i++){t=i/(N-1);P(t*2*ROT,c+.8*h*(2*t-1))}}
  else if(k==="circle"){N=128;for(i=0;i<=N;i++){t=2*Math.PI*i/N;P(cx+.8*h*Math.cos(t),c+.8*h*Math.sin(t))}}
  else if(k==="square"){var q=.7*h;[[-1,-1],[1,-1],[1,1],[-1,1],[-1,-1]].forEach(function(v){P(cx+q*v[0],c+q*v[1])})}
  else{var ro=.85*h,ri=ro*.375;for(i=0;i<=10;i++){var r=i%2?ri:ro;t=Math.PI/2+i*Math.PI/5;P(cx+r*Math.cos(t),c+r*Math.sin(t))}}
  return s}
$("bPat").onclick=function(){strokes.push(pattern($("pat").value));draw();regen()};
/* ---------- importar SVG: amostra os contornos com a geometria do próprio navegador ---------- */
function importSvg(text){var box=$("svgBox");box.innerHTML=text;var svg=box.querySelector("svg");if(!svg){box.innerHTML="";throw new Error("o arquivo não tem <svg>")}
  svg.setAttribute("width","800");svg.setAttribute("height","800");
  try{var els=svg.querySelectorAll("path,line,polyline,polygon,rect,circle,ellipse"),raw=[],lo=[1e9,1e9],hi=[-1e9,-1e9],tot=0,lens=[];
    els.forEach(function(el){var L=0;try{L=el.getTotalLength()}catch(e){}lens.push(L);tot+=L});
    if(!(tot>0))throw new Error("nenhum contorno encontrado (texto precisa virar caminho)");
    var step=Math.max(tot/12000,.5);
    els.forEach(function(el,ix){var L=lens[ix];if(!(L>0))return;var m=el.getScreenCTM();if(!m)return;var n=Math.max(2,Math.ceil(L/step)),s=[],prev=null;
      for(var i=0;i<=n;i++){var p=el.getPointAtLength(L*i/n),q=new DOMPoint(p.x,p.y).matrixTransform(m);
        if(prev&&Math.hypot(q.x-prev.x,q.y-prev.y)>3*step*Math.max(Math.abs(m.a),Math.abs(m.d),1e-9)+1){if(s.length>1)raw.push(s);s=[]}
        s.push([q.x,q.y]);prev=q;lo=[Math.min(lo[0],q.x),Math.min(lo[1],q.y)];hi=[Math.max(hi[0],q.x),Math.max(hi[1],q.y)]}
      if(s.length>1)raw.push(s)});
    if(!raw.length)throw new Error("nenhum contorno encontrado");
    var W=+$("svgSize").value,H=(AMAX-AMIN)*.95,w=Math.max(hi[0]-lo[0],1e-9),h=Math.max(hi[1]-lo[1],1e-9),sc=Math.min(W/w,H/h),
        r0=W>2000?view:view+800-w*sc/2,a0=(AMIN+AMAX)/2,cy=(lo[1]+hi[1])/2,pts=0;
    var add=raw.map(function(s){return rdp(s.map(function(q){return[Math.round(r0+(q[0]-lo[0])*sc),Math.round(a0-(q[1]-cy)*sc)]}),1)}).filter(function(s){return s.length>1});
    add.forEach(function(s){pts+=s.length;strokes.push(s)});return{strokes:add.length,points:pts}}
  finally{box.innerHTML=""}}
$("svgIn").onchange=function(){var f=this.files[0];this.value="";if(!f)return;f.text().then(function(t){var r=importSvg(t);draw();regen();toast("SVG importado: "+r.strokes+" traço(s), "+r.points+" pontos.")})
  .catch(function(e){toast("Não foi possível importar: "+e.message)})};
/* ---------- prévia no ovo: desenha o ARQUIVO (o gerado aqui ou o que está no EggBot) ---------- */
var ev=$("egg"),eg=ev.getContext("2d"),eView=0,eUser=false,ePts=[];
function wrap(x){return((x%ROT)+ROT)%ROT}
function parseJob(t){var out=[[0,0,0]],pen=0;t.split("\n").forEach(function(l){if(l==="U")pen=0;else if(l==="D")pen=1;else if(l.indexOf("L ")===0){var p=l.split(" ");out.push([+p[1],+p[2],pen])}});return out}
function eggW(t){return(0.72+0.08*Math.cos(t))*Math.sin(t)}
function setEgg(){var useRemote=tab==="imprimir"?!!remote:!file&&!!remote,t=useRemote?remote:file;ePts=parseJob(t);
  $("eSrc").textContent=useRemote?"o arquivo que está no EggBot":file?"o desenho atual (ainda no navegador)":"nada ainda";
  if(!eUser){var lo=1e9,hi=-1e9;ePts.forEach(function(p,i){if(i&&p[2]){lo=Math.min(lo,p[0]);hi=Math.max(hi,p[0])}});eView=lo<=hi?wrap(Math.round((lo+hi)/2)):wrap(view+VIEW/2)}drawEgg()}
function drawEgg(){var w=ev.clientWidth;if(!w)return;var h=Math.round(Math.min(Math.max(w*.7,220),320)),d=window.devicePixelRatio||1;
  if(ev.width!==Math.round(w*d)||ev.height!==Math.round(h*d)){ev.width=Math.round(w*d);ev.height=Math.round(h*d);ev.style.height=h+"px"}
  eg.setTransform(d,0,0,d,0,0);eg.clearRect(0,0,w,h);var cx=w/2,cy=h/2,S=h*.46,i,t;
  var gr=eg.createRadialGradient(cx-S*.3,cy-S*.35,S*.05,cx,cy,S*1.05);gr.addColorStop(0,"#f2cfb4");gr.addColorStop(.55,"#cb8d66");gr.addColorStop(1,"#8f5a3c");
  eg.fillStyle=gr;eg.beginPath();for(i=0;i<=60;i++){t=Math.PI*i/60;eg.lineTo(cx+eggW(t)*S,cy-Math.cos(t)*S)}for(i=60;i>=0;i--){t=Math.PI*i/60;eg.lineTo(cx-eggW(t)*S,cy-Math.cos(t)*S)}eg.fill();
  var prev=null;eg.lineCap=eg.lineJoin="round";
  ePts.forEach(function(p){var phi=p[1]/ROT*2*Math.PI,tt=Math.PI/2-phi,W=eggW(tt),th=(p[0]-eView)/ROT*2*Math.PI,q={x:cx+W*Math.sin(th)*S,y:cy-Math.cos(tt)*S,z:Math.cos(th)};
    if(prev&&prev.z>0&&q.z>0){eg.globalAlpha=.35+.65*q.z;eg.strokeStyle=p[2]?"#7a1414":"#2e9d5b";eg.lineWidth=p[2]?2:1;eg.setLineDash(p[2]?[]:[3,4]);eg.beginPath();eg.moveTo(prev.x,prev.y);eg.lineTo(q.x,q.y);eg.stroke()}prev=q});
  eg.setLineDash([]);eg.globalAlpha=1;$("eRot").value=eView}
var eDrag=null;
ev.addEventListener("pointerdown",function(e){ev.setPointerCapture(e.pointerId);eDrag={x:e.clientX,v:eView}});
ev.addEventListener("pointermove",function(e){if(!eDrag)return;eView=wrap(Math.round(eDrag.v-(e.clientX-eDrag.x)*6));eUser=true;drawEgg()});
ev.addEventListener("pointerup",function(){eDrag=null});
$("eRot").oninput=function(){eView=+this.value;eUser=true;drawEgg()};
$("bCenter").onclick=function(){eUser=false;setEgg()};
/* ---------- bloqueios ---------- */
function printing(){var p=st?st.print.state:"idle";return p==="running"||p==="paused"||p==="stopping"}
function lock(){var off=busy||fails>=2||!st,p=st?st.print.state:"idle",pr=printing(),ub=st&&(st.uno.busy||st.cfg_busy),jp=st&&st.job.present;
  $("bSend").disabled=off||!file||pr;$("bStart").disabled=off||!jp||pr||ub;
  $("bPause").disabled=off||p!=="running";$("bResume").disabled=off||p!=="paused";$("bCancel").disabled=off||(p!=="running"&&p!=="paused");
  $("bPause").style.display=p==="paused"?"none":"";$("bResume").style.display=p==="paused"?"":"none";
  $("bDel").disabled=off||!jp||pr;
  document.querySelectorAll("button.cmd,#cBtn,button.pt,#bZero,#bToZero").forEach(function(b){b.disabled=off||pr||ub});
  document.querySelectorAll(".cf,#bMarkMax,#bMarkMin").forEach(function(b){b.disabled=off||pr});
  $("bCfgSave").disabled=off||pr||!cfgDirty;$("bCfgReset").disabled=!cfgDirty}
/* ---------- envio do arquivo ---------- */
$("bSend").onclick=function(){busy=true;lock();var fd=new FormData();fd.append("file",new Blob([file],{type:"text/plain"}),"job.txt");var t0=performance.now();
  fetch("/job",{method:"POST",body:fd}).then(function(r){return r.json().catch(function(){return{ok:false,message:"Resposta inválida (HTTP "+r.status+")"}}).then(function(j){j.http=r.status;return j})})
  .then(function(j){var el=$("res");if(j.ok){el.className="res ok";el.textContent="✓ Arquivo válido e íntegro (CRC "+j.crc+" confere): "+j.ebb_cmds+" comandos, ~"+secs(j.est_s)+" de desenho. Ida e volta: "+Math.round(performance.now()-t0)+" ms.";$("bGoPr").style.display=""}
    else{el.className="res err";el.textContent="✗ Recusado pelo ESP32 (HTTP "+j.http+"): "+(j.message||"erro")}})
  .catch(function(){var el=$("res");el.className="res err";el.textContent="✗ Falha de rede no envio."})
  .then(function(){busy=false;refreshStatus()})};
$("bGoPr").onclick=function(){showTab("imprimir")};
/* ---------- ações ---------- */
function post(url){busy=true;lock();return fetch(url,{method:"POST",cache:"no-store"}).then(function(r){return r.json().then(function(j){if(!r.ok||!j.ok)throw new Error(j.message||("Erro "+r.status));return j})})
  .catch(function(e){toast(e.message||"Falha ao enviar");return null}).then(function(j){busy=false;refreshStatus();return j})}
function cmd(c){return post("/cmd?c="+encodeURIComponent(c))}
$("bStart").onclick=function(){post("/print/start")};$("bPause").onclick=function(){post("/print/pause")};$("bResume").onclick=function(){post("/print/resume")};
$("bCancel").onclick=function(){if(confirm("Cancelar a impressão? A caneta sobe, volta ao início e os motores desligam."))post("/print/cancel")};
$("bDel").onclick=function(){post("/job/delete").then(function(){$("res").className="res";$("bGoPr").style.display="none"})};
document.querySelectorAll("button.cmd").forEach(function(b){b.addEventListener("click",function(){cmd(b.getAttribute("data-c"))})});
$("cForm").addEventListener("submit",function(e){e.preventDefault();var v=$("cIn").value.trim().toUpperCase();if(v){cmd(v);$("cIn").value=""}});
/* ---------- configuração e calibração ---------- */
var CF={pen_up:"cPenUp",pen_down:"cPenDown",pen_ms:"cPenMs",speed_down:"cVDown",speed_up:"cVUp",arm_min:"cArmMin",arm_max:"cArmMax"};
function fillCfg(c){Object.keys(CF).forEach(function(k){$(CF[k]).value=c[k]});cfgDirty=false;showCfgVals()}
function showCfgVals(){$("vPenUp").textContent=$("cPenUp").value+" %";$("vPenDown").textContent=$("cPenDown").value+" %";
  $("cfgState").textContent=cfgDirty?"Alterações não salvas":"Configuração salva";lock()}
document.querySelectorAll(".cf").forEach(function(el){el.addEventListener("input",function(){cfgDirty=true;showCfgVals()})});
$("bCfgReset").onclick=function(){if(st)fillCfg(st.cfg)};
$("bCfgSave").onclick=function(){var q=Object.keys(CF).map(function(k){return k+"="+encodeURIComponent($(CF[k]).value.trim())}).join("&");
  post("/config?"+q).then(function(j){if(!j)return;cfgDirty=false;cfgSig="";toast(j.job_dropped?"Configuração salva. O arquivo no EggBot ficou fora dos novos limites e foi apagado.":"Configuração salva.")})};
$("bTUp").onclick=function(){post("/pen/test?pos=up&pct="+$("cPenUp").value)};$("bTDown").onclick=function(){post("/pen/test?pos=down&pct="+$("cPenDown").value)};
$("bZero").onclick=function(){post("/arm/zero")};
$("bToZero").onclick=function(){var p=st?st.arm_pos:0;if(!p){toast("O braço já está no centro.");return}var s=Math.max(-1600,Math.min(1600,-p));cmd("SM,"+Math.max(100,Math.ceil(Math.abs(s)*1000/400))+","+s+",0")};
$("bMarkMax").onclick=function(){var p=st?st.arm_pos:0;if(p<10){toast("Suba o braço acima do centro antes de marcar o limite superior.");return}$("cArmMax").value=p;cfgDirty=true;showCfgVals()};
$("bMarkMin").onclick=function(){var p=st?st.arm_pos:0;if(p>-10){toast("Desça o braço abaixo do centro antes de marcar o limite inferior.");return}$("cArmMin").value=p;cfgDirty=true;showCfgVals()};
/* ---------- estado ---------- */
var NOMES={idle:"Pronto para imprimir",running:"Imprimindo…",paused:"Pausado",stopping:"Parando com segurança…",done:"Desenho concluído",error:"Erro na impressão"};
function render(d){var j=d.job,p=d.print,u=d.uno,c=d.cfg;
  $("dNet").className="dot ok";$("sNet").textContent=d.ssid+" · "+d.clients+" cliente(s)";
  var sig=[c.arm_min,c.arm_max].join();if(sig!==cfgSig){cfgSig=sig;AMIN=c.arm_min;AMAX=c.arm_max;$("armLbl").textContent=AMIN+" a "+AMAX;draw();regen()}
  if(!cfgDirty)fillCfg(c);
  $("armPos").textContent=d.arm_pos===0?"no centro (0)":d.arm_pos+" passos "+(d.arm_pos>0?"acima":"abaixo")+" do centro";
  $("prArm").textContent=d.arm_pos===0?"no centro":d.arm_pos+" passos do centro";
  $("jName").textContent=j.present?j.name:"nenhum";$("jSize").textContent=j.present?kb(j.bytes)+" / "+j.points+" pontos":"—";
  $("jEst").textContent=j.present?"~"+secs(j.est_s)+" ("+j.ebb_cmds+" comandos)":"—";
  var act=p.state!=="idle",pct=p.total?Math.min(100,Math.round(p.done*100/p.total)):0;
  $("tabPr").textContent=printing()?"Imprimir "+pct+"%":"Imprimir";
  $("dPr").className="dot "+({running:"busy",paused:"busy",stopping:"busy",done:"ok",error:"err"}[p.state]||"");
  $("sPr").textContent=j.present||act?NOMES[p.state]:"Impressão (crie e envie um desenho na aba Criar)";
  $("pBar").style.width=(act?pct:0)+"%";$("pTxt").textContent=act&&p.total?p.done+" de "+p.total+" comandos ("+pct+"%)":"—";
  $("pTime").textContent=act?secs(p.elapsed_s)+(p.state==="running"||p.state==="paused"?" / ~"+secs(p.remaining_s):""):"—";
  $("pCmd").textContent=printing()&&p.cmd?p.cmd:"—";
  var m=$("pMsg");if(p.message){m.className="res "+(p.state==="error"?"err":p.state==="done"?"ok":"info");m.textContent=p.message}else m.className="res";
  var us=u.busy?"aguardando resposta…":u.state==="ok"?"respondendo":u.state==="no_reply"?"sem resposta":"não verificado";
  $("prUno").textContent=us;$("dUno").className="dot "+(u.busy?"busy":u.state==="ok"?"ok":u.state==="no_reply"?"err":"");$("sUno").textContent="Arduino: "+us;
  $("ver").textContent=u.version||"—";$("last").textContent=u.last&&u.last.cmd?u.last.cmd+" → "+(u.last.reply||"(nada)")+" ("+u.last.ms+" ms)":"—";
  $("errs").textContent=u.errors;$("sys").textContent=kb(d.free_heap)+" (mín. "+kb(d.min_free_heap)+") / "+kb(d.fs.used)+" de "+kb(d.fs.total);
  $("busyHint").textContent=printing()?"Comandos manuais ficam bloqueados durante a impressão.":d.cfg_busy?"Enviando a configuração da caneta ao Arduino…":u.busy?"Esperando o Arduino responder “"+u.pending+"”.":u.moving_ms>0?"Motor em movimento por mais ~"+Math.ceil(u.moving_ms/100)/10+" s.":"";
  var ul=$("log");ul.innerHTML="";
  d.log.forEach(function(e){var li=document.createElement("li"),a=document.createElement("span"),b=document.createElement("span");
    a.className="t";a.textContent=fmtT(e.t)+" ";b.className=e.dir===">"?"out":(e.dir==="!"?"bad":"in");
    b.textContent=(e.dir===">"?"ESP32 → Uno  ":e.dir==="!"?"erro         ":"Uno → ESP32  ")+e.txt;li.appendChild(a);li.appendChild(b);ul.appendChild(li)});
  var js=j.present?j.crc+j.bytes:"";if(js!==remoteSig){remoteSig=js;if(!j.present){remote="";setEgg()}
    else fetch("/job/raw",{cache:"no-store"}).then(function(r){return r.ok?r.text():""}).then(function(t){remote=t;setEgg()}).catch(function(){})}}
function refreshStatus(){return fetch("/status",{cache:"no-store"}).then(function(r){if(!r.ok)throw 0;return r.json()}).then(function(d){fails=0;st=d;render(d);lock()})
  .catch(function(){if(++fails>=2){$("dNet").className="dot err";$("sNet").textContent="sem conexão"}lock()})}
function tick(){refreshStatus().then(function(){setTimeout(tick,1000)})}
var t0="criar";try{t0=localStorage.getItem("eggbotTab")||"criar"}catch(e){}
showTab(["criar","imprimir","config"].indexOf(t0)>=0?t0:"criar");regen();tick();
})();
</script>
</body>
</html>
)rawliteral";

// Adaptador da prévia original. Mantém a simulação separada do transporte real.
// Não substitui a validação do firmware ou do movimento físico.
export function createDemoTransport() {
  const window = {};
  const timers = new Set();
  const setTimeout = (callback, delay) => {
    const timer = globalThis.setTimeout(() => { timers.delete(timer); callback(); }, delay);
    timers.add(timer);
    return timer;
  };

var t0=Date.now(),VEL=4,ms=function(){return Date.now()-t0},T=[],job=null,raw="",L=[];
var C={pen_up:60,pen_down:40,pen_ms:500,speed_down:300,speed_up:800,arm_min:-400,arm_max:400},armPos=0,cfgQ=[];
var U={state:"unknown",version:"",busy:false,pending:"",errors:0,last:{},mov:0};
var P={state:"idle",done:0,start:0,end:0,cmd:"",message:"",msDone:0,list:[],i:0,fim:[],inflight:false,pen:false,lifted:false,lower:false,r:0,a:0};
for(var n=0;n<256;n++){var c=n;for(var k=0;k<8;k++)c=c&1?0xEDB88320^(c>>>1):c>>>1;T[n]=c>>>0}
function crc(s){var b=new TextEncoder().encode(s),c=0xFFFFFFFF;for(var i=0;i<b.length;i++)c=T[(c^b[i])&255]^(c>>>8);return((c^0xFFFFFFFF)>>>0).toString(16).toUpperCase().padStart(8,"0")}
function conv(){var o={pen:0,r:0,a:0,out:[]};
  o.em=function(c,d){o.out.push({c:c,ms:d})};
  o.up=function(){if(o.pen){o.em("SP,0,"+C.pen_ms,C.pen_ms);o.pen=0}};o.down=function(){if(!o.pen){o.em("SP,1,"+C.pen_ms,C.pen_ms);o.pen=1}};
  o.go=function(nr,na){var dr=nr-o.r,da=na-o.a;if(!dr&&!da)return;var dur=Math.max(10,Math.ceil(Math.hypot(dr,da)/(o.pen?C.speed_down:C.speed_up)*1000)),parts=Math.ceil(dur/2000),pr=0,pa=0;
    for(var k=1;k<=parts;k++){var cr=Math.round(dr*k/parts),ca=Math.round(da*k/parts);o.em("SM,"+Math.ceil(dur/parts)+","+(ca-pa)+","+(cr-pr),Math.ceil(dur/parts));pr=cr;pa=ca}o.r=nr;o.a=na};
  return o}
function ebb(txt,a0){var ls=txt.split("\n"),o=conv();o.a=a0||0;o.em("EM,1",0);for(var i=3;i<ls.length;i++){var l=ls[i];if(l.indexOf("END ")===0)break;
    if(l==="U")o.up();else if(l==="D")o.down();else if(l[0]==="L"){var p=l.split(" ");o.go(+p[1],+p[2])}}
  o.up();o.go(0,0);o.em("EM,0",0);return o.out}
function validate(txt){var bytes=new TextEncoder().encode(txt).length;if(bytes>65536)return{http:413,message:"arquivo com "+(bytes/1024).toFixed(1).replace(".",",")+" KB, acima do limite de 64 KB"};
  var ls=txt.split("\n"),err=function(i,m){return{http:400,message:"linha "+(i+1)+": "+m}};
  if(ls[0]!=="EGGJOB 1")return err(0,"cabeçalho deve ser \"EGGJOB 1\"");
  var m=/^N ([A-Za-z0-9_-]{1,24})$/.exec(ls[1]||"");if(!m)return err(1,"nome inválido");
  var p=/^P (\d{1,5})$/.exec(ls[2]||"");if(!p||+p[1]<1||+p[1]>8000)return err(2,"contagem P inválida (1 a 8000)");
  var P_=+p[1],pts=0,dn=0;for(var i=3;i<3+P_;i++){var l=ls[i];if(l===undefined)return err(i,"arquivo terminou antes do esperado");
    if(l==="U")continue;if(l==="D"){dn++;continue}var q=/^L (-?\d{1,5}) (-?\d{1,5})$/.exec(l);if(!q)return err(i,"linha inválida \""+l.slice(0,20)+"\"");
    if(Math.abs(+q[1])>6400)return err(i,"rotação "+q[1]+" fora de ±6400");if(+q[2]<C.arm_min||+q[2]>C.arm_max)return err(i,"braço "+q[2]+" fora dos limites ("+C.arm_min+" a "+C.arm_max+")");pts++}
  var e=/^END ([0-9A-F]{8})$/.exec(ls[3+P_]||"");if(!e)return err(3+P_,"esperado \"END <crc>\" (P não confere com o número de linhas?)");
  var c=crc(ls.slice(0,3+P_).join("\n")+"\n");if(c!==e[1])return{http:400,message:"CRC não confere (arquivo diz "+e[1]+", calculado "+c+")"};
  var g=ebb(txt),tot=0;g.forEach(function(x){tot+=x.ms});
  return{ok:true,name:m[1],bytes:bytes,lines:3+P_+1,points:pts,strokes:dn,crc:c,ebb_cmds:g.length,est_s:Math.round(tot/1000),est_ms:tot,upload_ms:40+Math.round(bytes/60),parse_ms:2+Math.round(P_/250)}}
function lg(d,t){L.unshift({t:ms(),dir:d,txt:t});if(L.length>20)L.length=20}
function J(c,b){return Promise.resolve(new Response(JSON.stringify(b),{status:c,headers:{"Content-Type":"application/json"}}))}
/* Uno: um comando por vez; OK quando o movimento anterior termina */
function uno(c,d,cb){U.busy=true;U.pending=c;lg(">",c);var sm=/^SM,\d+,(-?\d+),/.exec(c);if(sm)armPos+=+sm[1];var sent=ms(),wait=Math.max(0,U.mov-ms());
  setTimeout(function(){var r;
    if(c==="v"){r="EBBv13_and_above Protocol emulated by Eggduino-Firmware V1.6a";U.version=r}
    else if(c==="QP"){lg("<",P.pen?"0":"1");r="OK"}else if(/^(EM|SP|SM|SC)(,|$)/.test(c))r="OK";else{r="unknown CMD";U.errors++}
    if(r==="OK"&&/^S[MP],/.test(c))U.mov=ms()+d/VEL;
    lg("<",r);U.state="ok";U.last={cmd:c,reply:c==="QP"?(P.pen?"0":"1")+" / OK":r,ok:r!=="unknown CMD",ms:ms()-sent};U.busy=false;U.pending="";cb&&cb(r==="unknown CMD")},wait+60)}
setTimeout(function(){uno("v",0)},1500);
/* executor da impressão */
function send(c,d,count){P.inflight=true;P.cmd=c;if(/^SP,1/.test(c))P.pen=true;else if(/^SP,0/.test(c))P.pen=false;
  else{var m=/^SM,\d+,(-?\d+),(-?\d+)/.exec(c);if(m){P.a+=+m[1];P.r+=+m[2]}}
  uno(c,d,function(){P.inflight=false;if(count){P.done++;P.msDone+=d}next()})}
function next(){if(P.inflight)return;
  if(P.state==="running"){if(P.lower){P.lower=false;return send("SP,1,"+C.pen_ms,C.pen_ms,false)}
    if(P.i<P.list.length){var x=P.list[P.i++];return send(x.c,x.ms,true)}
    P.state="done";P.end=ms();var s=Math.round((P.end-P.start)*VEL/1000);P.message="Desenho concluído: "+P.done+" comandos em "+Math.floor(s/60)+" min "+(s%60)+" s."}
  else if(P.state==="paused"){if(P.pen){P.lifted=true;send("SP,0,"+C.pen_ms,C.pen_ms,false)}}
  else if(P.state==="stopping"){var f=P.fim.shift();if(f)return send(f.c,f.ms,false);P.state="idle";P.end=ms()}}
function cfgNext(){if(U.busy||!cfgQ.length||P.state==="running"||P.state==="paused"||P.state==="stopping")return;var x=cfgQ.shift();uno(x.c,x.ms,cfgNext)}
window.fetch=function(u,o){u=String(u);var d=(o&&o.method)||"GET",pr=P.state==="running"||P.state==="paused"||P.state==="stopping";
  if(u.indexOf("/status")===0){var el=P.state==="idle"&&!P.start?0:Math.round(((pr?ms():P.end)-P.start)*VEL/1000);
    return J(200,{ssid:"EggBot-3F2A",clients:1,uptime_s:Math.floor(ms()/1000),free_heap:221000-Math.round(Math.random()*1500),min_free_heap:214200,fs:{total:1507328,used:8192+(job?Math.ceil(job.bytes/4096)*4096:0)},
      job:job?Object.assign({present:true},job):{present:false},
      uno:{state:U.state,version:U.version,busy:U.busy,pending:U.pending,moving_ms:Math.max(0,Math.round((U.mov-ms())*VEL)),errors:U.errors,last:U.last},
      cfg:C,arm_pos:armPos,cfg_busy:cfgQ.length>0,
      print:{state:P.state,done:P.done,total:P.total||(job?job.ebb_cmds:0),elapsed_s:el,remaining_s:pr&&job?Math.max(0,Math.round((P.est-P.msDone)/1000)):0,cmd:P.cmd,message:P.message},log:L.slice(0,12)})}
  if(u==="/job"&&d==="POST"){if(pr)return J(409,{ok:false,message:"Impressão em andamento: cancele antes de enviar outro arquivo."});
    return o.body.get("file").text().then(function(txt){var v=validate(txt);if(!v.ok)return J(v.http,{ok:false,message:v.message});
      job=v;raw=txt;if(P.state==="done"||P.state==="error"){P.state="idle";P.done=0;P.total=0;P.message="";P.cmd=""}return new Promise(function(res){setTimeout(function(){res(J(200,Object.assign({ok:true},v)))},v.upload_ms)})})}
  if(u.indexOf("/job/raw")===0)return job?Promise.resolve(new Response(raw,{status:200})):J(404,{ok:false});
  if(u==="/job/delete"){if(pr)return J(409,{ok:false,message:"Impressão em andamento: cancele antes de apagar."});job=null;raw="";P.state="idle";P.done=0;P.message="";P.cmd="";return J(200,{ok:true})}
  if(u==="/print/start"){if(pr)return J(409,{ok:false,message:"Já existe uma impressão em andamento."});if(!job)return J(404,{ok:false,message:"Nenhum arquivo no EggBot."});if(U.busy||cfgQ.length)return J(409,{ok:false,message:"Aguarde a resposta do comando anterior."});
    var li=ebb(raw,armPos),es=0;li.forEach(function(x){es+=x.ms});Object.assign(P,{state:"running",done:0,msDone:0,start:ms(),end:0,cmd:"",message:"",list:li,total:li.length,est:es,i:0,pen:false,lifted:false,lower:false,r:0,a:armPos});next();return J(200,{ok:true})}
  if(u==="/print/pause"){if(P.state!=="running")return J(409,{ok:false,message:"Não há impressão rodando."});P.state="paused";next();return J(200,{ok:true})}
  if(u==="/print/resume"){if(P.state!=="paused")return J(409,{ok:false,message:"A impressão não está pausada."});P.state="running";P.lower=P.lifted;P.lifted=false;next();return J(200,{ok:true})}
  if(u==="/print/cancel"){if(P.state!=="running"&&P.state!=="paused")return J(409,{ok:false,message:"Não há impressão para cancelar."});
    var cv=conv();cv.r=P.r;cv.a=P.a;cv.em("SP,0,"+C.pen_ms,C.pen_ms);cv.go(0,0);cv.em("EM,0",0);P.fim=cv.out;P.message="Impressão cancelada no comando "+P.done+" de "+P.total+".";P.state="stopping";next();return J(200,{ok:true})}
  if(u.indexOf("/cmd")===0){var c=decodeURIComponent(u.split("c=")[1]||"").toUpperCase();
    if(!/^[A-Z0-9,\-]{1,32}$/.test(c))return J(400,{ok:false,message:"Comando inválido."});if(pr)return J(409,{ok:false,message:"Impressão em andamento: use Pausar ou Cancelar."});
    if(U.busy||cfgQ.length)return J(409,{ok:false,message:"Aguarde a resposta do comando anterior."});if(c==="V")c="v";
    var m=/^S[MP],(\d+)(?:,(\d+))?/.exec(c),dd=m?(c[1]==="M"?+m[1]:+(m[2]||500)):0;if(/^SP,1/.test(c))P.pen=true;if(/^SP,0/.test(c))P.pen=false;uno(c,dd);return J(202,{ok:true})}
  if(u.indexOf("/config?")===0){if(pr)return J(409,{ok:false,message:"Impressão em andamento: cancele antes de mudar a configuração."});
    var q=new URLSearchParams(u.split("?")[1]),N=Object.assign({},C);Object.keys(C).forEach(function(k){if(q.has(k))N[k]=parseInt(q.get(k),10)});
    if(!(N.pen_up>=0&&N.pen_up<=100&&N.pen_down>=0&&N.pen_down<=100))return J(400,{ok:false,message:"Posições da caneta vão de 0 a 100 %."});
    if(!(N.pen_ms>=100&&N.pen_ms<=2000))return J(400,{ok:false,message:"Tempo da caneta deve ficar entre 100 e 2000 ms."});
    if(!(N.speed_down>=50&&N.speed_down<=1000&&N.speed_up>=50&&N.speed_up<=1000))return J(400,{ok:false,message:"Velocidades devem ficar entre 50 e 1000 passos/s (limite seguro do EggDuino)."});
    if(!(N.arm_max>=10&&N.arm_max<=1600))return J(400,{ok:false,message:"Limite superior do braço deve ficar entre 10 e 1600 passos."});
    if(!(N.arm_min<=-10&&N.arm_min>=-1600))return J(400,{ok:false,message:"Limite inferior do braço deve ficar entre −1600 e −10 passos."});
    var pen=N.pen_up!==C.pen_up||N.pen_down!==C.pen_down,drop=false;C=N;
    if(pen){cfgQ.push({c:"SC,5,"+240*(C.pen_up+25),ms:0},{c:"SC,4,"+240*(C.pen_down+25),ms:0});cfgNext()}
    if(job){var v2=validate(raw);if(v2.ok){v2.upload_ms=job.upload_ms;job=v2}else{job=null;raw="";drop=true;P.state="idle";P.done=0;P.total=0;P.message="O arquivo no EggBot ficou fora da nova configuração ("+v2.message+") e foi apagado; envie de novo."}}
    return J(200,{ok:true,job_dropped:drop})}
  if(u.indexOf("/pen/test")===0){if(pr)return J(409,{ok:false,message:"Impressão em andamento."});var q2=new URLSearchParams(u.split("?")[1]),up=q2.get("pos")==="up",pc=parseInt(q2.get("pct"),10);
    if(!(pc>=0&&pc<=100))return J(400,{ok:false,message:"Posição da caneta vai de 0 a 100 %."});if(U.busy||cfgQ.length)return J(409,{ok:false,message:"Aguarde a resposta do comando anterior."});
    cfgQ.push({c:"SC,"+(up?5:4)+","+240*(pc+25),ms:0},{c:"SP,"+(up?0:1)+","+C.pen_ms,ms:C.pen_ms});P.pen=!up;cfgNext();return J(200,{ok:true})}
  if(u==="/arm/zero"){if(pr)return J(409,{ok:false,message:"Impressão em andamento."});if(U.busy||cfgQ.length)return J(409,{ok:false,message:"Aguarde a resposta do comando anterior."});armPos=0;return J(200,{ok:true})}
  return J(404,{ok:false})};

  return {
    request: (path, options) => window.fetch(path, options),
    dispose: () => { timers.forEach(globalThis.clearTimeout); timers.clear(); },
  };
}

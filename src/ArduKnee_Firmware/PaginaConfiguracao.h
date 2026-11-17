#ifndef PAGINA_CONFIGURACAO_H
#define PAGINA_CONFIGURACAO_H

#include <Arduino.h>

// Pagina servida em "/" (Portal Cativo). Sem recursos externos (a rede nao tem internet)
// e sem innerHTML (so textContent). Alvos de toque >= 48 px, contraste >= 4,5:1 (RF24/PB-28).
// TODO(TAP 5.8): preencher PERFIS com os valores padrao oficiais de P1-P6. Enquanto isso,
// todos usam o padrao generico 10 / 70 / 10 em E1.
const char PAGINA_CONFIGURACAO[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ArduKnee</title>
<style>
:root{--tinta:#15231f;--papel:#f3f5f1;--linha:#b9c5bd;--acento:#0b5d4f;--erro:#9b1c1c;--ok:#14602d;--aviso-fundo:#fff3d1;--aviso:#5c3b00}
*{box-sizing:border-box}
body{margin:0;background:var(--papel);color:var(--tinta);font:16px/1.45 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
main{max-width:30rem;margin:0 auto;padding:1rem 1rem 3rem}
h1{font-size:1.1rem;margin:0;letter-spacing:.02em}
header{display:flex;justify-content:space-between;align-items:center;padding:.25rem 0 1rem}
#conexao{font-size:.85rem}
#conexao::before{content:"";display:inline-block;width:.6rem;height:.6rem;border-radius:50%;background:var(--ok);margin-right:.4rem}
#conexao.off::before{background:var(--erro)}
.painel{border:2px solid var(--tinta);padding:1rem;background:#fff}
#estado{font-size:1.15rem;font-weight:700;margin:0}
.leitura{display:flex;align-items:baseline;gap:.25rem;margin:.5rem 0 .25rem}
#angulo{font-size:3.5rem;font-weight:800;line-height:1;font-variant-numeric:tabular-nums}
.arco{height:.75rem;background:#e3e9e5;border:1px solid var(--linha);margin:.5rem 0}
.arco i{display:block;height:100%;width:0;background:var(--acento)}
dl{display:grid;grid-template-columns:auto 1fr;gap:.2rem 1rem;margin:.75rem 0 0}
dt{color:#44544d}dd{margin:0;font-weight:600;text-align:right}
#concluida{margin:.75rem 0 0;padding:.5rem .75rem;background:var(--ok);color:#fff;font-weight:700}
section{margin-top:1.5rem}
h2{font-size:1rem;margin:0 0 .5rem;display:flex;gap:.6rem;align-items:center}
h2 b{display:inline-grid;place-items:center;width:1.7rem;height:1.7rem;background:var(--tinta);color:#fff;border-radius:50%;font-size:.9rem}
label{display:block;margin:.9rem 0 .25rem;font-weight:600}
select,input,button{font:inherit;width:100%;min-height:3rem;padding:.5rem .75rem;border:2px solid var(--tinta);border-radius:0;background:#fff;color:var(--tinta)}
button{background:var(--acento);color:#fff;border-color:var(--acento);font-weight:700;margin-top:1rem;cursor:pointer}
button.sec{background:#fff;color:var(--acento)}
button:disabled{background:#d7ddd9;border-color:#d7ddd9;color:#44544d;cursor:not-allowed}
:focus-visible{outline:3px solid #1d4ed8;outline-offset:2px}
.dica{font-size:.85rem;color:#44544d;margin:.25rem 0 0}
.msg{margin:.75rem 0 0;font-weight:600;min-height:1.4rem}
.msg.erro{color:var(--erro)}.msg.ok{color:var(--ok)}
#aviso{margin-top:.75rem;padding:.75rem;background:var(--aviso-fundo);color:var(--aviso);border-left:6px solid var(--aviso)}
#aviso p{margin:0}
[hidden]{display:none!important}
.rodape{margin-top:1.5rem;font-size:.85rem;color:#44544d}
</style>
</head>
<body>
<main>
<header><h1>ArduKnee</h1><span id="conexao">Conectado</span></header>

<div class="painel" role="region" aria-label="Estado do aparelho">
  <p id="estado">Consultando…</p>
  <div class="leitura"><span id="angulo">–</span><span>°</span></div>
  <div class="arco" id="arco" hidden><i id="barra"></i></div>
  <dl>
    <dt>Repetições</dt><dd id="reps">–</dd>
    <dt>Amplitude máxima</dt><dd id="amp">–</dd>
    <dt>Calibração</dt><dd id="calib">–</dd>
    <dt>Parâmetros</dt><dd id="params">não configurado</dd>
  </dl>
  <p id="concluida" hidden>Sessão concluída</p>
</div>

<section>
<h2><b>1</b>Configurar a sessão</h2>
<form id="cfg" novalidate>
  <label for="perfil">Perfil clínico</label>
  <select id="perfil"><option>P1</option><option>P2</option><option>P3</option><option>P4</option><option>P5</option><option>P6</option></select>
  <label for="tipo">Tipo de exercício</label>
  <select id="tipo"><option value="E1">E1 · Flexão</option><option value="E2">E2 · Extensão</option><option value="E3">E3 · Ciclo completo</option></select>
  <label for="ini" id="lIni">Ângulo inicial (0 a 90)</label>
  <input id="ini" type="number" inputmode="numeric" min="0" max="90" step="1" required>
  <label for="alvo" id="lAlvo">Ângulo alvo (0 a 90)</label>
  <input id="alvo" type="number" inputmode="numeric" min="0" max="90" step="1" required>
  <p class="dica" id="dicaAng"></p>
  <label for="reps">Repetições</label>
  <input id="reps_in" type="number" inputmode="numeric" min="1" step="1" required>
  <div id="aviso" role="alert" hidden><p id="avisoTxt"></p><button type="button" id="confirmar">Confirmar e enviar</button></div>
  <button type="submit" id="enviar">Enviar configuração</button>
  <p class="msg" id="msgCfg" aria-live="polite"></p>
</form>
</section>

<section>
<h2><b>2</b>Calibrar o sensor</h2>
<p class="dica">Posicione o joelho num ângulo conhecido, informe-o abaixo e capture. Disponível depois que a configuração for aceita.</p>
<label for="ref">Ângulo de referência (0 a 90)</label>
<input id="ref" type="number" inputmode="numeric" min="0" max="90" step="1" value="0">
<button type="button" class="sec" id="calibrar" disabled>Capturar referência</button>
<p class="msg" id="msgCal" aria-live="polite"></p>
</section>

<section>
<h2><b>3</b>Iniciar no aparelho</h2>
<p>Aperte <strong>Iniciar</strong> duas vezes, com no máximo 5 segundos entre os toques. Encerrar e Reset existem só no aparelho (segure 2 s para encerrar ou resetar).</p>
</section>
<p class="rodape">Protótipo educacional. Os limites de ângulo e de repetições devem ser definidos com um fisioterapeuta.</p>
</main>

<script>
var PADRAO={tipo:"E1",ini:10,alvo:70,reps:10};
var PERFIS={P1:PADRAO,P2:PADRAO,P3:PADRAO,P4:PADRAO,P5:PADRAO,P6:PADRAO};
function $(i){return document.getElementById(i)}
function msg(id,t,c){var e=$(id);e.textContent=t;e.className="msg"+(c?" "+c:"")}
function pad(n){return(n<10?"0":"")+n}
function agoraISO(){var d=new Date(),o=-d.getTimezoneOffset(),s=o<0?"-":"+";o=Math.abs(o);
return d.getFullYear()+"-"+pad(d.getMonth()+1)+"-"+pad(d.getDate())+"T"+pad(d.getHours())+":"+pad(d.getMinutes())+":"+pad(d.getSeconds())+s+pad(Math.floor(o/60))+":"+pad(o%60)}

function aplicarPerfil(){var p=PERFIS[$("perfil").value];$("tipo").value=p.tipo;$("ini").value=p.ini;$("alvo").value=p.alvo;$("reps_in").value=p.reps;rotulos()}
function rotulos(){var e2=$("tipo").value==="E2";
$("lIni").textContent=e2?"Ângulo inicial (partida, maior que o alvo)":"Ângulo inicial (partida)";
$("lAlvo").textContent=e2?"Ângulo alvo (chegada)":"Ângulo alvo (chegada, maior que o inicial)";
$("dicaAng").textContent=e2?"Extensão: o alvo é menor que o ângulo inicial.":"Flexão e ciclo completo: o alvo é maior que o ângulo inicial."}

async function pedir(url,corpo){
 var c=("AbortController" in window)?new AbortController():null,t=setTimeout(function(){if(c)c.abort()},4000);
 try{var r=await fetch(url,{method:corpo?"POST":"GET",headers:corpo?{"Content-Type":"application/json"}:{},body:corpo?JSON.stringify(corpo):undefined,signal:c?c.signal:undefined,cache:"no-store"});
 var j={};try{j=await r.json()}catch(e){}return{http:r.status,j:j}}finally{clearTimeout(t)}}

function montarCorpo(conf){return{perfil:$("perfil").value,tipoExercicio:$("tipo").value,anguloInicial:parseInt($("ini").value,10),anguloAlvo:parseInt($("alvo").value,10),repeticoes:parseInt($("reps_in").value,10),timestampISO:agoraISO(),confirmado:conf}}

async function enviar(conf){
 $("aviso").hidden=true;msg("msgCfg","Enviando…");
 try{var r=await pedir("/configurar",montarCorpo(conf));
  if(r.http===200)msg("msgCfg","Configuração recebida. Agora calibre o sensor (passo 2).","ok");
  else if(r.http===202){$("avisoTxt").textContent=r.j.mensagem;$("aviso").hidden=false;msg("msgCfg","")}
  else if(r.http===409)msg("msgCfg","Há uma sessão em andamento. Encerre no aparelho antes de reconfigurar.","erro");
  else msg("msgCfg",r.j.mensagem||"Não foi possível enviar.","erro")}
 catch(e){msg("msgCfg","Sem resposta do aparelho. Confira a conexão e tente de novo.","erro")}}

async function calibrar(){
 var v=parseInt($("ref").value,10);
 if(isNaN(v)||v<0||v>90){msg("msgCal","Informe um ângulo de 0 a 90.","erro");return}
 msg("msgCal","Capturando…");
 try{var r=await pedir("/calibrar",{anguloReferencia:v});
  if(r.http===200)msg("msgCal","Referência capturada.","ok");
  else if(r.http===503)msg("msgCal","Leitura instável. Mantenha o joelho parado e capture de novo.","erro");
  else if(r.http===409)msg("msgCal","A calibração só é aceita depois da configuração e antes do início.","erro");
  else msg("msgCal",r.j.mensagem||"Não foi possível calibrar.","erro")}
 catch(e){msg("msgCal","Sem resposta do aparelho. Tente de novo.","erro")}}

function mostrar(s){
 $("estado").textContent=s.estado;
 $("angulo").textContent=s.calibrado?s.anguloAtual:"–";
 $("reps").textContent=s.configurado?s.repeticoes+" / "+s.metaRepeticoes:"–";
 $("amp").textContent=s.configurado?s.amplitudeMaxima+"°":"–";
 $("calib").textContent=s.calibrado?"Calibrado":"Pendente";
 var p=s.parametros;
 $("params").textContent=p?p.perfil+" · "+p.tipoExercicio+" · "+p.anguloInicial+"°→"+p.anguloAlvo+"° · "+p.repeticoes+"×":"não configurado";
 $("concluida").hidden=!s.sessaoConcluida;
 var arco=$("arco");
 if(p&&s.calibrado){var f=(s.anguloAtual-p.anguloInicial)/(p.anguloAlvo-p.anguloInicial);f=Math.max(0,Math.min(1,f));arco.hidden=false;$("barra").style.width=(f*100)+"%"}else arco.hidden=true;
 var emSessao=s.estado==="Sessão Ativa"||s.estado==="Sessão Pausada";
 $("enviar").disabled=emSessao;
 $("calibrar").disabled=s.estado!=="Aguardando Início"}

async function ciclo(){
 try{var r=await pedir("/status");if(r.http===200){mostrar(r.j);$("conexao").textContent="Conectado";$("conexao").className=""}}
 catch(e){$("conexao").textContent="Sem resposta";$("conexao").className="off"}
 setTimeout(ciclo,2000)}

$("perfil").addEventListener("change",aplicarPerfil);
$("tipo").addEventListener("change",rotulos);
$("cfg").addEventListener("submit",function(e){e.preventDefault();enviar(false)});
$("cfg").addEventListener("input",function(){$("aviso").hidden=true});
$("confirmar").addEventListener("click",function(){enviar(true)});
$("calibrar").addEventListener("click",calibrar);
aplicarPerfil();ciclo();
</script>
</body>
</html>
)rawliteral";

#endif

#pragma once
#include <Arduino.h>

// Device config page (online): the webhook + power settings. WiFi onboarding is
// handled by the FireLabs core wizard, so it's not here.
static const char CONFIG_HTML[] PROGMEM = R"=====(<!DOCTYPE html>
<html lang="en"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="color-scheme" content="light dark">
<title>FireLabs Weather Display</title>
<style>
:root{color-scheme:light dark;--red:#FA2800;--orange:#FFA01E;--grad:linear-gradient(135deg,#FA2800,#FF7A12 55%,#FFA01E);
--ink:#1B1613;--muted:#90847b;--card:#fff;--line:#e4d6c8;--page:#FBF6F1;--inp:#fffdfb}
@media(prefers-color-scheme:dark){:root{--ink:#f3ebe3;--muted:#9c8e82;--card:#1d1916;--line:#39312b;--page:#141110;--inp:#171310}}
*{box-sizing:border-box}html,body{overflow-x:hidden}body{margin:0;overflow-wrap:anywhere;background:var(--page);color:var(--ink);font-family:system-ui,Segoe UI,sans-serif}
.wrap{max-width:440px;margin:26px auto 60px;padding:0 18px}
h1{font-size:20px;margin:0 0 4px}.sub{color:var(--muted);font-size:13px;margin-bottom:18px}
.card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:16px 18px;margin-bottom:14px}
.hd{font-weight:700;font-size:14px;margin-bottom:6px}
label{display:block;margin-top:12px;font-size:12px;font-weight:600;color:var(--muted)}
input{width:100%;margin-top:5px;font-size:15px;color:var(--ink);padding:10px 11px;border:1px solid var(--line);border-radius:9px;background:var(--inp)}
.row{display:flex;gap:10px}.row>div{flex:1}
.btn{font-family:inherit;font-weight:700;font-size:14px;border:0;border-radius:10px;padding:12px;width:100%;margin-top:16px;cursor:pointer}
.btn.p{background:var(--grad);color:#fff}.btn.g{background:var(--card);color:var(--ink);border:1px solid var(--line)}
.help{font-size:12px;color:var(--muted);margin-top:6px}
.toast{position:fixed;left:50%;bottom:24px;transform:translateX(-50%) translateY(80px);background:var(--ink);color:var(--page);padding:11px 18px;border-radius:10px;font-size:13px;font-weight:600;opacity:0;transition:.3s}.toast.show{transform:translateX(-50%);opacity:1}
</style></head><body>
<div class="wrap">
<h1>FireLabs Weather Display</h1><div class="sub" id="host"></div>

<div class="card"><div class="hd">Home Assistant</div>
<label>Webhook URL<input id="webhook" placeholder="http://homeassistant.local:8123/api/webhook/xxxx"></label>
<div class="help">Copy it from the FireLabs integration when you add this device.</div></div>

<div class="card"><div class="hd">Power</div>
<label>Refresh every (minutes)<input id="sleep" type="number" min="5" max="240"></label>
<div class="row"><div><label>Quiet from (hr)<input id="qs" type="number" min="0" max="23"></label></div>
<div><label>Quiet until (hr)<input id="qe" type="number" min="0" max="23"></label></div></div></div>

<button class="btn p" onclick="save()">Save</button>

<div class="card" style="margin-top:14px"><div class="hd">Firmware update</div>
<form method="POST" action="/update" enctype="multipart/form-data"><input type="file" name="firmware" accept=".bin"><button class="btn g" style="margin-top:10px">Upload &amp; flash</button></form></div>
<button class="btn g" onclick="if(confirm('Erase all settings and reboot to setup?'))fetch('/api/reset',{method:'POST'})">Factory reset</button>
</div>
<div class="toast" id="t"></div>
<script>
function $(i){return document.getElementById(i)}
function toast(m){var t=$('t');t.textContent=m;t.classList.add('show');setTimeout(()=>t.classList.remove('show'),2500)}
fetch('/api/config').then(r=>r.json()).then(c=>{
$('host').textContent=(c.name||'')+'  ·  '+c.host+'.local  ·  '+c.mac;
$('webhook').value=c.webhook||'';$('sleep').value=c.sleep_min||30;$('qs').value=c.quiet_start;$('qe').value=c.quiet_end;});
function save(){var b={webhook:$('webhook').value,sleep_min:+$('sleep').value,quiet_start:+$('qs').value,quiet_end:+$('qe').value};
fetch('/api/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b)}).then(r=>r.json()).then(()=>toast('Saved. Rebooting...'));}
</script></body></html>)=====";

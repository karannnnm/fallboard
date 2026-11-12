#pragma once

static const char INDEX_HTML[] = R"HTML(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no,interactive-widget=resizes-content">
<title>Rescue Keyboard</title>
<style>
:root{--bg:#f4f4f2;--fg:#1b1b1a;--muted:#6b6b66;--card:#fff;--line:#d9d9d4;--pad:#e9e9e5;--accent:#2457d6;--on:#fff;--ok:#1f8a4c;--bad:#c0392b}
@media (prefers-color-scheme:dark){:root{--bg:#141413;--fg:#ececea;--muted:#9a9a94;--card:#1f1f1d;--line:#34342f;--pad:#232321;--accent:#6b93ff;--on:#0b1530;--ok:#4cc27e;--bad:#ff6b5b}}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html,body{height:100%}
body{margin:0;background:var(--bg);color:var(--fg);font:16px/1.4 -apple-system,system-ui,sans-serif;overscroll-behavior:none;-webkit-user-select:none;user-select:none}
button{font:inherit;font-size:15px;padding:10px 12px;border-radius:10px;border:1px solid var(--line);background:var(--card);color:var(--fg);cursor:pointer;touch-action:manipulation;white-space:nowrap}
button.on{background:var(--accent);border-color:var(--accent);color:var(--on)}
button.primary{background:var(--accent);border-color:var(--accent);color:var(--on);width:100%}
input[type=password],input[type=text]{width:100%;font-size:18px;padding:12px;border:1px solid var(--line);border-radius:10px;background:var(--bg);color:var(--fg);-webkit-user-select:text;user-select:text}
.hidden{display:none!important}
header{display:flex;align-items:center;gap:10px;padding:10px 16px}
header h1{font-size:17px;margin:0;flex:1}
.status{font-size:13px;color:var(--muted);display:flex;align-items:center}
.dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:6px;background:var(--bad)}
.dot.on{background:var(--ok)}
.link{background:none;border:none;padding:6px;color:var(--muted);font-size:14px}
#login{max-width:420px;margin:24px auto;padding:0 16px}
.card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:16px}
.msg{min-height:20px;font-size:14px;margin-top:10px;color:var(--bad)}
#app{position:fixed;inset:52px 0 0 0;display:flex;flex-direction:column;gap:8px;padding:0 12px 12px}
#pad{flex:1;min-height:120px;background:var(--pad);border:1px solid var(--line);border-radius:16px;touch-action:none;display:flex;align-items:center;justify-content:center;text-align:center;color:var(--muted);font-size:13px;padding:16px;position:relative}
#pad.locked{outline:2px solid var(--accent)}
.row{display:flex;gap:8px}
.row>button{flex:1}
#bar{display:flex;gap:6px;overflow-x:auto;scrollbar-width:none;padding-bottom:2px}
#bar::-webkit-scrollbar{display:none}
#bar button{padding:9px 11px;min-width:42px}
#bar .sep{flex:0 0 1px;background:var(--line);margin:4px 2px}
#kb{position:fixed;top:0;left:0;width:1px;height:1px;opacity:0;font-size:16px;border:0;padding:0;resize:none}
.sheet{position:fixed;inset:0;background:rgba(0,0,0,.35);display:flex;align-items:flex-end;justify-content:center;z-index:5}
.sheet .card{width:100%;max-width:440px;border-radius:16px 16px 0 0;padding:16px 16px 24px}
.sheet h2{font-size:16px;margin:0 0 12px}
.opt{display:flex;align-items:center;gap:8px;margin:12px 0;font-size:14px;color:var(--muted)}
.opt input[type=range]{flex:1}
#toast{position:fixed;left:50%;bottom:24px;transform:translateX(-50%);background:var(--fg);color:var(--bg);padding:8px 14px;border-radius:999px;font-size:14px;z-index:9;max-width:90%;text-align:center}
</style></head><body>
<header>
  <h1>Rescue Keyboard</h1>
  <span class="status"><span id="dot" class="dot"></span><span id="ble">…</span></span>
  <button class="link hidden" id="gear" aria-label="Settings">⚙</button>
</header>

<section id="login" class="hidden"><div class="card">
  <form id="loginForm">
    <input id="pin" type="password" placeholder="PIN" autocomplete="off">
    <button class="primary" type="submit" style="margin-top:12px">Unlock</button>
  </form>
  <div id="loginMsg" class="msg"></div>
</div></section>

<section id="app" class="hidden">
  <div id="pad"><span id="padHint"></span></div>
  <div class="row">
    <button data-click="1">Left</button>
    <button data-click="2">Right</button>
    <button id="drag">Hold</button>
  </div>
  <div id="bar">
    <button data-mod="8">⌘</button><button data-mod="4">⌥</button><button data-mod="1">⌃</button><button data-mod="2">⇧</button>
    <span class="sep"></span>
    <button data-key="41">esc</button><button data-key="43">tab</button><button data-key="42">⌫</button><button data-key="40">⏎</button>
    <button data-key="80">←</button><button data-key="82">↑</button><button data-key="81">↓</button><button data-key="79">→</button>
    <span class="sep"></span>
    <button data-key="44" data-mods="8">⌘ Space</button><button data-key="43" data-mods="8">⌘ Tab</button>
    <button data-key="20" data-mods="8">⌘ Q</button><button data-key="26" data-mods="8">⌘ W</button>
    <button data-key="20" data-mods="9">Lock</button>
    <span class="sep"></span>
    <button data-cc="234">🔉</button><button data-cc="233">🔊</button><button data-cc="226">🔇</button><button data-cc="205">⏯</button>
    <button data-cc="112">🔅</button><button data-cc="111">🔆</button>
  </div>
  <div class="row">
    <button id="kbBtn">⌨ Keyboard</button>
    <button id="pwBtn">Type password</button>
  </div>
  <textarea id="kb" autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false"></textarea>
</section>

<div id="pwSheet" class="sheet hidden"><div class="card">
  <h2>Type a password</h2>
  <form id="pwForm">
    <input id="pw" type="password" placeholder="Typed on the Mac, never shown" autocomplete="off" autocapitalize="off" autocorrect="off" spellcheck="false">
    <label class="opt"><input id="pwShow" type="checkbox"> Show</label>
    <label class="opt"><input id="pwEnter" type="checkbox" checked> Press Enter after</label>
    <div class="row"><button type="button" id="pwCancel">Cancel</button><button type="submit" class="primary">Type it</button></div>
  </form>
</div></div>

<div id="setSheet" class="sheet hidden"><div class="card">
  <h2>Settings</h2>
  <label class="opt">Pointer speed <input id="sens" type="range" min="0.5" max="5" step="0.25"></label>
  <label class="opt">Scroll speed <input id="ssens" type="range" min="0.25" max="3" step="0.25"></label>
  <label class="opt"><input id="invert" type="checkbox"> Reverse scroll direction</label>
  <div class="row"><button id="logout">Log out</button><button class="primary" id="setDone">Done</button></div>
</div></div>

<div id="toast" class="hidden"></div>

<script>
const $ = id => document.getElementById(id);
const FINE = matchMedia('(pointer:fine) and (hover:hover)').matches;
const MOD_CODES = {1: 0xE0, 2: 0xE1, 4: 0xE2, 8: 0xE3};

// ---- Settings (per device, best effort) ----
const settings = {sens: FINE ? 1 : 2, ssens: 1, invert: false};
try { Object.assign(settings, JSON.parse(localStorage.getItem('rk') || '{}')); } catch (e) {}
function saveSettings() { try { localStorage.setItem('rk', JSON.stringify(settings)); } catch (e) {} }
$('sens').value = settings.sens; $('ssens').value = settings.ssens; $('invert').checked = settings.invert;
$('sens').oninput = e => { settings.sens = +e.target.value; saveSettings(); };
$('ssens').oninput = e => { settings.ssens = +e.target.value; saveSettings(); };
$('invert').onchange = e => { settings.invert = e.target.checked; saveSettings(); };

let toastTimer;
function toast(m) {
  $('toast').textContent = m; $('toast').classList.remove('hidden');
  clearTimeout(toastTimer); toastTimer = setTimeout(() => $('toast').classList.add('hidden'), 2500);
}

// ---- Connection ----
let ws = null;
function send(cmd) {
  if (ws && ws.readyState === 1) { ws.send(cmd); return true; }
  return false;
}
function setBle(on) {
  $('dot').className = 'dot' + (on ? ' on' : '');
  $('ble').textContent = on ? 'Mac connected' : 'Mac not connected';
}
function showApp(on) {
  $('login').classList.toggle('hidden', on);
  $('app').classList.toggle('hidden', !on);
  $('gear').classList.toggle('hidden', !on);
}
async function check() {
  try {
    const s = await (await fetch('/api/status', {cache: 'no-store'})).json();
    setBle(s.ble);
    if (s.authed) connect(); else { showApp(false); $('pin').focus(); }
  } catch (e) { $('ble').textContent = 'Lost connection to device'; setTimeout(check, 2000); }
}
function connect() {
  if (ws && ws.readyState <= 1) return;
  ws = new WebSocket('ws://' + location.host + '/ws');
  ws.onopen = () => showApp(true);
  ws.onmessage = e => {
    if (e.data.startsWith('ble ')) setBle(e.data === 'ble 1');
    else if (e.data.startsWith('err ')) toast(e.data.slice(4));
  };
  ws.onclose = () => { ws = null; releaseLocal(); setTimeout(check, 800); };
}
$('loginForm').onsubmit = async e => {
  e.preventDefault();
  const r = await fetch('/api/login', {method: 'POST', headers: {'X-Rescue': '1'}, body: new URLSearchParams({pin: $('pin').value})});
  $('pin').value = '';
  if (r.ok) { $('loginMsg').textContent = ''; connect(); }
  else $('loginMsg').textContent = await r.text();
};
$('logout').onclick = async () => {
  $('setSheet').classList.add('hidden');
  await fetch('/api/logout', {method: 'POST', headers: {'X-Rescue': '1'}});
};

// ---- Sticky modifiers (one-shot: apply to the next key or click) ----
let mods = 0;
function setMods(m) {
  mods = m;
  document.querySelectorAll('[data-mod]').forEach(b => b.classList.toggle('on', !!(mods & +b.dataset.mod)));
}
function takeMods() { const m = mods; if (m) setMods(0); return m; }
document.querySelectorAll('[data-mod]').forEach(b => b.onclick = () => setMods(mods ^ +b.dataset.mod));
document.querySelectorAll('[data-key]').forEach(b => b.onclick = () =>
  send('kt ' + b.dataset.key + ' ' + ((+b.dataset.mods || 0) | takeMods())));
document.querySelectorAll('[data-cc]').forEach(b => b.onclick = () => send('cc ' + b.dataset.cc));

// ---- Mouse: accumulate locally, flush once per frame ----
let mx = 0, my = 0, sv = 0, sh = 0;
function flush() {
  if (ws && ws.readyState === 1 && ws.bufferedAmount < 512) {
    const ix = Math.trunc(mx), iy = Math.trunc(my);
    if (ix || iy) { ws.send('m ' + ix + ' ' + iy); mx -= ix; my -= iy; }
    const iv = Math.trunc(sv), ih = Math.trunc(sh);
    if (iv || ih) { ws.send('w ' + iv + ' ' + ih); sv -= iv; sh -= ih; }
  }
  requestAnimationFrame(flush);
}
requestAnimationFrame(flush);
function addMove(dx, dy) { mx += dx * settings.sens; my += dy * settings.sens; }
// Finger/wheel pixels -> wheel ticks. Default matches a trackpad with natural
// scrolling: content follows your fingers (macOS flips it again if you've turned that off).
function addScroll(dy, dx, perTick) {
  const k = settings.ssens / perTick * (settings.invert ? -1 : 1);
  sv += dy * k; sh -= dx * k;
}
function click(bits) {
  const m = takeMods();
  const codes = Object.keys(MOD_CODES).filter(b => m & b).map(b => MOD_CODES[b]);
  codes.forEach(c => send('kd ' + c));
  send('bc ' + bits);
  codes.forEach(c => send('ku ' + c));
}
document.querySelectorAll('[data-click]').forEach(b => b.onclick = () => click(+b.dataset.click));
let holding = false;
function setHold(on) {
  holding = on; $('drag').classList.toggle('on', on);
  send(on ? 'bd 1' : 'bu 1');
}
$('drag').onclick = () => setHold(!holding);

// ---- Trackpad gestures (touch) ----
const pad = $('pad');
const pts = new Map();
let g = null;  // current gesture
const TAP_MS = 280, TAP_SLOP = 10, HOLD_MS = 450;
pad.addEventListener('pointerdown', e => {
  if (e.pointerType === 'mouse') return;
  pad.setPointerCapture(e.pointerId);
  pts.set(e.pointerId, {x: e.clientX, y: e.clientY, sx: e.clientX, sy: e.clientY});
  if (!g) {
    g = {t0: performance.now(), max: 0, moved: false, dragging: false, timer: 0};
    g.timer = setTimeout(() => {
      if (g && !g.moved && pts.size === 1) {
        g.dragging = true; send('bd 1'); navigator.vibrate && navigator.vibrate(15);
      }
    }, HOLD_MS);
  }
  g.max = Math.max(g.max, pts.size);
});
pad.addEventListener('pointermove', e => {
  const p = pts.get(e.pointerId);
  if (!p || !g) return;
  const dx = e.clientX - p.x, dy = e.clientY - p.y;
  p.x = e.clientX; p.y = e.clientY;
  if (Math.hypot(p.x - p.sx, p.y - p.sy) > TAP_SLOP) { g.moved = true; clearTimeout(g.timer); }
  if (pts.size === 1) addMove(dx, dy);
  else if (pts.size === 2) addScroll(dy / 2, dx / 2, 14);
});
function pointerEnd(e) {
  if (!pts.delete(e.pointerId) || !g || pts.size) return;
  clearTimeout(g.timer);
  if (g.dragging) send('bu 1');
  else if (!g.moved && performance.now() - g.t0 < TAP_MS) click(g.max >= 3 ? 4 : g.max === 2 ? 2 : 1);
  g = null;
}
pad.addEventListener('pointerup', pointerEnd);
pad.addEventListener('pointercancel', pointerEnd);

// ---- Laptop mode: pointer lock + physical keyboard ----
const CODES = {Enter:40,Escape:41,Backspace:42,Tab:43,Space:44,Minus:45,Equal:46,BracketLeft:47,BracketRight:48,
  Backslash:49,Semicolon:51,Quote:52,Backquote:53,Comma:54,Period:55,Slash:56,CapsLock:57,PrintScreen:70,
  ScrollLock:71,Pause:72,Insert:73,Home:74,PageUp:75,Delete:76,End:77,PageDown:78,ArrowRight:79,ArrowLeft:80,
  ArrowDown:81,ArrowUp:82,NumLock:83,NumpadDivide:84,NumpadMultiply:85,NumpadSubtract:86,NumpadAdd:87,
  NumpadEnter:88,Numpad0:98,NumpadDecimal:99,IntlBackslash:100,ContextMenu:101,NumpadEqual:103,IntlRo:135,
  IntlYen:137,ControlLeft:224,ShiftLeft:225,AltLeft:226,MetaLeft:227,ControlRight:228,ShiftRight:229,
  AltRight:230,MetaRight:231};
for (let i = 0; i < 26; i++) CODES['Key' + String.fromCharCode(65 + i)] = 4 + i;
for (let i = 1; i <= 9; i++) { CODES['Digit' + i] = 29 + i; CODES['Numpad' + i] = 88 + i; }
CODES.Digit0 = 39;
for (let i = 1; i <= 12; i++) CODES['F' + i] = 57 + i;
for (let i = 13; i <= 24; i++) CODES['F' + i] = 91 + i;

let capture = false;
const down = new Set();
function setCapture(on) {
  capture = on;
  $('kbBtn').classList.toggle('on', on);
  if (!on) releaseLocal();
  updateHint();
}
function releaseLocal() {
  if (down.size || holding || (g && g.dragging)) send('ra');
  down.clear(); holding = false; $('drag').classList.remove('on');
}
document.addEventListener('keydown', e => {
  if (!capture) return;
  const c = CODES[e.code];
  if (c === undefined) return;
  e.preventDefault();
  if (e.repeat) return;  // the Mac does its own key repeat
  if (e.code === 'CapsLock') { send('kt ' + c + ' 0'); return; }
  down.add(c); send('kd ' + c);
});
document.addEventListener('keyup', e => {
  if (!capture) return;
  const c = CODES[e.code];
  if (c === undefined || e.code === 'CapsLock') return;
  e.preventDefault();
  // A Mac browser never sends keyup for keys released while Cmd is held,
  // so letting go of Cmd releases everything.
  if (e.code === 'MetaLeft' || e.code === 'MetaRight') { down.forEach(k => send('ku ' + k)); down.clear(); return; }
  down.delete(c); send('ku ' + c);
});
window.addEventListener('blur', releaseLocal);
document.addEventListener('visibilitychange', () => { if (document.hidden) releaseLocal(); });

const BTN = {0: 1, 1: 4, 2: 2};
pad.addEventListener('mousedown', e => {
  if (!FINE) return;
  if (document.pointerLockElement !== pad) { pad.requestPointerLock(); return; }
  if (BTN[e.button]) send('bd ' + BTN[e.button]);
});
pad.addEventListener('mouseup', e => {
  if (document.pointerLockElement === pad && BTN[e.button]) send('bu ' + BTN[e.button]);
});
pad.addEventListener('contextmenu', e => e.preventDefault());
document.addEventListener('mousemove', e => {
  if (document.pointerLockElement === pad) addMove(e.movementX, e.movementY);
});
pad.addEventListener('wheel', e => {
  if (document.pointerLockElement !== pad) return;
  e.preventDefault();
  const scale = e.deltaMode === 1 ? 16 : 1;
  addScroll(-e.deltaY * scale, -e.deltaX * scale, 40);
}, {passive: false});
document.addEventListener('pointerlockchange', () => {
  const locked = document.pointerLockElement === pad;
  pad.classList.toggle('locked', locked);
  if (locked) setCapture(true); else { send('ra'); setCapture(false); }
});

// ---- Phone keyboard: mirror the hidden textarea's edits as keystrokes ----
// Diffing the text (rather than reading key events) works with autocorrect,
// predictive text and swipe typing: a correction becomes backspaces + retype.
const kb = $('kb');
const SENTINEL = '    ';  // keeps Backspace working when "empty"
let last = SENTINEL, composing = false;
function resetKb() { kb.value = last = SENTINEL; kb.setSelectionRange(SENTINEL.length, SENTINEL.length); }
function syncKb() {
  const v = kb.value;
  let p = 0;
  while (p < last.length && p < v.length && last[p] === v[p]) p++;
  const del = last.length - p, add = v.slice(p);
  last = v;
  if (del) send('bs ' + del);
  if (add) {
    if (mods && add.length === 1 && !del) send('kc ' + takeMods() + ' ' + add);
    else for (let i = 0; i < add.length; i += 200) send('t ' + add.slice(i, i + 200));
  }
  if (!composing && (v.length > 64 || !v.startsWith(SENTINEL))) resetKb();
}
kb.addEventListener('input', syncKb);
kb.addEventListener('compositionstart', () => composing = true);
kb.addEventListener('compositionend', () => { composing = false; syncKb(); });
kb.addEventListener('blur', () => { if (!FINE) $('kbBtn').classList.remove('on'); });
$('kbBtn').onclick = () => {
  if (FINE) { setCapture(!capture); return; }
  if (document.activeElement === kb) { kb.blur(); return; }
  resetKb(); kb.focus(); $('kbBtn').classList.add('on');
};

// ---- Password sheet ----
$('pwBtn').onclick = () => {
  $('pwSheet').classList.remove('hidden'); $('pw').value = ''; $('pw').focus();
  if (capture) setCapture(false);
};
$('pwCancel').onclick = () => $('pwSheet').classList.add('hidden');
$('pwShow').onchange = e => $('pw').type = e.target.checked ? 'text' : 'password';
$('pwForm').onsubmit = e => {
  e.preventDefault();
  const text = $('pw').value + ($('pwEnter').checked ? '\n' : '');
  if (!text) return;
  if (text.length > 256) { toast('Too long (max 256 characters).'); return; }
  if (/[^\x20-\x7e\n]/.test(text)) toast('Some characters can’t be typed and will be skipped.');
  if (send('t ' + text)) { $('pw').value = ''; $('pwSheet').classList.add('hidden'); toast('Typed.'); }
  else toast('Not connected to the device.');
};

// ---- Settings sheet ----
$('gear').onclick = () => $('setSheet').classList.remove('hidden');
$('setDone').onclick = () => $('setSheet').classList.add('hidden');

function updateHint() {
  $('padHint').textContent = FINE
    ? (document.pointerLockElement === pad
        ? 'Controlling the Mac. Your mouse and keyboard go to it. Press Esc to stop.'
        : 'Click here to use this mouse and keyboard on the Mac')
    : 'Drag to move · tap to click · two-finger tap to right-click · two fingers to scroll · press and hold to drag';
  $('kbBtn').textContent = FINE ? (capture ? '⌨ Keyboard on' : '⌨ Keyboard') : '⌨ Keyboard';
}
// iOS doesn't shrink the page for the on-screen keyboard; keep the key bar above it.
if (window.visualViewport) {
  const fit = () => { $('app').style.bottom = Math.max(0, innerHeight - visualViewport.height - visualViewport.offsetTop) + 'px'; };
  visualViewport.addEventListener('resize', fit);
  visualViewport.addEventListener('scroll', fit);
}
updateHint();
check();
</script></body></html>)HTML";

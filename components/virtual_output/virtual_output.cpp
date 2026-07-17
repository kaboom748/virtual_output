#include "virtual_output.h"
#include "esphome/core/log.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace esphome::virtual_output {

static const char *const TAG = "virtual_output";

#if defined(USE_ESP8266)
static const char *const PLATFORM_NAME = "ESP8266";
#elif defined(USE_ESP32)
static const char *const PLATFORM_NAME = "ESP32";
#else
static const char *const PLATFORM_NAME = "generic";
#endif

static const char PAGE_HTML[] PROGMEM = R"HTMLDOC(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Virtual Output</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body { background: radial-gradient(circle at center, #2a2a2a 0%, #0a0a0a 100%); display: flex; flex-direction: column; align-items: center; justify-content: center; min-height: 100vh; font-family: sans-serif; padding: 20px; }
.main-row { display: flex; align-items: flex-start; gap: 24px; flex-wrap: wrap; justify-content: center; }
.left-col { display: flex; flex-direction: column; gap: 16px; flex: 1 1 660px; min-width: 0; max-width: 900px; }
.top-row { display: flex; align-items: flex-start; gap: 24px; }
.audio-panel { flex: 1 1 auto; min-width: 0; }
.stats-panel { flex: 0 0 310px; width: 310px; min-width: 310px; }
.graph-panel { min-width: 0; padding: 14px 14px 8px 14px; }
#scope { width: 100%; height: 300px; display: block; }
.gmeta { float: right; color: #555; letter-spacing: 0; font-size: 10px; }
.device { background: linear-gradient(145deg, #3a3a38, #1c1c1a); border-radius: 18px; padding: 22px 22px 40px 22px; box-shadow: 0 10px 30px rgba(0,0,0,0.6), inset 0 1px 1px rgba(255,255,255,0.08), inset 0 -2px 4px rgba(0,0,0,0.5); position: relative; }
.pins { text-align: center; font-size: 9px; letter-spacing: 4px; color: #888; margin-bottom: 10px; font-family: monospace; }
.can { width: 190px; height: 190px; border-radius: 50%; background: radial-gradient(circle at 35% 30%, #4a4a48, #141412); box-shadow: inset 0 0 14px rgba(0,0,0,0.9), 0 0 0 1px #444; display: flex; align-items: center; justify-content: center; position: relative; }
#disc { width: 118px; height: 118px; border-radius: 50%; background: radial-gradient(circle, #29E5FF 0%, #0d6b85 100%); box-shadow: 0 0 26px #29E5FF; opacity: 0.10; transition: opacity 0.04s linear; }
.hole { position: absolute; top: 26px; left: 50%; transform: translateX(-50%); width: 9px; height: 9px; border-radius: 50%; background: #000; box-shadow: inset 0 1px 2px rgba(255,255,255,0.15); }
#noteBig { position: absolute; bottom: 24px; left: 0; right: 0; text-align: center; color: #29E5FF; font-family: monospace; font-size: 15px; font-weight: bold; text-shadow: 0 0 8px rgba(41,229,255,0.5); }
.bracket { position: absolute; left: -14px; right: -14px; bottom: 12px; height: 10px; background: linear-gradient(180deg, #4a4a48, #222); border-radius: 4px; box-shadow: 0 2px 4px rgba(0,0,0,0.5); }
.label { text-align: center; color: #666; font-size: 11px; margin-top: 18px; letter-spacing: 1px; }
.level-box { display: flex; flex-direction: column; align-items: center; }
.level-box .cap { color: #888; font-size: 11px; margin-bottom: 8px; font-family: monospace; letter-spacing: 1px; }
.tube { width: 30px; height: 190px; background: #1a1a1a; border-radius: 6px; border: 1px solid #444; position: relative; overflow: hidden; }
#levelFill { position: absolute; bottom: 0; width: 100%; height: 0; background: linear-gradient(180deg, #29E5FF, #1a8bb0); transition: height 0.05s linear; }
.side-panel { display: flex; flex-direction: column; gap: 16px; }
.panel { background: #1a1a1a; border: 1px solid #333; border-radius: 8px; padding: 14px 18px; font-family: monospace; font-size: 12px; color: #ccc; min-width: 268px; }
.panel h3 { color: #29E5FF; font-size: 11px; letter-spacing: 2px; margin-bottom: 10px; font-weight: normal; }
.stat-row { display: flex; justify-content: space-between; padding: 3px 0; border-bottom: 1px solid #2a2a2a; gap: 12px; }
.stat-row span:first-child { color: #888; }
.stat-row span:last-child { color: #eee; font-weight: bold; text-align: right; }
.sep { height: 1px; background: #333; margin: 8px 0; }
#listen { width: 100%; background: #1a4d2e; color: #4ade80; border: 1px solid #2e7d4f; border-radius: 5px; padding: 9px; font-family: monospace; font-size: 13px; font-weight: bold; letter-spacing: 2px; cursor: pointer; }
#listen:hover { filter: brightness(1.3); }
#listen.live { background: #242424; color: #888; border-color: #3a3a3a; }
.ctl { margin-top: 10px; }
.ctl label { display: flex; justify-content: space-between; color: #888; font-size: 10px; margin-bottom: 3px; }
.ctl label b { color: #29E5FF; font-weight: normal; }
input[type=range] { width: 100%; accent-color: #29E5FF; }
.chk { display: flex; align-items: center; gap: 7px; margin-top: 11px; color: #888; font-size: 10px; }
.chk input { accent-color: #29E5FF; }
.hint { color: #666; font-size: 9.5px; margin-top: 7px; line-height: 1.45; }
</style>
</head>
<body>
<div class="main-row">
  <div class="left-col">
   <div class="top-row">
    <div class="device">
      <div class="pins">GND &nbsp; SIG</div>
      <div class="can"><div class="hole"></div><div id="disc"></div><div id="noteBig">--</div></div>
      <div class="bracket"></div>
    </div>

    <div class="level-box">
      <div class="cap">AMPL</div>
      <div class="tube"><div id="levelFill"></div></div>
    </div>

    <div class="panel audio-panel">
      <h3>AUDIO</h3>
      <button id="listen">LISTEN</button>
      <div class="ctl">
        <label>Monitor volume <b id="volLabel">30%</b></label>
        <input type="range" id="vol" min="0" max="1" step="0.01" value="0.3">
      </div>
      <div class="ctl">
        <label>Play-out buffer <b id="playoutLabel">100 ms</b></label>
        <input type="range" id="playout" min="0" max="400" step="10" value="100">
      </div>
      <div class="chk"><input type="checkbox" id="sched" checked><span>Schedule on ESP timestamp</span></div>
      <div class="hint">Off = apply on arrival. Compare by ear: if the rhythm holds either way, the link is clean and the timestamps are free insurance.</div>
    </div>
   </div>

    <div class="panel graph-panel">
      <h3>SIGNAL<span class="gmeta" id="gmeta">static  c4-b7  rtttl range</span></h3>
      <canvas id="scope"></canvas>
    </div>
  </div>

    <div class="panel stats-panel">
      <h3>NETWORK STATISTICS</h3>
      <div class="stat-row"><span>Platform</span><span id="statPlatform">-</span></div>
      <div class="stat-row"><span>Link</span><span id="statLink">-</span></div>
      <div class="stat-row"><span>Frame size</span><span id="statFrame">-</span></div>
      <div class="stat-row"><span>Beat rate / target</span><span id="statRate">-</span></div>
      <div class="stat-row"><span>Connection latency</span><span id="statLatency">-</span></div>
      <div class="stat-row"><span>Link jitter</span><span id="statJitter">-</span></div>
      <div class="stat-row"><span>ESP loop worst</span><span id="statEspMax">-</span></div>
      <div class="stat-row"><span>Throughput</span><span id="statThroughput">-</span></div>
      <div class="stat-row"><span>Events</span><span id="statEvents">0</span></div>
      <div class="stat-row"><span>Heartbeats</span><span id="statBeats">0</span></div>
      <div class="stat-row"><span>Resyncs</span><span id="statResync">0</span></div>
      <div class="stat-row"><span>ESP repairs</span><span id="statRepair">0</span></div>
      <div class="stat-row"><span>Reconnects</span><span id="statReconnect">0</span></div>
      <div class="sep"></div>
      <div class="stat-row"><span>Margin</span><span id="statMargin">-</span></div>
      <div class="stat-row"><span>Margin min</span><span id="statMinMargin">-</span></div>
      <div class="stat-row"><span>Late events</span><span id="statLate">0</span></div>
      <div class="stat-row"><span>Re-anchors</span><span id="statReanchor">0</span></div>
      <div class="sep"></div>
      <div class="stat-row"><span>Current note</span><span id="statNote">-</span></div>
      <div class="stat-row"><span>Duty / amplitude</span><span id="statDuty">-</span></div>
      <div class="stat-row"><span>Audio clock</span><span id="statAudio">not started</span></div>
    </div>
</div>
<div class="label">Virtual Output - virtual_output</div>

<script>
var SYNC = 0xAA;
var FLAG_BEAT = 0x01;
var FLAG_STATE = 0x02;
var RAMP = 0.002;
var BURST_GAP_MS = 2000;

var FRAME_SIZE = 9;
var BEAT_MS = 100;

var ctx = null, osc = null, dutyGain = null, master = null;
var aPrev = 0;

var anchorEsp = null, anchorAudio = 0, lastEventTs = 0;
var playout = 0.100;
var scheduled = true;

var nEvents = 0, nBeats = 0, nResync = 0, nReconnect = 0, nLate = 0, nReanchor = 0;
var minMargin = Infinity;
var beatTimes = [], beatIntervals = [], lastBeat = null;
var frameTimes = [];
var lastFrameAt = 0;
var stalls = 0, stalled = false, resync = false;
var lastBeatTs = null, espMax = 0, nRepair = 0, seenFirst = false, muted = false;

// The scope rides the audio clock, like the panel and the ears. Its history is
// keyed on the SCHEDULED time of each transition, not on arrival: what scrolls
// past the playhead is what you are hearing, to the sample.
var GRAPH_S = 8;
// rtttl refuses octaves outside 4..7 (MIN_OCTAVE/MAX_OCTAVE), so c4..c8 bounds
// every note it can ever emit. The axis is static because the source cannot
// leave it -- and a fixed axis means a given pitch always lands at a given
// height, which is how the ear already works.
var F_MIN = 262, F_MAX = 4186;
var LF0 = Math.log(F_MIN), LF1 = Math.log(F_MAX), LFS = LF1 - LF0;
var GRID = [[262, 'c4'], [523, 'c5'], [1047, 'c6'], [2093, 'c7'], [4186, 'c8']];
var FREEZE_MS = 250;
var hist = [], freezes = [];
var cv = null, g = null, idleDrawn = false;
// Two thresholds, not one. A stall means we no longer know what the pin is
// doing; it does not mean the pin went quiet. Muting on a hiccup invents a
// silence the buzzer never had, and chops a live melody. Only past DEAD_MS does
// a held tone become a louder lie than silence.
var STALL_MS = 1000;
var DEAD_MS = 4000;

function $(id) { return document.getElementById(id); }
function set(id, v) { $(id).textContent = v; }
function link(state) {
  var el = $('statLink');
  el.textContent = state + (stalls > 0 ? ' (' + stalls + ' stall' + (stalls > 1 ? 's' : '') + ')' : '');
  el.style.color = state === 'up' ? (stalls > 0 ? '#fbbf24' : '#4ade80') : '#ff6b6b';
}

// A PWM square wave of duty d has a fundamental amplitude of sin(pi*d): loudest
// at 50%, and SILENT at 100% -- a pin stuck high does not oscillate. Reproducing
// that is the whole point. gain: 100% must be as inaudible in this tab as it is
// on the real buzzer, or the rehearsal lies exactly where it should warn you.
function amp(d) { return Math.sin(Math.PI * (d / 255)); }

// A suspended context freezes ctx.currentTime. Everything pinned to that clock
// -- the scheduler, the scope, the display -- must test the state, not the mere
// existence of the object, or it schedules the whole melody at t=0.
function live() { return !!ctx && ctx.state === 'running'; }

function makeAudio() {
  ctx = new AudioContext();
  // One oscillator, started once, never stopped. A passive buzzer on PWM is a
  // single oscillator whose frequency register gets rewritten; silence comes
  // from the level, not from stopping the carrier. rtttl does the same thing.
  osc = new OscillatorNode(ctx, { type: 'square', frequency: 440 });
  dutyGain = new GainNode(ctx, { gain: 0 });
  master = new GainNode(ctx, { gain: 0 });
  osc.connect(dutyGain).connect(master).connect(ctx.destination);
  osc.start();
  setMaster();
}

// Muting silences the ear, not the instrument: the graph keeps scrolling, the
// panel keeps counting, the disc keeps lighting. So it rides the master gain and
// never ctx.suspend(), which would freeze the audio clock everything else is
// pinned to.
function setMaster() {
  if (!master || !ctx) return;
  var v = muted ? 0 : parseFloat($('vol').value);
  var t = ctx.currentTime;
  master.gain.cancelScheduledValues(t);
  master.gain.setValueAtTime(master.gain.value, t);
  master.gain.linearRampToValueAtTime(v, t + 0.02);
}

// The label states what a click will do, not what the page is. Two states, both
// always reachable -- there is no dead end to design around.
function paintListen() {
  var on = live() && !muted;
  var b = $('listen');
  b.textContent = on ? 'MUTE' : 'LISTEN';
  b.className = on ? 'live' : '';
}

$('listen').onclick = function () {
  var fresh = false;
  if (!ctx) { makeAudio(); fresh = true; }
  // A browser can suspend the context on its own -- device change, audio focus,
  // a backgrounded tab -- and resume() needs a gesture. Disabling this button
  // after the first click removed the only gesture left, so the page went
  // permanently mute while the panel honestly reported 'suspended'.
  // `fresh` matters: a browser may hand back an already-running context when the
  // origin's media engagement is high. Without it the very first click reads as
  // a mute and the button starts by silencing itself.
  if (fresh || ctx.state !== 'running') {
    muted = false;
    ctx.resume().then(paintListen, paintListen);
  } else {
    muted = !muted;
  }
  setMaster();
  paintListen();
};

function reanchor(ts) {
  if (anchorEsp !== null) { nReanchor++; set('statReanchor', nReanchor); }
  anchorEsp = ts;
  anchorAudio = ctx.currentTime + playout;
  lastEventTs = ts;
}

function margin(m) {
  set('statMargin', (m * 1000).toFixed(1) + ' ms');
  if (m < minMargin) {
    minMargin = m;
    var el = $('statMinMargin');
    el.textContent = (m * 1000).toFixed(1) + ' ms';
    el.style.color = m < 0.02 ? '#ff6b6b' : '#eee';
  }
}

function apply(ts, freq, duty, force) {
  if (!live()) { lastEventTs = ts; showNote(freq, duty); return; }
  var t;
  if (force || !scheduled) {
    t = ctx.currentTime;
    anchorEsp = null;
    lastEventTs = ts;
  } else {
    // Re-anchor per burst, not per elapsed time: rtttl plays a few seconds and
    // stops, so drift never accumulates inside one melody. Anchoring on time
    // since the ANCHOR instead of time since the last EVENT would re-anchor in
    // the middle of any song longer than the gap and break its rhythm.
    if (anchorEsp === null || ((ts - lastEventTs) >>> 0) > BURST_GAP_MS) reanchor(ts);
    t = anchorAudio + (((ts - anchorEsp) >>> 0) / 1000);
    var m = t - ctx.currentTime;
    margin(m);
    if (m < 0) { nLate++; set('statLate', nLate); reanchor(ts); t = anchorAudio; }
    lastEventTs = ts;
  }
  osc.frequency.setValueAtTime(Math.max(1, freq), t);
  var a = amp(duty);
  // linearRampToValueAtTime starts from the previously SCHEDULED value, not from
  // "now". Without this setValueAtTime anchor a 40 ms note becomes a 40 ms
  // glissando. And dutyGain.gain.value cannot supply it: it reports the value at
  // this instant, not the value at t. Hence aPrev, tracked by hand.
  dutyGain.gain.setValueAtTime(aPrev, t);
  dutyGain.gain.linearRampToValueAtTime(a, t + RAMP);
  aPrev = a;

  // The eyes ride the same delayed clock as the ears. This browser is a delayed
  // mirror of the pin: putting the audio on anchor+delta but painting the panel
  // the instant a frame lands leaves the display exactly one play-out buffer
  // ahead of what you hear. At 400 ms and 226 ms notes that is nearly two notes
  // of disagreement -- so raising the buffer to fix a rhythm problem made the
  // panel lie harder instead. An instrument that reads a different clock than
  // the ear is worse than no instrument.
  var lead = (t - ctx.currentTime) * 1000;
  if (lead <= 1) {
    showNote(freq, duty);
  } else {
    setTimeout(function () { showNote(freq, duty); }, lead);
  }
  hist.push({ t: t, f: freq, d: duty });
}

function mute() {
  if (!ctx) return;
  var t = ctx.currentTime;
  if (dutyGain.gain.cancelAndHoldAtTime) {
    dutyGain.gain.cancelAndHoldAtTime(t);
  } else {
    dutyGain.gain.cancelScheduledValues(t);
    dutyGain.gain.setValueAtTime(dutyGain.gain.value, t);
  }
  dutyGain.gain.linearRampToValueAtTime(0, t + 0.005);
  aPrev = 0;
  anchorEsp = null;
  showNote(0, 0);
}

function showNote(freq, duty) {
  var a = amp(duty);
  set('statNote', freq > 0 ? freq + ' Hz' : 'rest');
  set('statDuty', Math.round(duty / 255 * 100) + '% / ' + Math.round(a * 100) + '%');
  $('disc').style.opacity = (0.10 + 0.90 * a).toFixed(3);
  $('levelFill').style.height = Math.round(a * 100) + '%';
  $('noteBig').textContent = (duty > 0 && freq > 0) ? freq + ' Hz' : '--';
}

function onFrame(b) {
  var flags = b[1];
  // millis() rolls over at 49.7 days. C++ gets the wraparound for free on
  // unsigned subtraction; JS numbers are doubles, so >>> 0 buys it back.
  var ts = (b[2] | (b[3] << 8) | (b[4] << 16) | (b[5] << 24)) >>> 0;
  var freq = b[6] | (b[7] << 8);
  var duty = b[8];
  var now = performance.now();
  lastFrameAt = now;
  // A stall is an event, not a state. Latching the label on 'stalled' forever
  // buries the recovery; clearing it silently buries the stall. Show the live
  // state and keep the count.
  if (stalled) {
    stalled = false;
    link('up');
  }

  frameTimes.push(now);
  frameTimes = frameTimes.filter(function (t) { return now - t < 2000; });
  set('statThroughput', Math.round(frameTimes.length * FRAME_SIZE / 2) + ' B/s');

  if (flags & FLAG_BEAT) {
    nBeats++; set('statBeats', nBeats);
    if (lastBeat !== null && lastBeatTs !== null) {
      // Every beat is stamped on the ESP, before the network can touch it. So the
      // ts delta is the ESP's own loop period and the arrival delta is that plus
      // the link. Their difference names which of the two is at fault instead of
      // blaming whichever is nearer to hand -- measuring arrivals alone reports
      // a frozen ESP as network jitter.
      var espDelta = (ts - lastBeatTs) >>> 0;
      if (espDelta > espMax) {
        espMax = espDelta;
        var em = $('statEspMax');
        em.textContent = espDelta + ' ms';
        em.style.color = espDelta > 300 ? '#ff6b6b' : (espDelta > 150 ? '#fbbf24' : '#4ade80');
      }
      if (espDelta > FREEZE_MS && ctx && anchorEsp !== null && scheduled) {
        var te = anchorAudio + (((ts - anchorEsp) >>> 0) / 1000);
        freezes.push({ t0: te - espDelta / 1000, t1: te });
      }
      beatIntervals.push((now - lastBeat) - espDelta);
      if (beatIntervals.length > 30) beatIntervals.shift();
      var avg = 0, i;
      for (i = 0; i < beatIntervals.length; i++) avg += beatIntervals[i];
      avg /= beatIntervals.length;
      var v = 0;
      for (i = 0; i < beatIntervals.length; i++) v += (beatIntervals[i] - avg) * (beatIntervals[i] - avg);
      var j = Math.sqrt(v / beatIntervals.length);
      var el = $('statJitter');
      el.textContent = j.toFixed(1) + ' ms';
      lastBeatTs = ts;
      // Now that the ESP's own period is subtracted out, this is the link and
      // nothing else: a frozen ESP no longer shows up here as network jitter.
      el.style.color = j > 30 ? '#ff6b6b' : (j > 10 ? '#fbbf24' : '#4ade80');
    }
    lastBeat = now;
    lastBeatTs = ts;
    beatTimes.push(now);
    beatTimes = beatTimes.filter(function (t) { return now - t < 2000; });
    set('statRate', (beatTimes.length / 2).toFixed(1) + ' / ' + (1000 / BEAT_MS).toFixed(1) + ' /s');
    // Free, continuous read of anchor health -- but a heartbeat must never
    // schedule audio: it carries the emit time, not a note's time.
    if (ctx && anchorEsp !== null && scheduled) {
      margin(anchorAudio + (((ts - anchorEsp) >>> 0) / 1000) - ctx.currentTime);
    }
  } else {
    nEvents++; set('statEvents', nEvents);
  }

  // A heartbeat normally never touches audio: it carries the emit time, not a
  // note's. But after a stall it is the fastest truth available -- 100 ms away
  // instead of up to a whole half note -- and it carries the full state. That is
  // what FLAG_STATE was for; a local stall earns the same treatment.
  // The ESP only flags authoritative state when its queue overflowed and a
  // transition was thrown away. Counting it here is the only way that loss ever
  // becomes visible: it was silent before, and a lost transition looks exactly
  // like a held note.
  if ((flags & FLAG_STATE) && seenFirst) { nRepair++; set('statRepair', nRepair); }
  seenFirst = true;

  if ((flags & FLAG_STATE) || resync) {
    resync = false;
    apply(ts, freq, duty, true);
  } else if (!(flags & FLAG_BEAT)) {
    apply(ts, freq, duty, false);
  }
}

async function stream() {
  var t0 = performance.now();
  try {
    var res = await fetch('/events');
    set('statLatency', (performance.now() - t0).toFixed(1) + ' ms');
    FRAME_SIZE = parseInt(res.headers.get('X-Frame-Size') || '9', 10);
    BEAT_MS = parseInt(res.headers.get('X-Heartbeat-Ms') || '100', 10);
    set('statPlatform', res.headers.get('X-Platform') || 'unknown');
    set('statFrame', FRAME_SIZE + ' B');
    link('up');

    var reader = res.body.getReader();
    var buf = new Uint8Array(0);
    while (true) {
      var r = await reader.read();
      if (r.done) break;
      var nb = new Uint8Array(buf.length + r.value.length);
      nb.set(buf); nb.set(r.value, buf.length);
      buf = nb;
      while (buf.length >= FRAME_SIZE) {
        if (buf[0] !== SYNC) {
          var i = 1;
          while (i < buf.length && buf[i] !== SYNC) i++;
          if (i >= buf.length) { buf = new Uint8Array(0); break; }
          // Never normal here. oled_stream resynced routinely because a short
          // write only shifted pixels; a shifted frame here is a wrong note, so
          // the ESP closes on short write instead. Any resync is a bug.
          nResync++;
          $('statResync').textContent = nResync;
          $('statResync').style.color = '#ff6b6b';
          buf = buf.slice(i);
          continue;
        }
        onFrame(buf.subarray(0, FRAME_SIZE));
        buf = buf.slice(FRAME_SIZE);
      }
    }
  } catch (e) {
    console.error('stream ended', e);
  }
  stalled = false;
  link('down');
  mute();
  nReconnect++; set('statReconnect', nReconnect);
  setTimeout(stream, 300);
}

// Half-open TCP never finishes the reader. The heartbeat is the liveness test on
// this side, exactly as a failed write is on the ESP side.
setInterval(function () {
  if (lastFrameAt) {
    var gap = performance.now() - lastFrameAt;
    if (gap > DEAD_MS) {
      mute();
      resync = true;
      link('dead');
      lastFrameAt = 0;
    } else if (gap > STALL_MS && !stalled) {
      // Deliberately no mute. The buzzer did not stop ringing because the WiFi
      // hiccuped, so holding the last known state is a better guess than
      // inventing silence. Flag it, count it, and let the next frame -- beat or
      // event -- carry the truth back.
      stalled = true;
      stalls++;
      resync = true;
      link('stalled');
    }
  }
  set('statAudio', ctx ? ctx.state : 'not started');
  paintListen();
}, 200);

$('vol').oninput = function (e) {
  set('volLabel', Math.round(e.target.value * 100) + '%');
  setMaster();
};
$('playout').oninput = function (e) {
  playout = parseInt(e.target.value, 10) / 1000;
  set('playoutLabel', e.target.value + ' ms');
  minMargin = Infinity; set('statMinMargin', '-');
};
$('sched').onchange = function (e) {
  scheduled = e.target.checked;
  anchorEsp = null;
  minMargin = Infinity; set('statMinMargin', '-');
};

function draw() {
  requestAnimationFrame(draw);
  if (!cv) { cv = $('scope'); if (!cv) return; g = cv.getContext('2d'); }
  if (!live() && idleDrawn) return;
  idleDrawn = !live();

  var dpr = window.devicePixelRatio || 1;
  var W = cv.clientWidth, H = cv.clientHeight;
  if (!W || !H) return;
  if (cv.width !== Math.round(W * dpr) || cv.height !== Math.round(H * dpr)) {
    cv.width = Math.round(W * dpr); cv.height = Math.round(H * dpr);
  }
  g.setTransform(dpr, 0, 0, dpr, 0, 0);
  g.clearRect(0, 0, W, H);

  var L = 92, R = 10, AXH = 16, AH = 34, GAP = 10, TOP = 6;
  var pw = W - L - R, fh = H - TOP - AXH - AH - GAP;
  if (pw < 60 || fh < 60) return;
  var fy0 = TOP, fy1 = TOP + fh, ay0 = fy1 + GAP, ay1 = ay0 + AH;

  // The buffer widens the drawn span, so px/s shifts only when the slider moves.
  // Anything else would either clip the buffer or hide it.
  var span = GRAPH_S + playout;
  var now = live() ? ctx.currentTime : 0;
  var t0 = now - GRAPH_S, px = pw / span;
  var tx = function (t) { return L + (t - t0) * px; };
  var fy = function (f) {
    var c = Math.min(F_MAX, Math.max(F_MIN, f || F_MIN));
    return fy0 + (LF1 - Math.log(c)) / LFS * fh;
  };

  var cut = now - GRAPH_S - 0.5, i;
  // Keep one event older than the window: its value holds into view.
  while (hist.length > 1 && hist[1].t < cut) hist.shift();
  while (freezes.length && freezes[0].t1 < cut) freezes.shift();

  g.fillStyle = 'rgba(255,107,107,0.10)';
  for (i = 0; i < freezes.length; i++) {
    var b0 = Math.max(L, tx(freezes[i].t0)), b1 = Math.min(L + pw, tx(freezes[i].t1));
    if (b1 > b0) g.fillRect(b0, fy0, b1 - b0, ay1 - fy0);
  }
  var hx = tx(now);
  if (L + pw > hx) {
    g.fillStyle = 'rgba(41,229,255,0.07)';
    g.fillRect(hx, fy0, L + pw - hx, ay1 - fy0);
  }

  g.font = '10px monospace';
  g.textBaseline = 'middle';
  for (i = 0; i < GRID.length; i++) {
    var y = Math.round(fy(GRID[i][0])) + 0.5;
    g.strokeStyle = '#2c2c2a'; g.lineWidth = 1;
    g.beginPath(); g.moveTo(L, y); g.lineTo(L + pw, y); g.stroke();
    g.textAlign = 'right';
    g.fillStyle = '#888'; g.fillText(GRID[i][1], L - 36, y);
    g.fillStyle = '#555'; g.fillText(String(GRID[i][0]), L - 8, y);
  }
  g.strokeStyle = '#2c2c2a'; g.lineWidth = 1;
  g.beginPath(); g.moveTo(L, ay1 + 0.5); g.lineTo(L + pw, ay1 + 0.5); g.stroke();
  g.textAlign = 'right'; g.fillStyle = '#555';
  g.fillText('amp', L - 8, (ay0 + ay1) / 2);

  if (!live()) {
    g.textAlign = 'center'; g.fillStyle = '#444'; g.font = '11px monospace';
    g.fillText('click anywhere to listen - this graph rides the audio clock', L + pw / 2, (fy0 + fy1) / 2);
    return;
  }

  if (hist.length) {
    var step = function (get) {
      g.beginPath();
      for (var k = 0; k < hist.length; k++) {
        var xa = tx(hist[k].t);
        var xb = (k + 1 < hist.length) ? tx(hist[k + 1].t) : tx(now + playout);
        var yv = get(hist[k]);
        if (k === 0) g.moveTo(xa, yv); else g.lineTo(xa, yv);
        g.lineTo(xb, yv);
      }
    };
    g.save();
    g.beginPath(); g.rect(L, fy0 - 2, pw, ay1 - fy0 + 4); g.clip();

    step(function (e) { return ay1 - amp(e.d) * AH; });
    g.lineTo(tx(now + playout), ay1); g.lineTo(tx(hist[0].t), ay1); g.closePath();
    g.fillStyle = 'rgba(41,229,255,0.18)'; g.fill();
    step(function (e) { return ay1 - amp(e.d) * AH; });
    g.strokeStyle = '#29E5FF'; g.lineWidth = 1.5; g.stroke();

    // Steps, never a spline: rtttl writes a register, it does not glide.
    step(function (e) { return fy(e.f); });
    g.strokeStyle = '#29E5FF'; g.lineWidth = 2; g.lineJoin = 'round'; g.stroke();
    g.restore();
  }

  g.strokeStyle = '#29E5FF'; g.lineWidth = 1; g.setLineDash([3, 3]);
  g.beginPath(); g.moveTo(Math.round(hx) + 0.5, fy0); g.lineTo(Math.round(hx) + 0.5, ay1 + 4); g.stroke();
  g.setLineDash([]);

  g.font = '10px monospace'; g.textBaseline = 'top';
  g.textAlign = 'center'; g.fillStyle = '#666';
  for (var k2 = GRAPH_S; k2 >= 2; k2 -= 2) g.fillText('-' + k2 + 's', tx(now - k2), ay1 + 4);
  g.textAlign = 'right'; g.fillStyle = '#29E5FF'; g.fillText('now', hx - 3, ay1 + 4);
  if (playout > 0.02) {
    g.textAlign = 'left'; g.fillStyle = '#4a9fb5'; g.fillText('buf', hx + 4, ay1 + 4);
  }
}

// Ask at load instead of waiting for the button. Chrome hands back a suspended
// context unless this origin's media engagement index has been crossed -- and
// Web Audio use feeds that index -- in which case it comes back running and the
// page is already listening on a reload. Creating the context only inside the
// click meant the question was never put, which guarantees the answer is no.
makeAudio();
ctx.resume().then(paintListen, paintListen);
paintListen();

// The policy wants a gesture, not this particular button. Any click or key on
// the page is one, so the button is the discoverable affordance rather than the
// only door. The button itself is skipped: its own handler would otherwise read
// the same click as a mute.
function onGesture(e) {
  if (e.target && e.target.id === 'listen') return;
  if (ctx && ctx.state !== 'running') ctx.resume().then(paintListen, paintListen);
}
document.addEventListener('click', onGesture);
document.addEventListener('keydown', onGesture);

window.addEventListener('resize', function () { idleDrawn = false; });
draw();
stream();
</script>
</body>
</html>
)HTMLDOC";

// ===================================================================
// FloatOutput surface: two calls, both timestamped where they happen
// ===================================================================

void VirtualOutput::update_frequency(float frequency) {
  const uint16_t f = (uint16_t) clamp(frequency, 0.0f, 65535.0f);
  if (f == this->freq_)
    return;
  this->freq_ = f;
  this->push_(this->freq_, this->duty_);
}

void VirtualOutput::write_state(float state) {
  const uint8_t d = (uint8_t) lroundf(clamp(state, 0.0f, 1.0f) * 255.0f);
  if (d == this->duty_)
    return;
  this->duty_ = d;
  this->push_(this->freq_, this->duty_);
}

void VirtualOutput::push_(uint16_t freq, uint8_t duty) {
  // Nobody listening: the state above is still current, so there is nothing to
  // queue. A real buzzer does not stop being driven because you left the room,
  // and rtttl must never feel this component -- no backpressure, ever.
  if (!this->stream_client_)
    return;

  const uint32_t now = millis();

  // Same millisecond as the tail: this is one musical instant expressed as two
  // API calls (update_frequency then set_level). Fuse them. The timestamp IS the
  // coalescing key, which is why the 10 ms gap rtttl inserts between two
  // identical notes survives: its two set_level() calls are 10 ms apart.
  if (this->queue_count_ > 0) {
    Event &tail = this->queue_[(this->queue_head_ + this->queue_count_ - 1) % QUEUE_LEN];
    if (tail.ts == now) {
      tail.freq = freq;
      tail.duty = duty;
      return;
    }
  }

  if (this->queue_count_ >= QUEUE_LEN) {
    // We have not run in 8 note boundaries. The timing of this transition is
    // lost, so ask the next heartbeat to carry authoritative state instead of
    // pretending. Visible, countable, self-healing within one beat.
    this->dropped_++;
    this->state_dirty_ = true;
    return;
  }

  Event &e = this->queue_[(this->queue_head_ + this->queue_count_) % QUEUE_LEN];
  e.ts = now;
  e.freq = freq;
  e.duty = duty;
  this->queue_count_++;
}

// ===================================================================
// Component
// ===================================================================

void VirtualOutput::setup() {
  // One socket path for both targets. ESPHome's socket component covers ESP8266
  // through its lwip_tcp implementation, so the WiFiServer/socket::Socket split
  // oled_stream carries is not needed. Note that on lwip_tcp, ListenSocket and
  // Socket are distinct types: a plain socket_ip() cannot listen() there.
  this->server_ = socket::socket_ip_loop_monitored(SOCK_STREAM, 0);
  if (!this->server_) {
    ESP_LOGE(TAG, "Could not create socket (errno=%d)", errno);
    this->mark_failed();
    return;
  }
  // Not optional. A blocking accept() parks the whole ESPHome main loop on the
  // next inbound connection: no API, no OTA, no rtttl, nothing -- and the device
  // looks alive the entire time. Config validation and -Wall both pass happily.
  if (this->server_->setblocking(false) != 0) {
    ESP_LOGE(TAG, "Could not set non-blocking (errno=%d)", errno);
    this->mark_failed();
    return;
  }

  struct sockaddr_storage server_addr;
  socklen_t sl = socket::set_sockaddr_any((struct sockaddr *) &server_addr, sizeof(server_addr), this->port_);
  if (sl == 0) {
    ESP_LOGE(TAG, "Could not set sockaddr");
    this->mark_failed();
    return;
  }
  if (this->server_->bind((struct sockaddr *) &server_addr, sl) != 0) {
    ESP_LOGE(TAG, "Could not bind port %u (errno=%d)", this->port_, errno);
    this->mark_failed();
    return;
  }
  if (this->server_->listen(2) != 0) {
    ESP_LOGE(TAG, "Could not listen (errno=%d)", errno);
    this->mark_failed();
    return;
  }
  ESP_LOGI(TAG, "Listening on port %u", this->port_);
}

void VirtualOutput::dump_config() {
  ESP_LOGCONFIG(TAG, "Virtual Output:");
  ESP_LOGCONFIG(TAG, "  Port: %u", this->port_);
  ESP_LOGCONFIG(TAG, "  Frame: %u bytes, heartbeat: %u ms", (unsigned) FRAME_SIZE, (unsigned) HEARTBEAT_MS);
  LOG_FLOAT_OUTPUT(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Setup failed: no browser can attach, rtttl still plays");
  }
}

void VirtualOutput::loop() {
  const uint32_t now = millis();

  this->accept_client_(now);
  if (this->html_sending_) {
    this->pump_html_(now);
  } else {
    this->read_request_(now);
  }

  if (!this->stream_client_) {
    this->queue_count_ = 0;
    return;
  }

  // Drain every queued transition, each with its own timestamp. 8 frames is
  // 72 bytes: never worth splitting across loop passes, and splitting would
  // delay a note behind a heartbeat.
  //
  // Note that an event does NOT reset the heartbeat phase, though it proves
  // liveness just as well and would save a frame. The heartbeat is a measuring
  // instrument: letting the music suppress beats makes its rate and period dip
  // during playback -- exactly when the panel is being read -- and a metronome
  // that slows down when the thing it measures gets busy measures nothing.
  while (this->queue_count_ > 0) {
    const Event e = this->queue_[this->queue_head_];
    if (!this->send_frame_(0, e.ts, e.freq, e.duty))
      return;
    this->queue_head_ = (uint8_t) ((this->queue_head_ + 1) % QUEUE_LEN);
    this->queue_count_--;
  }

  if (now - this->last_beat_ >= HEARTBEAT_MS) {
    const uint8_t flags = (uint8_t) (FLAG_BEAT | (this->state_dirty_ ? FLAG_STATE : 0));
    if (this->send_frame_(flags, now, this->freq_, this->duty_))
      this->state_dirty_ = false;
    // Advance the phase; do not restart it. `last_beat_ = now` makes the real
    // period HEARTBEAT_MS *plus* whatever remains until the next loop pass, so a
    // perfectly healthy 16 ms loop reports 8.9 against a 10.0 target and the
    // panel's own reference becomes useless as a health indicator.
    this->last_beat_ += HEARTBEAT_MS;
    if (now - this->last_beat_ >= HEARTBEAT_MS)
      this->last_beat_ = now;  // over a full beat behind: resync rather than burst
  }
}

// ===================================================================
// Network
// ===================================================================

void VirtualOutput::accept_client_(uint32_t now) {
  if (!this->server_ || this->pending_client_)
    return;
  auto client = this->server_->accept(nullptr, nullptr);
  if (!client)
    return;
  client->setblocking(false);
  this->pending_client_ = std::move(client);
  this->request_len_ = 0;
  this->request_[0] = '\0';
  this->request_total_ = 0;
  this->header_eol_ = false;
  this->request_start_ = now;
  this->html_sending_ = false;
  this->html_offset_ = 0;
}

void VirtualOutput::read_request_(uint32_t now) {
  if (!this->pending_client_)
    return;

  // socket.h states the contract: once ready() reports data, read until it would
  // block or ready() stops reporting new data. So drain, do not read once.
  uint8_t buf[64];
  while (true) {
    const ssize_t len = this->pending_client_->read(buf, sizeof(buf));
    if (len == 0) {
      this->drop_pending_();
      return;
    }
    if (len < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;
      this->drop_pending_();
      return;
    }
    for (ssize_t i = 0; i < len; i++) {
      const char c = (char) buf[i];
      this->request_total_++;
      // Only the head of the request is kept; a browser sends ~500 bytes of
      // headers, so no fixed buffer holds the whole thing plus its terminator.
      if (this->request_len_ < REQUEST_LINE_SIZE - 1) {
        this->request_[this->request_len_++] = c;
        this->request_[this->request_len_] = '\0';
      }
      if (c == '\n') {
        if (this->header_eol_) {
          this->handle_request_(now);
          return;
        }
        this->header_eol_ = true;
      } else if (c != '\r') {
        this->header_eol_ = false;
      }
    }
    if (this->request_total_ >= REQUEST_MAX_BYTES) {
      this->drop_pending_();
      return;
    }
  }
  // Subtracting is safe across the 49.7 day millis() rollover.
  if (now - this->request_start_ > REQUEST_TIMEOUT_MS)
    this->drop_pending_();
}

void VirtualOutput::handle_request_(uint32_t now) {
  if (strstr(this->request_, "GET /events") != nullptr) {
    this->start_stream_(now);
  } else {
    static const char HEADERS[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
    this->pending_client_->write(HEADERS, sizeof(HEADERS) - 1);
    this->html_sending_ = true;
    this->html_offset_ = 0;
    this->request_start_ = now;
  }
}

void VirtualOutput::start_stream_(uint32_t now) {
  if (this->stream_client_)
    this->drop_stream_();

  // The ESP8266 branch of oled_stream calls setNoDelay(true); its ESP32 branch
  // never does. Every frame here is 9 bytes, so all of them are sub-MSS: Nagle
  // would hold each note until the previous ACK returns, and under the default
  // ESP32 power_save_mode that ACK is a beacon interval away.
  int enable = 1;
  if (this->pending_client_->setsockopt(IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) != 0)
    ESP_LOGW(TAG, "TCP_NODELAY failed (errno=%d), expect jitter", errno);

  // snprintf on a stack buffer: integer-to-text plus string concatenation would
  // allocate on the heap, which ESPHome forbids on long-running devices.
  char headers[320];
  const int len = snprintf(headers, sizeof(headers),
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: application/octet-stream\r\n"
                           "Cache-Control: no-store\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Access-Control-Expose-Headers: X-Frame-Size, X-Heartbeat-Ms, X-Platform\r\n"
                           "X-Frame-Size: %u\r\n"
                           "X-Heartbeat-Ms: %u\r\n"
                           "X-Platform: %s\r\n"
                           "Connection: keep-alive\r\n\r\n",
                           (unsigned) FRAME_SIZE, (unsigned) HEARTBEAT_MS, PLATFORM_NAME);
  this->pending_client_->write(headers, len);

  this->stream_client_ = std::move(this->pending_client_);
  this->queue_count_ = 0;
  this->last_beat_ = now;

  // The newcomer gets the truth before it gets any transition. Same frame the
  // heartbeat uses, flagged authoritative: apply now, do not schedule.
  this->send_frame_((uint8_t) (FLAG_BEAT | FLAG_STATE), now, this->freq_, this->duty_);
  this->state_dirty_ = false;
  ESP_LOGD(TAG, "Stream client attached");
}

void VirtualOutput::pump_html_(uint32_t now) {
  if (!this->pending_client_)
    return;
  const size_t html_len = sizeof(PAGE_HTML) - 1;
  char chunk[HTML_CHUNK_SIZE];

  // Bounded work per pass. The page is ~14 kB and the socket is non-blocking:
  // lwip's write() returns EWOULDBLOCK on a full send buffer and short-writes
  // whatever fits otherwise, so one blast would truncate the page. Sending it a
  // byte at a time, as an earlier version of oled_stream did, meant 15584 calls
  // into the TCP stack from inside loop().
  for (uint8_t n = 0; n < HTML_CHUNKS_PER_LOOP && this->html_offset_ < html_len; n++) {
    const size_t len = std::min((size_t) HTML_CHUNK_SIZE, html_len - this->html_offset_);
    progmem_memcpy(chunk, PAGE_HTML + this->html_offset_, len);
    const ssize_t w = this->pending_client_->write(chunk, len);
    if (w < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;
      this->drop_pending_();
      return;
    }
    this->html_offset_ += (size_t) w;
    if ((size_t) w < len)
      break;
  }

  if (this->html_offset_ >= html_len || now - this->request_start_ > HTML_TIMEOUT_MS)
    this->drop_pending_();
}

bool VirtualOutput::send_frame_(uint8_t flags, uint32_t ts, uint16_t freq, uint8_t duty) {
  uint8_t f[FRAME_SIZE];
  f[0] = SYNC_BYTE;
  f[1] = flags;
  f[2] = (uint8_t) ts;
  f[3] = (uint8_t) (ts >> 8);
  f[4] = (uint8_t) (ts >> 16);
  f[5] = (uint8_t) (ts >> 24);
  f[6] = (uint8_t) freq;
  f[7] = (uint8_t) (freq >> 8);
  f[8] = duty;

  const ssize_t written = this->stream_client_->write(f, FRAME_SIZE);
  if (written == (ssize_t) FRAME_SIZE)
    return true;

  // Anything short splits a frame across the sync boundary and every later note
  // decodes from the wrong offset. oled_stream absorbed that with a resync
  // counter because a shifted frame was only shifted pixels; here it is a wrong
  // note. Closing costs one 300 ms reconnect, and the browser gets authoritative
  // state on the way back in.
  ESP_LOGW(TAG, "Short write (%d/%u), dropping client", (int) written, (unsigned) FRAME_SIZE);
  this->drop_stream_();
  return false;
}

void VirtualOutput::drop_pending_() {
  if (this->pending_client_) {
    this->pending_client_->close();
    this->pending_client_ = nullptr;
  }
  this->html_sending_ = false;
  this->html_offset_ = 0;
}

void VirtualOutput::drop_stream_() {
  if (this->stream_client_) {
    this->stream_client_->close();
    this->stream_client_ = nullptr;
  }
  this->queue_count_ = 0;
}

}  // namespace esphome::virtual_output

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
function ss(ms) {
  ms = Math.max(0, Math.floor(Number(ms) || 0));
  const s = Math.floor((ms % 60000) / 1000);
  return `${String(s).padStart(2, "0")}`;
}

function mss(ms) {
  ms = Math.max(0, Math.floor(Number(ms) || 0));
  const m = Math.floor(ms / 60000);
  const s = Math.floor((ms % 60000) / 1000);
  return `${String(m).padStart(1, "0")}:${String(s).padStart(2, "0")}`;
}

function mmss(ms) {
  ms = Math.max(0, Math.floor(Number(ms) || 0));
  const m = Math.floor(ms / 60000);
  const s = Math.floor((ms % 60000) / 1000);
  return `${String(m).padStart(2, "0")}:${String(s).padStart(2, "0")}`;
}

function hhmmss(ms) {
  ms = Math.max(0, Math.floor(Number(ms) || 0));
  const h = Math.floor(ms / 3600000);
  const m = Math.floor((ms % 3600000) / 60000);
  const s = Math.floor((ms % 60000) / 1000);
  return `${String(h).padStart(2, "0")}:${String(m).padStart(2, "0")}:${String(s).padStart(2, "0")}`;
}

async function fetchState() {
  const res = await fetch("plugin.json", { cache: "no-store" });
  if (!res.ok) throw new Error("fetch failed");
  return await res.json();
}

function normalizeIncomingState(payload) {
  if (!payload) return null;
  if (payload.type === "state" && payload.state) return payload.state;
  if (payload.home || payload.guest || payload.custom_fields) return payload;
  return null;
}

function liveTimerMs(timer, bt) {
  if (!timer) return 0;

  const mode = timer.mode || "countdown";
  const running = !!timer.running;
  const baseRemaining = Number(timer.current_moment_ms ?? 0);
  const lastTick = Number(bt.last_tick_ms ?? 0);

  // If not running or no tick info, just trust current_moment_ms
  if (!running || !lastTick || !bt.running) {
    return baseRemaining;
  }

  const now = Date.now();
  const delta = now - lastTick;

  if (mode === "countup") {
    return baseRemaining + delta;
  }

  const v = baseRemaining - delta;
  return v < 0 ? 0 : v;
}

// -----------------------------------------------------------------------------
// Expression engine
// -----------------------------------------------------------------------------
function resolvePath(obj, path) {
  if (!obj || !path) return undefined;

  const re = /([^[.\]]+)|\[(\d+)\]/g;
  let cur = obj;
  let m;

  while ((m = re.exec(path)) !== null) {
    if (m[1]) {
      cur = cur ? cur[m[1]] : undefined;
    } else if (m[2]) {
      const idx = Number(m[2]);
      if (!Array.isArray(cur)) return undefined;
      cur = cur[idx];
    }
    if (cur == null) return cur;
  }

  return cur;
}

function stripQuotes(s) {
  if (!s) return s;
  s = s.trim();
  if (
    (s.startsWith('"') && s.endsWith('"')) ||
    (s.startsWith("'") && s.endsWith("'"))
  ) {
    return s.slice(1, -1);
  }
  return s;
}

function isTruthyValue(val) {
  if (val === null || val === undefined) return false;
  if (typeof val === "string") return val.trim() !== "";
  return !!val;
}

function isValue(val) {
  if (val === null || val === undefined) return 0;
  if (typeof val === "number") return true;
  return !!val;
}

function intToHex(colorInt) {
  if (colorInt === undefined || colorInt === null) return "#555555";
  const hex = Number(colorInt).toString(16).padStart(6, "0");
  return "#" + hex;
}

function fieldKey(label, index) {
  // const canonical = ["score", "touts", "f2min", "s2min", "t2min", "discq"];
  const canonical = ["score", "fouls", "yellow_cards", "red_cards", "corners"];

  if (canonical[index]) return canonical[index];
  const normalized = String(label || "")
    .trim()
    .toLowerCase()
    .replace(/[^a-z0-9]+/g, "_")
    .replace(/^_+|_+$/g, "");
  return normalized || `field_${index}`;
}

/**
 * Evaluate an expression used inside {{ ... }} for text/attributes.
 * Supports:
 *   path                → team_x.logo, timers[0].mmss, fields_xy[1].x, etc.
 *   cond ? 'a' : 'b'    → swap_sides ? 'foo' : 'bar'
 *   !path               → returns boolean true/false, but will get stringified when used
 */
function evaluateExpression(expr, data) {
  if (!expr) return "";

  expr = expr.trim();

  // Ternary: cond ? 'a' : 'b'
  const ternaryMatch = expr.match(
    /^(.+?)\s*\?\s*(['"].*?['"])\s*:\s*(['"].*?['"])$/
  );
  if (ternaryMatch) {
    const condExpr = ternaryMatch[1].trim();
    const trueLit = stripQuotes(ternaryMatch[2]);
    const falseLit = stripQuotes(ternaryMatch[3]);

    const ok = evaluateCondition(condExpr, data);
    return ok ? trueLit : falseLit;
  }

  // Simple boolean: !path (returns true/false)
  if (expr.startsWith("!")) {
    const val = resolvePath(data, expr.slice(1).trim());
    return !isTruthyValue(val);
  }

  // Simple path
  const v = resolvePath(data, expr);
  return v == null ? "" : v;
}

/**
 * Advanced boolean evaluator for fs-if and ternary conditions.
 * Supports:
 *   - path (fields_xy[2].visible)
 *   - !path
 *   - &&, ||
 *   - parentheses (...)
 * Precedence: ! > && > ||
 */
function evaluateCondition(expr, data) {
  if (!expr) return false;
  expr = expr.trim();
  if (!expr) return false;

  // ---------------------------
  // Tokenizer
  // ---------------------------
  function tokenizeBoolExpr(s) {
    const tokens = [];
    let i = 0;

    while (i < s.length) {
      const c = s[i];

      if (/\s/.test(c)) {
        i++;
        continue;
      }

      if (c === "(" || c === ")") {
        tokens.push(c);
        i++;
        continue;
      }

      if (c === "!") {
        tokens.push("!");
        i++;
        continue;
      }

      if (s.startsWith("&&", i)) {
        tokens.push("&&");
        i += 2;
        continue;
      }

      if (s.startsWith("||", i)) {
        tokens.push("||");
        i += 2;
        continue;
      }

      if (s.startsWith(">", i)) {
        tokens.push(">");
        i++;
        continue;
      }

      if (s.startsWith("<", i)) {
        tokens.push("<");
        i++;
        continue;
      }

      // Path token: read until whitespace or operator/parens
      const start = i;
      while (i < s.length) {
        if (/\s/.test(s[i])) break;
        if (s.startsWith("&&", i) || s.startsWith("||", i)) break;
        if (s[i] === "!" || s[i] === "(" || s[i] === ")") break;
        if (s[i] === ">" || s[i] === "<") break;
        i++;
      }
      tokens.push(s.slice(start, i));
    }

    return tokens;
  }

  const tokens = tokenizeBoolExpr(expr);

  // ---------------------------
  // Shunting-yard to RPN
  // ---------------------------
  const prec = { "!": 3, "&&": 2, "||": 1, "<": 0, ">": 0 };
  const rightAssoc = { "!": true };

  const out = [];
  const ops = [];

  for (const t of tokens) {
    if (t === "(") {
      ops.push(t);
      continue;
    }

    if (t === ")") {
      while (ops.length && ops[ops.length - 1] !== "(") {
        out.push(ops.pop());
      }
      if (ops.length && ops[ops.length - 1] === "(") ops.pop();
      continue;
    }

    if (t === "!" || t === "&&" || t === "||" || t === ">" || t === "<") {
      while (ops.length) {
        const top = ops[ops.length - 1];
        if (!(top in prec)) break;

        const shouldPop = rightAssoc[t]
          ? prec[top] > prec[t]
          : prec[top] >= prec[t];

        if (!shouldPop) break;
        out.push(ops.pop());
      }
      ops.push(t);
      continue;
    }

    // path/value token
    out.push(t);
  }

  while (ops.length) out.push(ops.pop());

  // ---------------------------
  // Evaluate RPN
  // ---------------------------
  const stack = [];

  for (const tok of out) {
    if (tok === "!") {
      const a = stack.pop() || false;
      stack.push(!a);
      continue;
    }

    if (tok === "&&" || tok === "||") {
      const b = stack.pop() || false;
      const a = stack.pop() || false;
      stack.push(tok === "&&" ? (a && b) : (a || b));
      continue;
    }

    if (tok === ">" || tok === "<") {
      const b = stack.pop() || false;
      const a = stack.pop() || false;
      stack.push(tok === ">" ? (a > b) : (a < b));
      continue;
    }

    // resolve path -> truthy
    const val = resolvePath(data, tok);
    if (isValue(tok) && !val && val != 0) {
      stack.push(parseInt(tok));
    } else if (!!val || val == 0) {
      stack.push(val);
    } else {
      stack.push(isTruthyValue(val));
    }
  }

  return stack.pop() || false;
}

// -----------------------------------------------------------------------------
// Template engine ({{ }}) and fs-if
// -----------------------------------------------------------------------------
const templateBindings = [];
const ifBindings = [];

function createBinding(targetNode, kind, attrName, templateString) {
  const parts = [];
  const regex = /{{\s*([^}]+?)\s*}}/g;
  let lastIndex = 0;
  let match;

  while ((match = regex.exec(templateString)) !== null) {
    if (match.index > lastIndex) {
      parts.push({
        type: "literal",
        value: templateString.slice(lastIndex, match.index),
      });
    }
    parts.push({
      type: "expr",
      value: match[1].trim(),
    });
    lastIndex = regex.lastIndex;
  }

  if (!parts.length) return null;

  if (lastIndex < templateString.length) {
    parts.push({
      type: "literal",
      value: templateString.slice(lastIndex),
    });
  }

  return { targetNode, kind, attrName, parts };
}

function collectTemplateBindings() {
  // Text nodes
  const walker = document.createTreeWalker(
    document.body,
    NodeFilter.SHOW_TEXT,
    null
  );

  let node;
  while ((node = walker.nextNode())) {
    const text = node.nodeValue;
    if (text && text.includes("{{")) {
      const binding = createBinding(node, "text", null, text);
      if (binding) templateBindings.push(binding);
    }
  }

  // Attributes (including fs-if)
  const all = document.querySelectorAll("*");
  all.forEach((el) => {
    for (const attr of el.attributes) {
      if (attr.name === "fs-if") {
        ifBindings.push({ el, expr: attr.value.trim() });
        continue;
      }

      const val = attr.value;
      if (val && val.includes("{{")) {
        const binding = createBinding(el, "attr", attr.name, val);
        if (binding) templateBindings.push(binding);
      }
    }
  });
}

function applyTemplateBindings(data) {
  if (!data) return;

  for (const b of templateBindings) {
    let str = "";

    for (const part of b.parts) {
      if (part.type === "literal") {
        str += part.value;
      } else {
        const v = evaluateExpression(part.value, data);
        str += v == null ? "" : String(v);
      }
    }

    if (b.kind === "text") {
      b.targetNode.nodeValue = str;
    } else if (b.kind === "attr") {
      b.targetNode.setAttribute(b.attrName, str);
    }
  }
}

function applyIfBindings(data) {
  if (!data) return;

  for (const b of ifBindings) {
    const ok = evaluateCondition(b.expr, data);

    if (!ok) {
      // Hide and mark aria-hidden, but keep in DOM so we can re-show later
      b.el.style.display = "none";
      b.el.setAttribute("aria-hidden", "true");
    } else {
      b.el.style.display = "";
      b.el.removeAttribute("aria-hidden");
    }
  }
}

// -----------------------------------------------------------------------------
// Rendering
// -----------------------------------------------------------------------------
let currentJsonState = null;
let socket = null;
let socketRetryTimer = null;
let socketHasState = false;
let socketStateVersion = 0;

function renderFrame() {
  if (!currentJsonState) return;

  // Deep clone to avoid mutating original
  const st = JSON.parse(JSON.stringify(currentJsonState));

  // Ensure timers.main array
  if (!Array.isArray(st.timers.main)) {
    st.timers.main = [];
  }

  // Compute live timer values (mm:ss)
  for (let i = 0; i < st.timers.main.length; i++) {
    const t = st.timers.main[i] || {};
    const ms = liveTimerMs(t, t);
    t.live_ms = ms;
    t.mmss = mmss(ms);
    t.hhmmss = hhmmss(ms);
    t.total_seconds = Math.floor(ms / 1000);
    t.hours = Math.floor(ms / 3600000);
    t.minutes = Math.floor((ms % 3600000) / 60000);
    t.seconds = Math.floor((ms % 60000) / 1000);
    st.timers.main[i] = t;
  }
  const activeTimer = st.timers.main[0] || {};

  // ---------------------------------------------------------------------------
  // Build derived view model:
  //   - team_x / team_y: left/right teams based on swap_sides
  //   - fields_xy: each custom field mapped to x/y based on swap_sides
  // ---------------------------------------------------------------------------
  const baseHome = st.home || {};
  const baseGuest = st.guest || {};
  const swap = !!st.swap_sides;

  const team_x = swap ? baseGuest : baseHome; // left side
  const team_y = swap ? baseHome : baseGuest; // right side

  for (let tmrs of ["home_2minSusp", "guest_2minSusp"]) {
    st.timers[tmrs] = [];
    let timerID = 0;
    let team = st[tmrs.split("_")[0]];
    let goals = {};
    // const elements = document.querySelectorAll('.goal-item');
    // elements.forEach(element => element.remove());

    for (let pl of team.players.concat(team.officials)) {
      if (!!pl["goals"]) {
        for (let g of pl["goals"]) {
          let id = (g.moment_ms - (g.moment_ms % 1000)) / 1000;
          id = (id - (id % 60)) / 60;
          goals[id] = !goals[id] ? [] : goals[id];
          goals[id].push(g.goalType);
        }

        for (let i in goals) {
          for (let j in goals[i]) {

            let r = (tmrs.at(0) === 'h') ? 3 - parseInt(j) : 5 + parseInt(j);
            let c = parseInt(i) + 3;
            const element = document.querySelector(".goal-item-" + r + "-" + c);
            if (!element) {
              console.log("i = " + i + " j = " + j + " content = " + goals[i][j]);
              const newLi = document.createElement('li');
              newLi.textContent = goals[i][j].at(0);
              newLi.className = "goal-item-" + r + "-" + c; // Опционален клас за стил
              newLi.style.gridRow = r;
              newLi.style.gridColumn = c;
              newLi.style.background = intToHex(team.color);

              const container = document.querySelector('ul.goals-by-time');
              if (!!container) {
                container.appendChild(newLi);
              }
            } else {
              console.log("R = " + r + " C = " + c + " element = " + element.textContent);
            }
          }
        }
      }

      for (let pl2minSusp of ["fst2minSusp_ms", "snd2minSusp_ms", "trd2minSusp_ms", "disqualification_ms", "blueCard_ms"]) {
        if (!!pl[pl2minSusp] && ((pl[pl2minSusp] + 120 * 1000) >= activeTimer.current_moment_ms)) {
          const t = {};
          t.current_moment_ms = 120000 - (activeTimer.current_moment_ms - pl[pl2minSusp]);
          t.mode = "countdown";
          t.running = !!activeTimer.running;
          const ms = liveTimerMs(t, activeTimer);
          t.player = (!!pl.playerTshirtNumber) ? pl.playerTshirtNumber : pl.officialLetter;
          t.live_ms = ms;
          t.mss = mss(ms);
          t.mmss = mmss(ms);
          t.hhmmss = hhmmss(ms);
          t.total_seconds = Math.floor(ms / 1000);
          t.hours = Math.floor(ms / 3600000);
          t.minutes = Math.floor((ms % 3600000) / 60000);
          t.seconds = Math.floor((ms % 60000) / 1000);
          t.visible = true;
          st.timers[tmrs][timerID++] = t;
        }
      }
    }

    for (let to_ms of ["fstTimeout_ms", "sndTimeout_ms", "trdTimeout_ms"]) {
      if (!!team[to_ms] && (team[to_ms] == activeTimer.current_moment_ms)) {
        const t = !!st.timers.timeout ? st.timers.timeout : { current_moment_ms: 60000 };
        t.mode = "countdown";
        t.running = !activeTimer.running;
        const ms = liveTimerMs(t, t);
        t.live_ms = ms;
        t.ss = ss(ms);
        t.mmss = mmss(ms);
        t.hhmmss = hhmmss(ms);
        t.total_seconds = Math.floor(ms / 1000);
        t.hours = Math.floor(ms / 3600000);
        t.minutes = Math.floor((ms % 3600000) / 60000);
        t.seconds = Math.floor((ms % 60000) / 1000);
        t.visible = true;
        st.timers.timeout = t;
      }
    }
  }

  if (team_x.color) team_x.color = intToHex(team_x.color);
  if (team_y.color) team_y.color = intToHex(team_y.color);

  const fields_xy = Array.isArray(st.custom_fields)
    ? st.custom_fields.map((cf, index) => {
      const leftVal = swap ? cf.guest : cf.home;
      const rightVal = swap ? cf.home : cf.guest;
      return {
        ...cf,
        key: fieldKey(cf.label, index),
        x: leftVal,
        y: rightVal,
      };
    })
    : [];

  const fields_by_key = {};
  fields_xy.forEach((field) => {
    fields_by_key[field.key] = field;
  });

  const view = {
    ...st,
    team_x,
    team_y,
    fields_xy,
    fields_by_key,
  };

  // First apply fs-if (visibility), then template bindings (text/attrs)
  applyIfBindings(view);
  applyTemplateBindings(view);
}

// -----------------------------------------------------------------------------
// Poll loop
// -----------------------------------------------------------------------------
async function pollLoop() {
  try {
    // State is broadcast on changes, so a running timer can have a quiet socket
    // indefinitely. Only use the file until this connection supplies live state.
    if (!socket || socket.readyState !== WebSocket.OPEN || !socketHasState) {
      const version = socketStateVersion;
      const st = await fetchState();
      // A live update may arrive while the file read is in flight.
      if (version === socketStateVersion) {
        currentJsonState = st;
      }
    }
  } catch (e) {
    // ignore; keep last state
  } finally {
    setTimeout(pollLoop, 1000);
  }
}

function connectSocket() {
  if (socket && (socket.readyState === WebSocket.OPEN || socket.readyState === WebSocket.CONNECTING)) {
    return;
  }

  const params = new URLSearchParams(window.location.search);
  const wsUrl = params.get("ws") || "ws://127.0.0.1:4457";

  socketHasState = false;
  try {
    socket = new WebSocket(wsUrl);
  } catch (e) {
    scheduleSocketReconnect();
    return;
  }

  socket.addEventListener("open", () => {
    socket.send(JSON.stringify({ type: "get_state" }));
  });

  socket.addEventListener("message", (event) => {
    try {
      const payload = JSON.parse(event.data);
      const st = normalizeIncomingState(payload);
      if (st) {
        currentJsonState = st;
        socketHasState = true;
        socketStateVersion++;
      }
    } catch (e) {
      // ignore malformed remote messages
    }
  });

  socket.addEventListener("close", scheduleSocketReconnect);
  socket.addEventListener("error", () => {
    try {
      socket.close();
    } catch (e) {
      scheduleSocketReconnect();
    }
  });
}

function scheduleSocketReconnect() {
  if (socketRetryTimer) return;
  socketRetryTimer = setTimeout(() => {
    socketRetryTimer = null;
    connectSocket();
  }, 1000);
}

function animationLoop() {
  renderFrame();
  requestAnimationFrame(animationLoop);
}

// -----------------------------------------------------------------------------
// Boot
// -----------------------------------------------------------------------------
collectTemplateBindings();
connectSocket();
pollLoop();
animationLoop();

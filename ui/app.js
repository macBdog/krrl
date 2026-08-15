(() => {
  const $ = (s, r = document) => r.querySelector(s);
  const $$ = (s, r = document) => [...r.querySelectorAll(s)];

  const state = {
    tel: {},
    cut: {},
    dry: false,
    exp: null,
    machine: null,
    ws: null,
  };

  function show(hash) {
    const id = (hash || location.hash || "#live").slice(1);
    $$(".page").forEach((p) => p.classList.toggle("hidden", p.id !== id));
    $$("nav a").forEach((a) => a.classList.toggle("on", a.getAttribute("href") === "#" + id));
  }

  async function api(path, opts) {
    const r = await fetch(path, Object.assign({ headers: { "Content-Type": "application/json" } }, opts));
    if (r.status === 204) return null;
    const t = await r.text();
    return t ? JSON.parse(t) : null;
  }

  function cmd(line) {
    return api("/api/cmd", { method: "POST", body: JSON.stringify({ line }) });
  }

  function fmt(n, d) {
    if (n === undefined || n === null || Number.isNaN(n)) return "—";
    return Number(n).toFixed(d);
  }

  function paintTel(t) {
    state.tel = t;
    $("#r-rpm").textContent = fmt(t.rpm, 3);
    $("#r-x").textContent = fmt(t.x, 2);
    $("#r-z").textContent = fmt(t.z, 3);
    $("#r-t").textContent = fmt(t.t, 1);
    $("#r-state").textContent = t.state || "—";
    $("#r-vac").textContent = t.vac ? "ON" : "off";
    $("#estop").classList.toggle("hidden", !t.estop);
    paintLocks();
  }

  function paintLocks() {
    const t = state.tel || {};
    const e = state.exp;
    const heat = e ? Number(e.cutter.stylus_c) : 0;
    const needVac = e ? !!e.aux.vacuum : true;
    const items = [
      ["E-stop clear", !t.estop],
      ["X homed", !!t.xhomed],
      ["Z homed", !!t.zhomed],
      ["Platter", !e || Math.abs((t.rpm || 0) - e.platter_rpm) < 0.4 || (state.cut && state.cut.running)],
      ["Heat band", heat < 1 || Math.abs((t.t || 0) - heat) <= 5],
      ["Vacuum", !needVac || !!t.vac],
    ];
    $("#locks").innerHTML = items.map(([n, ok]) =>
      `<li class="${ok ? "ok" : "bad"}">${n}</li>`
    ).join("");
    const ready = items.every((x) => x[1]);
    $("#btn-cut").disabled = !e || !ready || !!(state.cut && state.cut.running);
  }

  function fillForm(e) {
    const f = $("#exp-form");
    f.name.value = e.name || "";
    f.diameter_in.value = String(e.blank.diameter_in);
    f.material.value = e.blank.material;
    f.platter_rpm.value = String(e.platter_rpm);
    f.lpi.value = e.groove.lpi;
    f.start_radius_mm.value = e.groove.start_radius_mm;
    f.end_radius_mm.value = e.groove.end_radius_mm;
    f.lead_in_s.value = e.groove.lead_in_s;
    f.lead_out_s.value = e.groove.lead_out_s;
    f.lock_groove.checked = !!e.groove.lock_groove;
    f.depth_um.value = e.cutter.depth_um;
    f.stylus_c.value = e.cutter.stylus_c;
    f.audio_path.value = e.audio.path || "";
    f.gain_db.value = e.audio.gain_db;
    f.vacuum.checked = !!e.aux.vacuum;
    f.notes.value = e.notes || "";
    $("#cut-name").textContent = e.name;
    cap();
  }

  function readForm() {
    const f = $("#exp-form");
    const e = state.exp || {};
    return {
      id: e.id,
      name: f.name.value,
      blank: { diameter_in: Number(f.diameter_in.value), material: f.material.value },
      platter_rpm: Number(f.platter_rpm.value),
      groove: {
        lpi: Number(f.lpi.value),
        start_radius_mm: Number(f.start_radius_mm.value),
        end_radius_mm: Number(f.end_radius_mm.value),
        lead_in_s: Number(f.lead_in_s.value),
        lead_out_s: Number(f.lead_out_s.value),
        lock_groove: f.lock_groove.checked,
      },
      cutter: { depth_um: Number(f.depth_um.value), stylus_c: Number(f.stylus_c.value) },
      audio: { path: f.audio_path.value, gain_db: Number(f.gain_db.value) },
      aux: { vacuum: f.vacuum.checked },
      notes: f.notes.value,
      result: e.result || { status: "planned", started_at: null, log: [] },
    };
  }

  function cap() {
    const e = readForm();
    const v = (e.platter_rpm / 60) * (25.4 / e.groove.lpi);
    const audio = Math.abs(e.groove.end_radius_mm - e.groove.start_radius_mm) / (v || 1e-9);
    const lock = e.groove.lock_groove && e.platter_rpm ? 3 * 60 / e.platter_rpm : 0;
    const cut = e.groove.lead_in_s + audio + e.groove.lead_out_s + lock;
    const mmss = (s) => {
      const m = Math.floor(s / 60);
      const sec = Math.round(s - m * 60);
      return m + ":" + String(sec).padStart(2, "0");
    };
    $("#capacity").textContent = "Audio " + mmss(audio) + "  ·  wall " + mmss(cut) + "  ·  feed " + v.toFixed(3) + " mm/s";
  }

  async function loadList(selectId) {
    const rows = await api("/api/experiments");
    const ul = $("#exp-list");
    ul.innerHTML = rows.map((e) =>
      `<li data-id="${e.id}" class="${state.exp && state.exp.id === e.id ? "on" : ""}">${e.name} <span style="color:#8a8474">${e.result.status}</span></li>`
    ).join("") || "<li>empty</li>";
    if (!state.exp && rows[0]) {
      state.exp = rows[0];
      fillForm(state.exp);
    }
    if (selectId) {
      const e = rows.find((x) => x.id === selectId);
      if (e) { state.exp = e; fillForm(e); }
    }
    $$("#exp-list li[data-id]").forEach((li) => {
      li.onclick = async () => {
        state.exp = await api("/api/experiments/" + li.dataset.id);
        fillForm(state.exp);
        loadList();
      };
    });
  }

  function fillSetup(m) {
    const f = $("#setup-form");
    f.serial_port.value = m.serial.port;
    f.serial_baud.value = m.serial.baud;
    f.audio_device.value = m.audio.device;
    f.camera_device.value = m.camera.device;
    f.x_steps_per_mm.value = m.x.steps_per_mm;
    f.z_steps_per_mm.value = m.z.steps_per_mm;
    f.platter_steps_per_rev.value = m.platter.steps_per_rev;
    f.tach_ppr.value = m.platter.tach_ppr;
    f.heater_kp.value = m.heater.kp;
    f.heater_ki.value = m.heater.ki;
    f.heater_kd.value = m.heater.kd;
  }

  function readSetup() {
    const f = $("#setup-form");
    const m = JSON.parse(JSON.stringify(state.machine));
    m.serial.port = f.serial_port.value;
    m.serial.baud = Number(f.serial_baud.value);
    m.audio.device = f.audio_device.value;
    m.camera.device = f.camera_device.value;
    m.x.steps_per_mm = Number(f.x_steps_per_mm.value);
    m.z.steps_per_mm = Number(f.z_steps_per_mm.value);
    m.platter.steps_per_rev = Number(f.platter_steps_per_rev.value);
    m.platter.tach_ppr = Number(f.tach_ppr.value);
    m.heater.kp = Number(f.heater_kp.value);
    m.heater.ki = Number(f.heater_ki.value);
    m.heater.kd = Number(f.heater_kd.value);
    return m;
  }

  function connectWs() {
    const proto = location.protocol === "https:" ? "wss" : "ws";
    const ws = new WebSocket(proto + "://" + location.host + "/ws");
    state.ws = ws;
    ws.onopen = () => { $("#conn").textContent = state.dry ? "dry-run" : "online"; $("#conn").classList.add("ok"); };
    ws.onclose = () => { $("#conn").textContent = "offline"; $("#conn").classList.remove("ok"); setTimeout(connectWs, 1500); };
    ws.onmessage = (ev) => {
      const msg = JSON.parse(ev.data);
      if (msg.t === "tel") paintTel(msg.d);
      if (msg.t === "cut") {
        state.cut = msg.d;
        $("#cut-phase").textContent = msg.d.phase || "idle";
        paintLocks();
      }
    };
  }

  async function poll() {
    try {
      const s = await api("/api/status");
      state.dry = s.dry_run;
      state.cut = s.cut;
      paintTel(s.tel);
      $("#cut-phase").textContent = s.cut.phase || "idle";
      $("#setup-mode").textContent = s.dry_run ? "Dry-run simulator (no Mega)." : "Live serial to Mega.";
      $("#conn").textContent = s.dry_run ? "dry-run" : (s.connected ? "online" : "offline");
      $("#conn").classList.toggle("ok", s.dry_run || s.connected);
      if (state.exp && state.exp.id) {
        const e = await api("/api/experiments/" + state.exp.id);
        if (e && e.result && e.result.log) {
          $("#cut-log").textContent = (e.result.log || []).slice(-24).join("\n");
        }
      }
    } catch (err) { /* keep last paint */ }
  }

  $("#btn-abort").onclick = () => { cmd("ABORT"); api("/api/cut/abort", { method: "POST" }); };
  $("#btn-cut-abort").onclick = $("#btn-abort").onclick;
  $("#btn-home").onclick = () => cmd("HOME ALL");
  $("#btn-heat").onclick = () => cmd("HEAT " + $("#heat").value);
  $("#btn-rpm").onclick = () => cmd("SET RPM " + $("#rpm").value);
  $("#btn-vac").onclick = () => cmd("VAC " + (state.tel.vac ? "0" : "1"));
  $$("[data-jog]").forEach((b) => {
    b.onclick = () => {
      const [ax, d] = b.dataset.jog.split(",");
      cmd("JOG " + ax + " " + d);
    };
  });
  $("#exp-form").oninput = cap;
  $("#btn-save").onclick = async (ev) => {
    ev.preventDefault();
    const e = readForm();
    state.exp = await api(e.id ? "/api/experiments/" + e.id : "/api/experiments", {
      method: e.id ? "PUT" : "POST",
      body: JSON.stringify(e),
    });
    fillForm(state.exp);
    loadList(state.exp.id);
  };
  $("#btn-new").onclick = async () => {
    state.exp = await api("/api/experiments", { method: "POST", body: "{}" });
    fillForm(state.exp);
    loadList(state.exp.id);
  };
  $("#btn-dup").onclick = async () => {
    if (!state.exp) return;
    state.exp = await api("/api/experiments/" + state.exp.id + "/duplicate", { method: "POST" });
    fillForm(state.exp);
    loadList(state.exp.id);
  };
  $("#btn-del").onclick = async () => {
    if (!state.exp) return;
    await api("/api/experiments/" + state.exp.id, { method: "DELETE" });
    state.exp = null;
    await loadList();
  };
  $("#btn-cut").onclick = async () => {
    if (!state.exp) return;
    await api("/api/experiments/" + state.exp.id, { method: "PUT", body: JSON.stringify(readForm()) });
    await api("/api/cut/start", { method: "POST", body: JSON.stringify({ id: state.exp.id }) });
  };
  $("#btn-setup-save").onclick = async () => {
    state.machine = await api("/api/machine", { method: "PUT", body: JSON.stringify(readSetup()) });
  };

  window.addEventListener("hashchange", () => show());
  show();
  (async () => {
    const s = await api("/api/status");
    state.dry = s.dry_run;
    paintTel(s.tel);
    state.machine = await api("/api/machine");
    fillSetup(state.machine);
    await loadList();
    connectWs();
    setInterval(poll, 400);
  })();
})();

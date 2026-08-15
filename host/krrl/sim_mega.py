"""In-process Mega twin. Same line grammar as firmware/docs/PROTOCOL.md."""

from __future__ import annotations

import asyncio
import time


class SimMega:
    def __init__(self, machine: dict):
        self.m = machine
        self.rpm = 0.0
        self.target_rpm = 0.0
        self.x = machine["x"]["max_mm"]
        self.z = 0.0
        self.x_tgt = self.x
        self.z_tgt = 0.0
        self.xvel = 0.0
        self.t = 25.0
        self.heat_tgt = 0.0
        self.vac = 0
        self.estop = 0
        self.state = "IDLE"
        self.xhomed = 0
        self.zhomed = 0
        self._abort = False
        self._out: asyncio.Queue[str] = asyncio.Queue()
        self._task = None

    def start(self):
        self._task = asyncio.create_task(self._run())
        self._emit("HELLO MEGA KRRL/1")

    async def close(self):
        if self._task:
            self._task.cancel()
            try:
                await self._task
            except asyncio.CancelledError:
                pass

    def _emit(self, line: str):
        self._out.put_nowait(line)

    async def readline(self) -> str:
        return await self._out.get()

    def write(self, line: str):
        self._handle(line.strip())

    def _ok(self):
        self._emit("OK")

    def _err(self, m: str):
        self._emit("ERR " + m)

    def _handle(self, s: str):
        if not s:
            return
        if s == "HELLO KRRL/1":
            self._emit("HELLO MEGA KRRL/1")
            return
        if s == "PING":
            self._emit("PONG")
            return
        if s == "ABORT":
            self._abort = True
            self.state = "ABORT"
            self._ok()
            self._emit("EVT ABORTED")
            return
        if self.estop:
            self._err("ESTOP")
            return
        if s.startswith("SET RPM "):
            self.target_rpm = float(s[8:])
            self.state = "SPINUP" if self.target_rpm > 0 else "READY"
            self._ok()
            return
        if s.startswith("SET XVEL "):
            self.xvel = float(s[9:])
            self._ok()
            return
        if s.startswith("SET X "):
            self.x_tgt = float(s[6:])
            self.xvel = 0.0
            self._ok()
            return
        if s.startswith("SET Z "):
            self.z_tgt = max(0.0, float(s[6:]))
            self._ok()
            return
        if s.startswith("HEAT "):
            self.heat_tgt = float(s[5:])
            self._ok()
            return
        if s.startswith("VAC "):
            self.vac = 1 if int(float(s[4:])) else 0
            self._ok()
            return
        if s.startswith("JOG X "):
            self.x_tgt = self.x + float(s[6:])
            self._ok()
            return
        if s.startswith("JOG Z "):
            self.z_tgt = max(0.0, self.z + float(s[6:]))
            self._ok()
            return
        if s == "HOME X":
            self.x = self.m["x"]["max_mm"]
            self.x_tgt = self.x
            self.xhomed = 1
            self.state = "READY"
            self._ok()
            self._emit("EVT HOMED X")
            return
        if s == "HOME Z":
            self.z = 0.0
            self.z_tgt = 0.0
            self.zhomed = 1
            self.state = "READY"
            self._ok()
            self._emit("EVT HOMED Z")
            return
        if s == "HOME ALL":
            self._handle("HOME X")
            self._handle("HOME Z")
            return
        if s == "ZERO X":
            self.x = self.m["x"]["max_mm"]
            self.xhomed = 1
            self._ok()
            return
        if s == "START":
            if self.estop:
                self._err("ESTOP")
                return
            if not (self.xhomed and self.zhomed):
                self._err("NOT_HOMED")
                return
            if abs(self.rpm - self.target_rpm) > self.m["platter"]["rpm_band"] and self.target_rpm > 0:
                self._err("NOT_AT_SPEED")
                return
            if self.heat_tgt > 0.5 and abs(self.t - self.heat_tgt) > self.m["heater"]["band_c"]:
                self._err("HEAT_BAND")
                return
            if self.heat_tgt > 0.5 and not self.vac:
                self._err("VAC_REQUIRED")
                return
            self.state = "CUT"
            self._ok()
            return
        self._err("UNKNOWN")

    async def _run(self):
        xmin, xmax = self.m["x"]["min_mm"], self.m["x"]["max_mm"]
        zmax = self.m["z"]["max_mm"]
        last = time.monotonic()
        tel_dt = 1.0 / max(5, int(self.m.get("host", {}).get("tel_hz", 15)))
        acc = 0.0
        try:
            while True:
                await asyncio.sleep(0.01)
                now = time.monotonic()
                dt = now - last
                last = now
                if self._abort:
                    self.xvel = 0.0
                    self.target_rpm = 0.0
                    self.z_tgt = 0.0
                    self.heat_tgt = 0.0
                    if abs(self.z) < 0.01 and self.rpm < 0.2:
                        self._abort = False
                        self.state = "READY" if self.xhomed else "IDLE"
                self.rpm += (self.target_rpm - self.rpm) * min(1.0, dt * 4)
                if self.heat_tgt > 0:
                    self.t += (self.heat_tgt - self.t) * min(1.0, dt * 1.2)
                else:
                    self.t += (22.0 - self.t) * min(1.0, dt * 0.4)
                if abs(self.xvel) > 1e-6:
                    self.x += self.xvel * dt
                    self.x_tgt = self.x
                else:
                    dx = self.x_tgt - self.x
                    step = max(-8 * dt, min(8 * dt, dx))
                    self.x += step
                dz = self.z_tgt - self.z
                self.z += max(-4 * dt, min(4 * dt, dz))
                self.x = min(xmax, max(xmin, self.x))
                self.z = min(zmax, max(0.0, self.z))
                if self.state == "SPINUP" and abs(self.rpm - self.target_rpm) <= self.m["platter"]["rpm_band"]:
                    self.state = "READY"
                    self._emit("EVT AT_SPEED")
                if self.estop:
                    self.state = "FAULT"
                acc += dt
                if acc >= tel_dt:
                    acc = 0.0
                    self._emit(
                        "TEL rpm=%.3f x=%.2f z=%.3f t=%.1f vac=%d estop=%d state=%s xhomed=%d zhomed=%d"
                        % (self.rpm, self.x, self.z, self.t, self.vac, self.estop,
                           self.state, self.xhomed, self.zhomed)
                    )
        except asyncio.CancelledError:
            return

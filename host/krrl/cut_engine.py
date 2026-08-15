"""Constant-pitch cut sequencer."""

from __future__ import annotations

import asyncio
from datetime import datetime, timezone

from .protocol import x_vel_mm_s
from .serial_link import Link
from .audio import Audio
from .experiments import Store


class CutEngine:
    def __init__(self, link: Link, audio: Audio, store: Store, machine: dict):
        self.link = link
        self.audio = audio
        self.store = store
        self.machine = machine
        self.running = False
        self.phase = "idle"
        self.exp_id = None
        self._abort = asyncio.Event()
        self._task = None

    def status(self) -> dict:
        return {
            "running": self.running,
            "phase": self.phase,
            "exp_id": self.exp_id,
        }

    async def abort(self):
        self._abort.set()
        try:
            await self.link.cmd("ABORT", wait="OK", timeout=3)
        except Exception:
            await self.link.send("ABORT")
        await self.audio.stop()
        self.phase = "aborted"
        self.running = False

    async def start(self, exp: dict):
        if self.running:
            raise RuntimeError("cut already running")
        self._abort = asyncio.Event()
        self.exp_id = exp["id"]
        self.running = True
        self._task = asyncio.create_task(self._run(exp))

    async def _log(self, exp, msg: str):
        ts = datetime.now(timezone.utc).isoformat(timespec="seconds")
        line = "%s %s" % (ts, msg)
        self.phase = msg
        try:
            self.store.append_log(exp["id"], line)
        except KeyError:
            pass

    async def _sleep(self, s: float):
        try:
            await asyncio.wait_for(self._abort.wait(), timeout=s)
            raise asyncio.CancelledError()
        except asyncio.TimeoutError:
            return

    async def _wait_x(self, tgt: float, tol=0.4):
        for _ in range(400):
            if self._abort.is_set():
                raise asyncio.CancelledError()
            if abs(self.link.tel.x - tgt) <= tol:
                return
            await asyncio.sleep(0.05)
        raise TimeoutError("X did not reach %.2f mm" % tgt)

    async def _wait_z(self, tgt: float, tol=0.02):
        for _ in range(400):
            if self._abort.is_set():
                raise asyncio.CancelledError()
            if abs(self.link.tel.z - tgt) <= tol:
                return
            await asyncio.sleep(0.05)
        raise TimeoutError("Z did not reach %.3f mm" % tgt)

    async def _run(self, exp: dict):
        g = exp["groove"]
        rpm = float(exp["platter_rpm"])
        lpi = float(g["lpi"])
        start = float(g["start_radius_mm"])
        end = float(g["end_radius_mm"])
        depth_mm = float(exp["cutter"]["depth_um"]) / 1000.0
        heat = float(exp["cutter"].get("stylus_c") or 0)
        vac = bool(exp.get("aux", {}).get("vacuum", True))
        sign = -1.0 if end < start else 1.0
        vel = sign * abs(x_vel_mm_s(rpm, lpi))
        try:
            await self._log(exp, "heat")
            await self.link.cmd("HEAT %.1f" % heat)
            if heat > 0.5:
                for _ in range(800):
                    if abs(self.link.tel.t - heat) <= self.machine["heater"]["band_c"]:
                        break
                    await self._sleep(0.05)
            if vac:
                await self._log(exp, "vacuum")
                await self.link.cmd("VAC 1")
            await self._log(exp, "spinup")
            await self.link.cmd("SET RPM %.3f" % rpm)
            for _ in range(800):
                if abs(self.link.tel.rpm - rpm) <= self.machine["platter"]["rpm_band"]:
                    break
                await self._sleep(0.05)
            await self._log(exp, "goto-start")
            await self.link.cmd("SET X %.2f" % start)
            await self._wait_x(start)
            await self._log(exp, "lead-in")
            await self.link.cmd("SET XVEL %.4f" % vel)
            await self._sleep(float(g.get("lead_in_s") or 0))
            await self._log(exp, "drop")
            await self.link.cmd("SET Z %.4f" % depth_mm)
            await self._wait_z(depth_mm)
            await self.link.cmd("START")
            await self._log(exp, "audio")
            try:
                await self.audio.start(exp.get("audio", {}).get("path") or "", exp.get("audio", {}).get("gain_db") or 0)
            except FileNotFoundError:
                await self._log(exp, "audio-missing")
            await self._log(exp, "feed")
            await self._wait_x(end, tol=0.5)
            await self.link.cmd("SET XVEL 0")
            await self._log(exp, "lead-out")
            await self._sleep(float(g.get("lead_out_s") or 0))
            if g.get("lock_groove") and rpm:
                await self._log(exp, "lock-groove")
                n = float(self.machine.get("cut", {}).get("lock_groove_revs") or 3)
                await self._sleep(n * 60.0 / rpm)
            await self._log(exp, "retract")
            await self.link.cmd("SET Z 0")
            await self._wait_z(0.0)
            await self.audio.stop()
            await self.link.cmd("SET RPM 0")
            await self._sleep(1.0)
            await self.link.cmd("VAC 0")
            rec = self.store.get(exp["id"])
            rec["result"]["status"] = "cut"
            rec["result"]["started_at"] = rec["result"].get("started_at")
            self.store.put(rec)
            await self._log(exp, "done")
            self.phase = "done"
        except asyncio.CancelledError:
            await self.audio.stop()
            rec = self.store.get(exp["id"])
            rec["result"]["status"] = "aborted"
            self.store.put(rec)
            await self._log(exp, "aborted")
        except Exception as e:
            await self.audio.stop()
            try:
                await self.link.send("ABORT")
            except Exception:
                pass
            rec = self.store.get(exp["id"])
            rec["result"]["status"] = "fault"
            self.store.put(rec)
            await self._log(exp, "fault %s" % e)
            self.phase = "fault"
        finally:
            self.running = False
            await self.audio.stop()

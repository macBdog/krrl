"""Line protocol helpers shared by host, simulator, and tests."""

from __future__ import annotations

import re
from dataclasses import dataclass


TEL_RE = re.compile(r"([a-z]+)=(\S+)")


@dataclass
class Tel:
    rpm: float = 0.0
    x: float = 160.0
    z: float = 0.0
    t: float = 25.0
    vac: int = 0
    estop: int = 0
    state: str = "IDLE"
    xhomed: int = 0
    zhomed: int = 0
    raw: str = ""

    def as_dict(self) -> dict:
        return {
            "rpm": self.rpm,
            "x": self.x,
            "z": self.z,
            "t": self.t,
            "vac": self.vac,
            "estop": self.estop,
            "state": self.state,
            "xhomed": self.xhomed,
            "zhomed": self.zhomed,
        }


def parse_tel(line: str) -> Tel | None:
    if not line.startswith("TEL "):
        return None
    kw = {m.group(1): m.group(2) for m in TEL_RE.finditer(line)}
    def f(k, d=0.0):
        try:
            return float(kw.get(k, d))
        except ValueError:
            return d
    def i(k, d=0):
        try:
            return int(float(kw.get(k, d)))
        except ValueError:
            return d
    return Tel(
        rpm=f("rpm"), x=f("x", 160), z=f("z"), t=f("t", 25),
        vac=i("vac"), estop=i("estop"),
        state=kw.get("state", "IDLE"),
        xhomed=i("xhomed"), zhomed=i("zhomed"),
        raw=line,
    )


def mm_per_rev(lpi: float) -> float:
    return 25.4 / lpi


def x_vel_mm_s(rpm: float, lpi: float) -> float:
    return (rpm / 60.0) * mm_per_rev(lpi)


def feed_time_s(start_mm: float, end_mm: float, rpm: float, lpi: float) -> float:
    v = x_vel_mm_s(rpm, lpi)
    if v <= 1e-9:
        return 0.0
    return abs(end_mm - start_mm) / v


def side_time_s(exp: dict) -> float:
    g = exp["groove"]
    rpm = float(exp["platter_rpm"])
    feed = feed_time_s(g["start_radius_mm"], g["end_radius_mm"], rpm, g["lpi"])
    lock = 0.0
    if g.get("lock_groove") and rpm:
        lock = 3.0 * 60.0 / rpm
    return g.get("lead_in_s", 0) + feed + g.get("lead_out_s", 0) + lock


def capacity_s(exp: dict) -> dict:
    """Audio seconds that fit, and total cut wall time."""
    total = side_time_s(exp)
    g = exp["groove"]
    rpm = float(exp["platter_rpm"])
    audio = feed_time_s(g["start_radius_mm"], g["end_radius_mm"], rpm, g["lpi"])
    return {"cut_s": total, "audio_s": audio}

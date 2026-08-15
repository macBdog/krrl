"""JSON experiment store."""

from __future__ import annotations

import json
from datetime import datetime, timezone
from pathlib import Path


def utc_now() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S")


def default_experiment(name="untitled") -> dict:
    return {
        "id": utc_now() + "-001",
        "name": name,
        "blank": {"diameter_in": 12, "material": "lacquer"},
        "platter_rpm": 33.333,
        "groove": {
            "lpi": 220,
            "start_radius_mm": 146,
            "end_radius_mm": 60,
            "lead_in_s": 2,
            "lead_out_s": 4,
            "lock_groove": True,
        },
        "cutter": {"depth_um": 40, "stylus_c": 180},
        "audio": {"path": "", "gain_db": -3},
        "aux": {"vacuum": True},
        "notes": "",
        "result": {"status": "planned", "started_at": None, "log": []},
    }


class Store:
    def __init__(self, root: Path):
        self.root = Path(root)
        self.root.mkdir(parents=True, exist_ok=True)

    def _path(self, eid: str) -> Path:
        safe = "".join(c if c.isalnum() or c in "-_" else "_" for c in eid)
        return self.root / (safe + ".json")

    def list(self) -> list[dict]:
        rows = []
        for p in sorted(self.root.glob("*.json")):
            try:
                rows.append(json.loads(p.read_text(encoding="utf-8")))
            except (OSError, json.JSONDecodeError):
                continue
        rows.sort(key=lambda e: e.get("id", ""), reverse=True)
        return rows

    def get(self, eid: str) -> dict:
        p = self._path(eid)
        if not p.exists():
            raise KeyError(eid)
        return json.loads(p.read_text(encoding="utf-8"))

    def put(self, exp: dict) -> dict:
        if not exp.get("id"):
            exp["id"] = utc_now() + "-001"
        p = self._path(exp["id"])
        tmp = p.with_suffix(".tmp")
        tmp.write_text(json.dumps(exp, indent=2) + "\n", encoding="utf-8")
        tmp.replace(p)
        return exp

    def delete(self, eid: str):
        p = self._path(eid)
        if p.exists():
            p.unlink()

    def duplicate(self, eid: str) -> dict:
        exp = self.get(eid)
        exp["id"] = utc_now() + "-copy"
        exp["name"] = (exp.get("name") or eid) + " copy"
        exp["result"] = {"status": "planned", "started_at": None, "log": []}
        return self.put(exp)

    def append_log(self, eid: str, line: str) -> dict:
        exp = self.get(eid)
        exp.setdefault("result", {}).setdefault("log", []).append(line)
        return self.put(exp)

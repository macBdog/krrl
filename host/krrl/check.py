"""Desktop checks: math, YAML, protocol, simulator, store. python3 -m krrl.check"""

from __future__ import annotations

import asyncio
import tempfile
from pathlib import Path

from .experiments import Store, default_experiment
from .machine import load_yaml, dump_yaml, load_machine
from .protocol import mm_per_rev, x_vel_mm_s, feed_time_s, parse_tel, side_time_s
from .sim_mega import SimMega
from .app import App
from .audio import Audio
from .camera import Camera
from .serial_link import SimLink
from .__main__ import Ctx

ROOT = Path(__file__).resolve().parents[2]


def _eq(a, b, eps=1e-6):
    if abs(a - b) > eps:
        raise AssertionError("%r != %r" % (a, b))


def test_math():
    _eq(mm_per_rev(254), 0.1)
    v = x_vel_mm_s(33.333, 220)
    _eq(v, 33.333 / 60.0 * (25.4 / 220), 1e-9)
    t = feed_time_s(146, 60, 33.333, 220)
    _eq(t, 86 / v, 1e-6)
    exp = default_experiment()
    assert side_time_s(exp) > exp["groove"]["lead_in_s"]


def test_tel():
    tel = parse_tel("TEL rpm=33.331 x=142.20 z=0.040 t=179.8 vac=1 estop=0 state=CUT xhomed=1 zhomed=1")
    assert tel.state == "CUT"
    _eq(tel.rpm, 33.331, 1e-6)
    _eq(tel.z, 0.04, 1e-6)
    assert tel.vac == 1 and tel.xhomed == 1


def test_yaml():
    m = load_machine(ROOT / "config" / "machine.yaml")
    assert m["serial"]["baud"] == 115200
    assert m["cut"]["require_vacuum"] is True
    roundtrip = load_yaml(dump_yaml(m))
    assert roundtrip["platter"]["steps_per_rev"] == m["platter"]["steps_per_rev"]


def test_store():
    with tempfile.TemporaryDirectory() as d:
        s = Store(Path(d))
        e = s.put(default_experiment("alpha"))
        assert s.get(e["id"])["name"] == "alpha"
        c = s.duplicate(e["id"])
        assert c["id"] != e["id"]
        s.delete(e["id"])
        try:
            s.get(e["id"])
            raise AssertionError("deleted")
        except KeyError:
            pass


async def test_sim():
    m = load_machine(ROOT / "config" / "machine.yaml")
    sim = SimMega(m)
    sim.start()
    hello = await asyncio.wait_for(sim.readline(), 1)
    assert "HELLO MEGA" in hello
    sim.write("START")
    found = False
    for _ in range(40):
        line = await asyncio.wait_for(sim.readline(), 1)
        if "ERR NOT_HOMED" in line:
            found = True
            break
    assert found
    sim.write("HOME ALL")
    sim.write("START")
    for _ in range(40):
        await asyncio.wait_for(sim.readline(), 1)
        if sim.state == "CUT":
            break
    assert sim.xhomed and sim.zhomed
    assert sim.state == "CUT"
    await sim.close()


async def test_http():
    m = load_machine(ROOT / "config" / "machine.yaml")
    with tempfile.TemporaryDirectory() as d:
        store = Store(Path(d))
        store.put(default_experiment("http-check"))
        link = SimLink(m)
        link.start()
        ctx = Ctx(m, ROOT / "config" / "machine.yaml", link,
                  store, Audio(dry_run=True), Camera(dry_run=True), ROOT / "ui")
        app = App(ctx)
        port = await app.server.start("127.0.0.1", 0)
        task = asyncio.create_task(app.server._srv.serve_forever())
        try:
            r, w = await asyncio.open_connection("127.0.0.1", port)
            w.write(b"GET /api/status HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n")
            await w.drain()
            data = await r.read()
            w.close()
            assert b'"dry_run": true' in data
            assert b"200" in data.split(b"\r\n", 1)[0]
        finally:
            task.cancel()
            await link.close()
            app.server._srv.close()
            await app.server._srv.wait_closed()


def main():
    test_math()
    test_tel()
    test_yaml()
    test_store()
    asyncio.run(test_sim())
    asyncio.run(test_http())
    print("krrl.check: ok")


if __name__ == "__main__":
    main()

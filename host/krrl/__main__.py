"""KRRL-01 host entry: python3 -m krrl --dry-run"""

from __future__ import annotations

import argparse
import asyncio
from pathlib import Path

from .app import App
from .audio import Audio
from .camera import Camera
from .cut_engine import CutEngine
from .experiments import Store, default_experiment
from .machine import load_machine, save_machine
from .serial_link import SerialLink, SimLink

ROOT = Path(__file__).resolve().parents[2]


class Ctx:
    def __init__(self, machine, machine_path, link, store, audio, camera, ui_dir):
        self.machine = machine
        self.machine_path = machine_path
        self.link = link
        self.store = store
        self.audio = audio
        self.camera = camera
        self.ui_dir = ui_dir
        self.cut = CutEngine(link, audio, store, machine)

    def update_machine(self, obj: dict):
        self.machine.update(obj)
        save_machine(self.machine_path, self.machine)


async def main_async(args):
    cfg_path = Path(args.config)
    machine = load_machine(cfg_path)
    ui_dir = Path(args.ui)
    data = Path(args.data)
    store = Store(data / "experiments")
    if not store.list():
        store.put(default_experiment("12in test lacquer A"))

    if args.dry_run:
        link = SimLink(machine)
        link.start()
    else:
        port = args.serial or machine["serial"]["port"]
        link = SerialLink(port, int(machine["serial"]["baud"]))
        await link.open()

    audio = Audio(machine.get("audio", {}).get("device") or "default", dry_run=args.dry_run)
    camera = Camera(machine.get("camera", {}).get("device") or "/dev/video0", dry_run=args.dry_run)
    ctx = Ctx(machine, cfg_path, link, store, audio, camera, ui_dir)
    app = App(ctx)

    loop = asyncio.get_event_loop()

    def on_line(kind, payload):
        if kind == "tel":
            loop.create_task(app.broadcast_tel(payload))
        else:
            d = payload.as_dict() if hasattr(payload, "as_dict") else payload
            loop.create_task(app.broadcast_evt(kind, d))

    link.on_line(on_line)
    bind = args.bind or machine.get("host", {}).get("bind") or "0.0.0.0"
    port = int(args.port or machine.get("host", {}).get("port") or 8080)
    print("KRRL-01 http://%s:%s  dry_run=%s" % (bind if bind != "0.0.0.0" else "127.0.0.1", port, args.dry_run))
    try:
        await app.server.serve(bind, port)
    finally:
        await link.close()
        await audio.stop()


def build_parser():
    p = argparse.ArgumentParser(prog="krrl")
    p.add_argument("--dry-run", action="store_true", help="Mega simulator, no hardware")
    p.add_argument("--config", default=str(ROOT / "config" / "machine.yaml"))
    p.add_argument("--ui", default=str(ROOT / "ui"))
    p.add_argument("--data", default=str(ROOT / "data"))
    p.add_argument("--bind", default="")
    p.add_argument("--port", type=int, default=0)
    p.add_argument("--serial", default="")
    return p


def main():
    args = build_parser().parse_args()
    asyncio.run(main_async(args))


if __name__ == "__main__":
    main()

"""HTTP routes and WebSocket fan-out."""

from __future__ import annotations

import asyncio
import json
from datetime import datetime, timezone
from pathlib import Path

from .httpd import Request, Server
from .protocol import capacity_s
from .experiments import default_experiment


MIME = {
    ".html": "text/html; charset=utf-8",
    ".js": "application/javascript; charset=utf-8",
    ".css": "text/css; charset=utf-8",
    ".svg": "image/svg+xml",
    ".json": "application/json",
    ".yaml": "text/yaml; charset=utf-8",
    ".md": "text/plain; charset=utf-8",
}


def _json(obj, status=200):
    body = json.dumps(obj).encode("utf-8")
    return status, body, "application/json; charset=utf-8", None


class App:
    def __init__(self, ctx):
        self.ctx = ctx
        self.ui = ctx.ui_dir
        self.server = Server(self.handle, self.ws)

    async def broadcast_tel(self, tel):
        await self.server.broadcast(json.dumps({"t": "tel", "d": tel.as_dict()}))

    async def broadcast_evt(self, kind, payload):
        await self.server.broadcast(json.dumps({"t": kind, "d": payload}))

    async def ws(self, client):
        await client.send_text(json.dumps({"t": "tel", "d": self.ctx.link.tel.as_dict()}))
        await client.send_text(json.dumps({"t": "cut", "d": self.ctx.cut.status()}))
        while client.alive:
            msg = await client.recv_text()
            if msg is None:
                break
            if not msg:
                continue
            try:
                obj = json.loads(msg)
            except json.JSONDecodeError:
                continue
            if obj.get("t") == "cmd" and obj.get("line"):
                try:
                    await self.ctx.link.cmd(str(obj["line"]))
                except Exception as e:
                    await client.send_text(json.dumps({"t": "err", "d": str(e)}))

    async def handle(self, req: Request, writer):
        p, m = req.path, req.method
        if p in ("/", "/index.html"):
            return self._file(self.ui / "index.html")
        if p.startswith("/ui/"):
            return self._file(self.ui / p[4:])
        rel = p.lstrip("/")
        cand = self.ui / rel
        if m == "GET" and cand.is_file() and self.ui.resolve() in cand.resolve().parents:
            return self._file(cand)
        if p == "/api/status" and m == "GET":
            return _json(self._status())
        if p == "/api/machine" and m == "GET":
            return _json(self.ctx.machine)
        if p == "/api/machine" and m == "PUT":
            self.ctx.update_machine(req.json())
            return _json(self.ctx.machine)
        if p == "/api/experiments" and m == "GET":
            rows = self.ctx.store.list()
            for e in rows:
                e["capacity"] = capacity_s(e)
            return _json(rows)
        if p == "/api/experiments" and m == "POST":
            body = req.json() or default_experiment()
            if not body.get("id"):
                body = {**default_experiment(body.get("name") or "untitled"), **body}
            return _json(self.ctx.store.put(body), 201)
        if p.startswith("/api/experiments/"):
            rest = p[len("/api/experiments/"):]
            if rest.endswith("/duplicate") and m == "POST":
                eid = rest[: -len("/duplicate")]
                return _json(self.ctx.store.duplicate(eid), 201)
            eid = rest
            if m == "GET":
                try:
                    e = self.ctx.store.get(eid)
                except KeyError:
                    return _json({"error": "missing"}, 404)
                e["capacity"] = capacity_s(e)
                return _json(e)
            if m == "PUT":
                body = req.json()
                body["id"] = eid
                return _json(self.ctx.store.put(body))
            if m == "DELETE":
                self.ctx.store.delete(eid)
                return 204, b"", "text/plain", None
        if p == "/api/cmd" and m == "POST":
            line = (req.json() or {}).get("line") or ""
            try:
                await self.ctx.link.cmd(line)
                return _json({"ok": True})
            except Exception as e:
                return _json({"error": str(e)}, 400)
        if p == "/api/cut/start" and m == "POST":
            eid = (req.json() or {}).get("id")
            try:
                exp = self.ctx.store.get(eid)
                exp["result"]["status"] = "cutting"
                exp["result"]["started_at"] = datetime.now(timezone.utc).isoformat(timespec="seconds")
                self.ctx.store.put(exp)
                await self.ctx.cut.start(exp)
                return _json(self.ctx.cut.status())
            except Exception as e:
                return _json({"error": str(e)}, 409)
        if p == "/api/cut/abort" and m == "POST":
            await self.ctx.cut.abort()
            return _json(self.ctx.cut.status())
        if p == "/api/camera" and m == "GET":
            extra = ["Content-Type: multipart/x-mixed-replace; boundary=krrl", "Cache-Control: no-store"]
            writer.write(
                ("HTTP/1.1 200 OK\r\n" + "\r\n".join(extra) + "\r\n\r\n").encode("ascii")
            )
            await writer.drain()
            try:
                await self.ctx.camera.mjpeg(self._write(writer))
            except (ConnectionError, BrokenPipeError, asyncio.CancelledError):
                pass
            return 0, b"", "", None
        if p == "/api/camera.jpg" and m == "GET":
            jpg = await self.ctx.camera.frame()
            return 200, jpg, "image/jpeg", None
        return _json({"error": "not found"}, 404)

    def _write(self, writer):
        async def w(data: bytes):
            writer.write(data)
            await writer.drain()
        return w

    def _file(self, path: Path):
        if not path.is_file():
            return _json({"error": "not found"}, 404)
        data = path.read_bytes()
        ctype = MIME.get(path.suffix, "application/octet-stream")
        return 200, data, ctype, None

    def _status(self):
        return {
            "tel": self.ctx.link.tel.as_dict(),
            "cut": self.ctx.cut.status(),
            "dry_run": self.ctx.link.dry_run,
            "connected": self.ctx.link.connected or self.ctx.link.dry_run,
            "audio": {"playing": self.ctx.audio.playing, "path": self.ctx.audio.path},
        }

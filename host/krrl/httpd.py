"""Tiny HTTP/1.1 + WebSocket server. Stdlib only."""

from __future__ import annotations

import asyncio
import base64
import hashlib
import json
from urllib.parse import unquote, urlparse

WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


class Request:
    def __init__(self, method, path, headers, body, query):
        self.method = method
        self.path = path
        self.headers = headers
        self.body = body
        self.query = query

    def json(self):
        if not self.body:
            return {}
        return json.loads(self.body.decode("utf-8"))


async def _read_req(reader) -> Request:
    line = await reader.readline()
    if not line:
        raise ConnectionError("eof")
    parts = line.decode("latin1").split()
    if len(parts) < 2:
        raise ConnectionError("bad request")
    method, raw = parts[0], parts[1]
    headers = {}
    while True:
        h = await reader.readline()
        if h in (b"\r\n", b"\n", b""):
            break
        k, _, v = h.decode("latin1").partition(":")
        headers[k.lower().strip()] = v.strip()
    n = int(headers.get("content-length") or 0)
    body = await reader.readexactly(n) if n else b""
    u = urlparse(raw)
    path = unquote(u.path)
    query = dict(p.split("=", 1) if "=" in p else (p, "") for p in u.query.split("&") if p)
    return Request(method, path, headers, body, query)


def _http(status: int, body: bytes, content_type="text/plain; charset=utf-8", extra=None):
    reason = {200: "OK", 201: "Created", 204: "No Content", 400: "Bad Request",
              404: "Not Found", 409: "Conflict", 500: "Error"}.get(status, "OK")
    hdr = [
        "HTTP/1.1 %d %s" % (status, reason),
        "Content-Type: %s" % content_type,
        "Content-Length: %d" % len(body),
        "Connection: close",
        "Cache-Control: no-store",
    ]
    if extra:
        hdr.extend(extra)
    return ("\r\n".join(hdr) + "\r\n\r\n").encode("ascii") + body


class WsClient:
    def __init__(self, reader, writer):
        self.reader = reader
        self.writer = writer
        self.alive = True

    async def send_text(self, s: str):
        data = s.encode("utf-8")
        n = len(data)
        hdr = bytearray([0x81])
        if n < 126:
            hdr.append(n)
        elif n < 65536:
            hdr.append(126)
            hdr.extend(n.to_bytes(2, "big"))
        else:
            hdr.append(127)
            hdr.extend(n.to_bytes(8, "big"))
        self.writer.write(bytes(hdr) + data)
        await self.writer.drain()

    async def recv_text(self) -> str | None:
        h = await self.reader.readexactly(2)
        op = h[0] & 0x0F
        masked = h[1] & 0x80
        n = h[1] & 0x7F
        if n == 126:
            n = int.from_bytes(await self.reader.readexactly(2), "big")
        elif n == 127:
            n = int.from_bytes(await self.reader.readexactly(8), "big")
        mask = await self.reader.readexactly(4) if masked else b""
        payload = bytearray(await self.reader.readexactly(n))
        if masked:
            for i in range(n):
                payload[i] ^= mask[i % 4]
        if op == 0x8:
            self.alive = False
            return None
        if op == 0x9:
            self.writer.write(b"\x8a" + bytes([n]) + bytes(payload))
            await self.writer.drain()
            return ""
        return payload.decode("utf-8", "replace")


class Server:
    def __init__(self, handler, ws_handler=None):
        self.handler = handler
        self.ws_handler = ws_handler
        self.clients: list[WsClient] = []

    async def broadcast(self, s: str):
        dead = []
        for c in list(self.clients):
            try:
                await c.send_text(s)
            except Exception:
                dead.append(c)
        for c in dead:
            c.alive = False
            if c in self.clients:
                self.clients.remove(c)

    async def _client(self, reader, writer):
        try:
            req = await _read_req(reader)
            if req.path == "/ws" and "websocket" in req.headers.get("upgrade", "").lower():
                await self._ws(req, reader, writer)
                return
            status, body, ctype, extra = await self.handler(req, writer)
            if status == 0:
                return
            writer.write(_http(status, body, ctype, extra))
            await writer.drain()
        except (ConnectionError, asyncio.IncompleteReadError, BrokenPipeError):
            pass
        except Exception as e:
            try:
                writer.write(_http(500, str(e).encode("utf-8")))
                await writer.drain()
            except Exception:
                pass
        finally:
            try:
                writer.close()
                await writer.wait_closed()
            except Exception:
                pass

    async def _ws(self, req, reader, writer):
        key = req.headers.get("sec-websocket-key", "")
        acc = base64.b64encode(hashlib.sha1((key + WS_GUID).encode()).digest()).decode()
        writer.write(
            ("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\n"
             "Connection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n" % acc).encode("ascii")
        )
        await writer.drain()
        client = WsClient(reader, writer)
        self.clients.append(client)
        try:
            if self.ws_handler:
                await self.ws_handler(client)
            else:
                while client.alive:
                    msg = await client.recv_text()
                    if msg is None:
                        break
        finally:
            if client in self.clients:
                self.clients.remove(client)

    async def start(self, host, port):
        self._srv = await asyncio.start_server(self._client, host, port)
        return self._srv.sockets[0].getsockname()[1]

    async def serve(self, host, port):
        await self.start(host, port)
        async with self._srv:
            await self._srv.serve_forever()

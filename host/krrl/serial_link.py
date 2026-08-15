"""Serial (or dry-run) link to the Mega."""

from __future__ import annotations

import asyncio
from collections import deque
from typing import Callable

from .protocol import Tel, parse_tel
from .sim_mega import SimMega


class Link:
    def __init__(self):
        self.tel = Tel()
        self.events: deque[str] = deque(maxlen=64)
        self._waiters: list[tuple[str, asyncio.Future]] = []
        self._listeners: list[Callable[[str, object], None]] = []
        self.connected = False
        self.dry_run = False

    def on_line(self, fn: Callable[[str, object], None]):
        self._listeners.append(fn)

    def _dispatch(self, kind: str, payload):
        for fn in self._listeners:
            try:
                fn(kind, payload)
            except Exception:
                pass

    def _on_rx(self, line: str):
        line = line.strip()
        if not line:
            return
        tel = parse_tel(line)
        if tel:
            self.tel = tel
            self._dispatch("tel", tel)
            return
        if line.startswith("EVT "):
            self.events.append(line[4:])
            self._dispatch("evt", line[4:])
        elif line.startswith("ERR "):
            self._dispatch("err", line[4:])
        elif line.startswith("HELLO "):
            self.connected = True
            self._dispatch("hello", line)
        self._wake(line)

    def _wake(self, line: str):
        keep = []
        for prefix, fut in self._waiters:
            if fut.done():
                continue
            hit = line.startswith(prefix) or (prefix == "OK" and line.startswith("ERR"))
            if hit:
                fut.set_result(line)
            else:
                keep.append((prefix, fut))
        self._waiters = keep

    async def cmd(self, line: str, wait="OK", timeout=8.0) -> str:
        await self.send(line)
        if wait is None:
            return ""
        fut = asyncio.get_running_loop().create_future()
        self._waiters.append((wait, fut))
        try:
            reply = await asyncio.wait_for(fut, timeout)
        except asyncio.TimeoutError:
            raise TimeoutError("no %s for %s" % (wait, line)) from None
        if reply.startswith("ERR"):
            raise RuntimeError(reply)
        return reply

    async def send(self, line: str):
        raise NotImplementedError

    async def close(self):
        pass


class SimLink(Link):
    def __init__(self, machine: dict):
        super().__init__()
        self.dry_run = True
        self.sim = SimMega(machine)
        self._rx_task = None

    def start(self):
        self.sim.start()
        self._rx_task = asyncio.create_task(self._pump())

    async def _pump(self):
        try:
            while True:
                line = await self.sim.readline()
                self._on_rx(line)
        except asyncio.CancelledError:
            return

    async def send(self, line: str):
        self.sim.write(line)

    async def close(self):
        if self._rx_task:
            self._rx_task.cancel()
        await self.sim.close()


class SerialLink(Link):
    def __init__(self, port: str, baud: int):
        super().__init__()
        self.port = port
        self.baud = baud
        self._ser = None
        self._rx_task = None

    async def open(self):
        try:
            import serial_asyncio
        except ImportError:
            import serial
            self._ser = serial.Serial(self.port, self.baud, timeout=0.05)
            self._rx_task = asyncio.create_task(self._pump_pyserial())
            await self.send("HELLO KRRL/1")
            return
        self._reader, self._writer = await serial_asyncio.open_serial_connection(
            url=self.port, baudrate=self.baud
        )
        self._rx_task = asyncio.create_task(self._pump_async())
        await self.send("HELLO KRRL/1")

    async def _pump_async(self):
        try:
            while True:
                raw = await self._reader.readline()
                self._on_rx(raw.decode("ascii", "replace"))
        except asyncio.CancelledError:
            return

    async def _pump_pyserial(self):
        loop = asyncio.get_event_loop()
        buf = b""
        try:
            while True:
                chunk = await loop.run_in_executor(None, self._ser.read, 256)
                if not chunk:
                    await asyncio.sleep(0.01)
                    continue
                buf += chunk
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    self._on_rx(line.decode("ascii", "replace"))
        except asyncio.CancelledError:
            return

    async def send(self, line: str):
        data = (line.strip() + "\n").encode("ascii")
        if getattr(self, "_writer", None):
            self._writer.write(data)
            await self._writer.drain()
        elif self._ser:
            await asyncio.get_event_loop().run_in_executor(None, self._ser.write, data)

    async def close(self):
        if self._rx_task:
            self._rx_task.cancel()
        if getattr(self, "_writer", None):
            self._writer.close()
        if self._ser:
            self._ser.close()

"""Native playback via mpv (PipeWire/ALSA). Dry-run is a no-op clock."""

from __future__ import annotations

import asyncio
import shutil
from pathlib import Path


class Audio:
    def __init__(self, device="default", dry_run=False):
        self.device = device
        self.dry_run = dry_run
        self._proc = None
        self.playing = False
        self.path = ""

    async def start(self, path: str, gain_db: float = 0.0):
        await self.stop()
        self.path = path or ""
        if self.dry_run or not path:
            self.playing = True
            return
        if not Path(path).exists():
            raise FileNotFoundError(path)
        mpv = shutil.which("mpv")
        if not mpv:
            self.playing = True
            return
        cmd = [
            mpv, "--no-video", "--really-quiet",
            "--audio-device=%s" % self.device,
            "--volume=%s" % str(int(min(100, max(0, 100 + gain_db * 4)))),
            path,
        ]
        self._proc = await asyncio.create_subprocess_exec(
            *cmd, stdout=asyncio.subprocess.DEVNULL, stderr=asyncio.subprocess.DEVNULL
        )
        self.playing = True

    async def stop(self):
        self.playing = False
        if self._proc and self._proc.returncode is None:
            self._proc.terminate()
            try:
                await asyncio.wait_for(self._proc.wait(), 2)
            except asyncio.TimeoutError:
                self._proc.kill()
        self._proc = None

"""MJPEG camera. v4l2/ffmpeg when present; otherwise a still JPEG."""

from __future__ import annotations

import asyncio
import shutil
from pathlib import Path

STILL = bytes.fromhex(
    "ffd8ffe000104a46494600010100000100010000ffdb004300"
    "01010101010101010101010101010101010101010101010101"
    "01010101010101010101010101010101010101010101010101"
    "01010101010101ffc0000b0800010001010111ffc40014000100"
    "000000000000000000000000000000ffda00080001000100003f"
    "00fb00ffd9"
)


class Camera:
    def __init__(self, device="/dev/video0", dry_run=False):
        self.device = device
        self.dry_run = dry_run
        self._frame = STILL

    async def frame(self) -> bytes:
        if self.dry_run or not Path(self.device).exists():
            return self._frame
        ffmpeg = shutil.which("ffmpeg")
        if not ffmpeg:
            return self._frame
        proc = await asyncio.create_subprocess_exec(
            ffmpeg, "-hide_banner", "-loglevel", "error",
            "-f", "v4l2", "-i", self.device,
            "-frames:v", "1", "-f", "mjpeg", "pipe:1",
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.DEVNULL,
        )
        out, _ = await proc.communicate()
        if out:
            self._frame = out
        return self._frame

    async def mjpeg(self, write, boundary="krrl"):
        while True:
            jpg = await self.frame()
            head = (
                "--%s\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n"
                % (boundary, len(jpg))
            ).encode("ascii")
            await write(head + jpg + b"\r\n")
            await asyncio.sleep(0.12)

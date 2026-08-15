# Raspberry Pi OS for KRRL-01

**Distro: Raspberry Pi OS Lite 64-bit** (Debian Bookworm or successor Trixie).

Not Ubuntu, DietPi, or a desktop image. Official Pi 4 I2S overlays, `libcamera`, USB serial, and a slow Debian package cadence matter more here than a longer Ubuntu LTS window.

## Why Lite

Motor pulses are on the Mega. The Pi still must play a WAV for the length of a side without resampling or scheduler gaps. A compositor, Bluetooth, and Wi-Fi power save work against that.

## Install outline

1. Raspberry Pi Imager → Raspberry Pi OS Lite (64-bit) → enable SSH, set user `krrl`.
2. Boot, `sudo apt update && sudo apt install -y python3 python3-serial mpv ffmpeg v4l-utils`
3. I2S DAC HAT (HiFiBerry DAC2 HD or equivalent): enable the vendor overlay in `/boot/firmware/config.txt`. Do not use the onboard 3.5 mm jack for the cutter amp.
4. `sudo bash /opt/krrl-01/deploy/tune-pi.sh`
5. `sudo cp /opt/krrl-01/deploy/krrl.service /etc/systemd/system/`
6. `sudo systemctl enable --now krrl`

## Audio

PipeWire on current Raspberry Pi OS. Pin 48 kHz, quantum 1024 (see `deploy/pipewire-krrl.conf`). Playback is `mpv --audio-device=…` so the mixer's hot path is native.

Optional later: official `PREEMPT_RT` kernel. Not required for v1.

## Camera

USB UVC (`/dev/video0`) or CSI via `libcamera`. The host serves MJPEG at `/api/camera`.

## Serial

Mega appears as `/dev/ttyACM0`. User `krrl` must be in group `dialout`.

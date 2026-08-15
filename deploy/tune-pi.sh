#!/bin/bash
# Harden Raspberry Pi OS Lite 64-bit for KRRL-01 audio + serial.
# Idempotent. Run as root: sudo bash deploy/tune-pi.sh
set -euo pipefail

if [[ ${EUID} -ne 0 ]]; then
  echo "run as root" >&2
  exit 1
fi

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y python3 python3-serial mpv ffmpeg v4l-utils

usermod -aG dialout,video,audio "${SUDO_USER:-krrl}" 2>/dev/null || true

# USB serial and I2S; disable Bluetooth which steals UART on some images.
CFG=/boot/firmware/config.txt
if [[ ! -f $CFG ]]; then
  CFG=/boot/config.txt
fi
if [[ -f $CFG ]]; then
  grep -q '^dtparam=audio=off' "$CFG" || echo 'dtparam=audio=off' >> "$CFG"
  grep -q '^dtoverlay=disable-bt' "$CFG" || echo 'dtoverlay=disable-bt' >> "$CFG"
fi

# Wi-Fi power save off if wlan0 exists.
if command -v iw >/dev/null && ip link show wlan0 >/dev/null 2>&1; then
  iw dev wlan0 set power_save off || true
  mkdir -p /etc/NetworkManager/conf.d
  printf '[connection]\nwifi.powersave=2\n' > /etc/NetworkManager/conf.d/krrl-wifi.conf 2>/dev/null || true
fi

# PipeWire quantum. Drop-in if PipeWire is present.
if [[ -d /etc/pipewire ]]; then
  mkdir -p /etc/pipewire/pipewire.conf.d
  cp "$(dirname "$0")/pipewire-krrl.conf" /etc/pipewire/pipewire.conf.d/krrl.conf
fi

# Isolate core 3 for playback (applied after reboot).
CMDLINE=/boot/firmware/cmdline.txt
if [[ ! -f $CMDLINE ]]; then
  CMDLINE=/boot/cmdline.txt
fi
if [[ -f $CMDLINE ]] && ! grep -q isolcpus "$CMDLINE"; then
  sed -i 's/$/ isolcpus=3/' "$CMDLINE"
fi

systemctl disable --now bluetooth.service 2>/dev/null || true
systemctl disable --now hciuart.service 2>/dev/null || true

install -d -m 0755 /opt/krrl-01
echo "tune-pi: done. Reboot, then enable deploy/krrl.service"

# KRRL-01

Vinyl record lathe control software: Raspberry Pi 4 host (audio, camera, UI) plus Arduino Mega 2560 firmware (motors, heater, vacuum, E-stop).

## Stack

- **OS:** Raspberry Pi OS Lite 64-bit. Motion timing stays on the Mega, not Linux.
- **Host:** Python 3.11+ asyncio, stdlib HTTP, compact WebSocket. Optional `pyserial`.
- **Firmware:** Arduino C++ on Mega 2560. TMC2209 STEP/DIR; UART for driver setup.
- **UI:** Vanilla HTML/CSS/JS in `ui/`. No npm build.

## Layout

```
README.md
AGENTS.md
firmware/krrl_mega/
host/krrl/
ui/
config/machine.yaml
deploy/
docs/PROTOCOL.md
```

## Rules

- Compact, readable, explicit units in names (`mm`, `um`, `rpm`).
- Do not put stepper pulse timing on the Pi.
- Firmware owns abort: Z retract, X stop, platter down, heater off.
- Dry-run (`python3 -m krrl --dry-run`) must work without hardware.

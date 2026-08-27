# KRRL-01

Control software and operator UI for the KRRL-01 vinyl record lathe.

**Hardware:** Raspberry Pi 4 (brain + audio + camera) and Arduino Mega 2560 (real-time motors and IO).  
**OS:** Raspberry Pi OS Lite 64-bit (Debian). See [docs/OS.md](docs/OS.md).

The Pi never generates step pulses. The Mega owns platter, X (lead screw), Z (depth), heated stylus PID, chip vacuum, limits, and E-stop.

## Quick start (desktop, no hardware)

```bash
cd krrl-01
PYTHONPATH=host python3 -m krrl --dry-run --port 8080
```

Open http://127.0.0.1:8080 — Live, Experiment, Cut, and Setup all run against a Mega simulator.

```bash
PYTHONPATH=host python3 -m krrl.check
```

## Pi (with Mega)

1. Flash **Raspberry Pi OS Lite 64-bit**.
2. Run `sudo bash deploy/tune-pi.sh`.
3. Copy this tree to `/opt/krrl-01`.
4. `sudo cp deploy/krrl.service /etc/systemd/system && sudo systemctl enable --now krrl`
5. Connect the Mega on USB (`/dev/ttyACM0`). I2S DAC HAT for the cutter amp; not the 3.5 mm jack.

## Layout

| Path | Role |
|------|------|
| `firmware/krrl_mega/` | Mega 2560 sketch (lathe) |
| `firmware/krrl_player/` | Nano sketch (player turntable variant) |
| `host/krrl/` | Python host |
| `ui/` | Operator console |
| `config/machine.yaml` | Calibration and devices |
| `docs/PROTOCOL.md` | Serial line protocol |
| `docs/PLAYER.md` | Player turntable variant |
| `docs/TACHOMETER.md` | Optional optical index monitor + marking/machining |
| `docs/CALIBRATION.md` | Open-loop platter speed calibration |
| `hardware/` | Drive/bearing/belt/stepper/phono notes + BOM |
| `deploy/` | systemd, PipeWire, OS tune |

## Player variant

A lower-cost, playback-only turntable on an **Arduino Nano only** (no Pi, no UI):
33/45/78 with ±8% pitch and start/stop, reusing the lathe belt drive, TMC2209
driver and motion core. See [docs/PLAYER.md](docs/PLAYER.md).

## Cut v1

Constant pitch only. Groove feed is `mm/s = rpm/60 * (25.4 / lpi)`. Variable-pitch preview is reserved for v2.

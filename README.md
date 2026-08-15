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
| `firmware/krrl_mega/` | Mega 2560 sketch |
| `host/krrl/` | Python host |
| `ui/` | Operator console |
| `config/machine.yaml` | Calibration and devices |
| `docs/PROTOCOL.md` | Serial line protocol |
| `deploy/` | systemd, PipeWire, OS tune |

## Cut v1

Constant pitch only. Groove feed is `mm/s = rpm/60 * (25.4 / lpi)`. Variable-pitch preview is reserved for v2.

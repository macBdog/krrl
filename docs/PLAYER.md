# KRRL-01 Player (Nano variant)

A separate, lower-cost **playback** turntable. Where the lathe pairs a
Raspberry Pi with an Arduino Mega, the player is an **Arduino Nano only** — pure
hardware, no host, no UI, no serial protocol.

## What it does

- Playback at **33⅓ / 45 / 78** rpm, selected by a single **mode button**
  (cycles STOP → 33 → 45 → 78) with a **multicolour LED** showing the mode.
- Continuous **speed pot** (pitch trim) of **±8%** (centre detent = 0%).
- Smooth rate-slewed belt spin-up/down.
- **Open-loop feedforward speed** — no runtime control loop and **no speed
  sensor**. Set absolute speed via [docs/CALIBRATION.md](CALIBRATION.md). The
  LED reflects the feedforward ramp reaching the commanded rate.

No cutter, heater, vacuum, camera, audio pipeline, homing or limits — none of the
lathe's cut path is present.

## What it shares with the lathe

- The **belt drive stepper motor** and pulley/belt ratio.
- The **TMC2209** driver (STEP/DIR at runtime, optional UART config at boot).
- The **motion core**: the same Timer1 phase-accumulator step generation and the
  same open-loop speed math (`sps = rpm / 60 * 3200`). The platter steps-per-rev
  matches the lathe (`firmware/krrl_mega/config.h`, `config/machine.yaml`) and
  the shared math lives in `firmware/krrl_player/playspeed.h`.
- The **open-loop platter drive** and the calibration approach. See
  [docs/CALIBRATION.md](CALIBRATION.md). (The optional optical index monitor is a
  lathe-only feature — [docs/TACHOMETER.md](TACHOMETER.md).)

## Controls

A single momentary **mode button**, a **speed pot**, a **multicolour (RGB) mode
LED** and a **power LED**. Wiring and pins are in
[`firmware/krrl_player/README.md`](../firmware/krrl_player/README.md); the
single-board design is in [`hardware/player_pcb/`](../hardware/player_pcb).

| Control | Action |
|---------|--------|
| Mode button | Cycle STOP → 33 → 45 → 78 (STOP spins down) |
| Speed pot | Continuous ±8% pitch trim of the running speed; centre = 0% |
| RGB mode LED | off = stopped, green = 33, blue = 45, red = 78; blinks while spinning up |
| Power LED | Hardwired to +5V; power present |

## Firmware

`firmware/krrl_player/` — build for **Arduino Nano** (ATmega328P). Default build
is STEP/DIR only and needs no libraries. See the firmware README for the TMC2209
UART option and the host-side test of the speed math.

## Board

A single fabricable KiCad PCB (Nano + TMC2209 + RIAA phono + I/O) is in
[`hardware/player_pcb/`](../hardware/player_pcb), which also specifies the
required external power supply.

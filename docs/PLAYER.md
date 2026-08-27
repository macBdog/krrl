# KRRL-01 Player (Nano variant)

A separate, lower-cost **playback** turntable. Where the lathe pairs a
Raspberry Pi with an Arduino Mega, the player is an **Arduino Nano only** — pure
hardware, no host, no UI, no serial protocol.

## What it does

- Playback at **33⅓ / 45 / 78** rpm.
- Continuous **pitch fader** of **±8%** (centre detent = 0%).
- **Start / Stop** with a smooth rate-slewed belt spin-up.
- **Open-loop feedforward speed** (no runtime control loop); set absolute speed
  via [docs/CALIBRATION.md](CALIBRATION.md). An optional once-per-rev optical
  index drives the at-speed LED ([docs/TACHOMETER.md](TACHOMETER.md)).

No cutter, heater, vacuum, camera, audio pipeline, homing or limits — none of the
lathe's cut path is present.

## What it shares with the lathe

- The **belt drive stepper motor** and pulley/belt ratio.
- The **TMC2209** driver (STEP/DIR at runtime, optional UART config at boot).
- The **motion core**: the same Timer1 phase-accumulator step generation and the
  same open-loop speed math (`sps = rpm / 60 * 3200`). The platter steps-per-rev
  matches the lathe (`firmware/krrl_mega/config.h`, `config/machine.yaml`) and
  the shared math lives in `firmware/krrl_player/playspeed.h`.
- The **open-loop platter drive** and the optional single-index optical monitor
  and its marking scheme. See [docs/TACHOMETER.md](TACHOMETER.md) and
  [docs/CALIBRATION.md](CALIBRATION.md).

## Controls

Five momentary buttons, one pitch fader and the onboard LED. Wiring and pins are
in [`firmware/krrl_player/README.md`](../firmware/krrl_player/README.md).

| Control | Action |
|---------|--------|
| 33 / 45 / 78 | Select nominal speed (persists across start/stop) |
| START / STOP | Spin platter up / down |
| Pitch fader | Continuous ±8% trim of running speed; centre detent = 0% |

LED: solid while running at nominal speed, blinking while a pitch trim is applied.

## Firmware

`firmware/krrl_player/` — build for **Arduino Nano** (ATmega328P). Default build
is STEP/DIR only and needs no libraries. See the firmware README for the TMC2209
UART option and the host-side test of the speed math.

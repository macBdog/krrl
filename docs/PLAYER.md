# KRRL-01 Player (Nano variant)

A separate, lower-cost **playback** turntable. Where the lathe pairs a
Raspberry Pi with an Arduino Mega, the player is an **Arduino Nano only** — pure
hardware, no host, no UI, no serial protocol.

## What it does

- Playback at **33⅓ / 45 / 78** rpm.
- **Pitch** trim of **±8%**, up/down, hold to sweep.
- **Start / Stop** with a smooth rate-slewed belt spin-up.

No cutter, heater, vacuum, camera, audio pipeline, homing or limits — none of the
lathe's cut path is present.

## What it shares with the lathe

- The **belt drive stepper motor** and pulley/belt ratio.
- The **TMC2209** driver (STEP/DIR at runtime, optional UART config at boot).
- The **motion core**: the same Timer1 phase-accumulator step generation and the
  same open-loop speed math (`sps = rpm / 60 * 3200`). The platter steps-per-rev
  matches the lathe (`firmware/krrl_mega/config.h`, `config/machine.yaml`) and
  the shared math lives in `firmware/krrl_player/playspeed.h`.

## Controls

Seven momentary buttons and the onboard LED. Wiring and pins are in
[`firmware/krrl_player/README.md`](../firmware/krrl_player/README.md).

| Button | Action |
|--------|--------|
| 33 / 45 / 78 | Select nominal speed (persists across start/stop) |
| START / STOP | Spin platter up / down |
| PITCH+ / PITCH− | Trim running speed ±8% (hold to sweep; both = reset) |

LED: solid while running at nominal speed, blinking while a pitch trim is applied.

## Firmware

`firmware/krrl_player/` — build for **Arduino Nano** (ATmega328P). Default build
is STEP/DIR only and needs no libraries. See the firmware README for the TMC2209
UART option and the host-side test of the speed math.

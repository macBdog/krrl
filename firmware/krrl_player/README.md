# Nano player firmware

Minimal belt-drive **playback** turntable: 33/45/78 with a ±8% pitch trim,
start/stop, and nothing else. No host, no serial protocol, no cutter, heater or
vacuum. It reuses the KRRL-01 belt drive motor, TMC2209 driver and the lathe's
open-loop step-rate motion core (`playspeed.h`, `platter.cpp` derived from
`firmware/krrl_mega/motion.cpp`).

Open `krrl_player.ino` in Arduino IDE (board: **Arduino Nano**, ATmega328P).
The default build is STEP/DIR only and needs no libraries.

## Wiring (Arduino Nano)

| Signal | Pin | Notes |
|--------|-----|-------|
| Platter STEP | D3 | to TMC2209 STEP |
| Platter DIR | D4 | to TMC2209 DIR |
| Platter EN | D5 | active LOW |
| 33 select | D6 | momentary to GND |
| 45 select | D7 | momentary to GND |
| 78 select | D8 | momentary to GND |
| START | D9 | momentary to GND |
| STOP | D10 | momentary to GND |
| Pitch fader | A0 | 10k linear pot wiper; ends to 5V and GND |
| Run LED | D13 | onboard; solid = running, blink = pitch trimmed |

Buttons are `INPUT_PULLUP`, wired button-to-ground. The pitch fader is a 10k
linear potentiometer: outer legs to 5V and GND, wiper to A0. Pins are in
`config.h`.

Pitch is continuous: the fader **centre detent reads 0%** (a small deadband
holds nominal speed), and each end reaches **±8%**. Firmware smooths the reading
so the platter speed doesn't jitter.

## TMC2209 current / microsteps (optional)

Default is STEP/DIR only; the driver runs at its jumper defaults. To set current
and microsteps over UART at boot:

1. Install [TMCStepper](https://github.com/teemuatlut/TMCStepper).
2. Build with `-DTMC_UART`.
3. Wire PDN/UART to the Nano hardware UART (D0/D1); driver address `00`
   (MS1/MS2 low), same as the lathe platter.

## Speed math

`playspeed.h` is shared, pure C, and unit-tested on the host:

```
cd firmware/krrl_player/test
c++ -std=c++11 -o test_playspeed test_playspeed.cpp && ./test_playspeed
```

Step rate is `sps = rpm / 60 * 3200`, matching the lathe platter
(`PLATTER_STEPS_PER_REV`). Pitch is multiplicative: `rpm * (1 ± pct/100)`.

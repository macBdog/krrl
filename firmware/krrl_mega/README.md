# Mega 2560 firmware

Open `krrl_mega.ino` in Arduino IDE (board: **Arduino Mega or Mega 2560**).

Default build is STEP/DIR only and does not need libraries.

To configure TMC2209 current/microsteps over UART at boot:

1. Install [TMCStepper](https://github.com/teemuatlut/TMCStepper)
2. Add compile flag `-DTMC_UART` (Arduino CLI) or `#define TMC_UART` in `config.h`
3. Wire PDN/UART to Serial1 (pins 18/19) with the usual 1 kΩ TX/RX arrangement
4. Set MS1/MS2 addresses: platter 00, X 01, Z 10

Pins match `config.h` and the comments in `config/machine.yaml`.

## Platter speed: open-loop feedforward

The platter runs open-loop — the step rate sets the speed, with no runtime
control loop. Set absolute speed by **calibration**
([`docs/CALIBRATION.md`](../../docs/CALIBRATION.md)); the fine trim is
`PLATTER_CAL_PPM` in `config.h` (or adjust `PLATTER_STEPS_PER_REV`).

The 1-pulse-per-rev optical tach (`PIN_TACH`, pin 3) is a **passive monitor**:
it feeds telemetry `rpm` and the `START` at-speed interlock but does not steer
the step rate. See [`docs/TACHOMETER.md`](../../docs/TACHOMETER.md) for the
sensor and the platter marking/machining (shared with the player variant).

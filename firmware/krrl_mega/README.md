# Mega 2560 firmware

Open `krrl_mega.ino` in Arduino IDE (board: **Arduino Mega or Mega 2560**).

Default build is STEP/DIR only and does not need libraries.

To configure TMC2209 current/microsteps over UART at boot:

1. Install [TMCStepper](https://github.com/teemuatlut/TMCStepper)
2. Add compile flag `-DTMC_UART` (Arduino CLI) or `#define TMC_UART` in `config.h`
3. Wire PDN/UART to Serial1 (pins 18/19) with the usual 1 kΩ TX/RX arrangement
4. Set MS1/MS2 addresses: platter 00, X 01, Z 10

Pins match `config.h` and the comments in `config/machine.yaml`.

## Closed-loop platter speed

The platter runs closed-loop on a 1-pulse-per-rev optical tachometer (`PIN_TACH`,
pin 3). Feedforward step rate plus a proportional trim from the measured RPM; see
[`docs/TACHOMETER.md`](../../docs/TACHOMETER.md) for how it works and how to mark
and machine the platter (shared with the player variant).

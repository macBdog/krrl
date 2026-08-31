#include "config.h"
#include "playspeed.h"
#include "platter.h"
#include "controls.h"
#include "tmc.h"

/* KRRL-01 Player: hardware-only belt-drive turntable on an Arduino Nano.
 *
 * Panel: ( MODE momentary )  ( SPEED pot )  [RGB mode LED]  [power LED]
 *  - MODE cycles STOP -> 33 -> 45 -> 78 -> STOP. STOP spins the platter down.
 *  - SPEED pot is a continuous pitch trim: centre detent = 0%, ends = +/-8%.
 *  - RGB LED shows the mode: off = stopped, green = 33, blue = 45, red = 78;
 *    it blinks while spinning up and goes solid at speed.
 *  - Power LED is hardwired to +5V on the board.
 *  - Speed is open-loop feedforward; calibrate per docs/CALIBRATION.md. */

enum Mode { MODE_STOP = 0, MODE_33, MODE_45, MODE_78, MODE_COUNT };

static uint8_t mode = MODE_STOP;
static float pitch_pct = 0.0f;

static float mode_rpm(uint8_t m) {
  switch (m) {
    case MODE_33: return KRRL_RPM_33;
    case MODE_45: return KRRL_RPM_45;
    case MODE_78: return KRRL_RPM_78;
    default:      return 0.0f;   /* STOP */
  }
}

static void apply_speed() {
  platter_set_target_rpm(krrl_pitched_rpm(mode_rpm(mode), pitch_pct));
}

/* Common-cathode RGB: HIGH = on. Off when stopped; the mode colour blinks
 * while spinning up and is solid once at speed. */
static void update_led() {
  bool r = mode == MODE_78;
  bool g = mode == MODE_33;
  bool b = mode == MODE_45;
  bool on = mode != MODE_STOP &&
            (platter_at_speed() || ((millis() / 250) & 1));
  digitalWrite(PIN_LED_R, (r && on) ? HIGH : LOW);
  digitalWrite(PIN_LED_G, (g && on) ? HIGH : LOW);
  digitalWrite(PIN_LED_B, (b && on) ? HIGH : LOW);
}

void setup() {
  tmc_begin();          /* optional TMC2209 UART config (D0/D1) */
  platter_begin();
  controls_begin();
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  update_led();
  apply_speed();
}

void loop() {
  controls_poll();

  if (press_mode()) mode = (mode + 1) % MODE_COUNT;
  pitch_pct = controls_pitch_pct();

  apply_speed();
  platter_poll();
  update_led();
}

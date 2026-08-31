#include "config.h"
#include "playspeed.h"
#include "platter.h"
#include "controls.h"
#include "tmc.h"

/* KRRL-01 Player: hardware-only belt-drive turntable on an Arduino Nano.
 *
 * Panel: [33] [45] [78]  [START] [STOP]  ( PITCH fader )
 *  - 33/45/78 select the nominal speed (persists across start/stop).
 *  - START/STOP spin the platter up/down (rate-slewed for the belt).
 *  - PITCH is a continuous fader: centre detent = 0%, ends = +/-8%.
 *  - Speed is open-loop feedforward; calibrate per docs/CALIBRATION.md.
 *  - Onboard LED: off = stopped, blinking = spinning up, solid = at speed. */

static float base_rpm = DEFAULT_RPM;
static float pitch_pct = 0.0f;
static bool running = false;

static void apply_speed() {
  float rpm = running ? krrl_pitched_rpm(base_rpm, pitch_pct) : 0.0f;
  platter_set_target_rpm(rpm);
}

static void update_led() {
  if (!running) {
    digitalWrite(PIN_LED_RUN, LOW);
  } else if (platter_at_speed()) {
    digitalWrite(PIN_LED_RUN, HIGH);
  } else {
    digitalWrite(PIN_LED_RUN, (millis() / 250) & 1 ? HIGH : LOW);
  }
}

void setup() {
  tmc_begin();          /* optional TMC2209 UART config (D0/D1) */
  platter_begin();
  controls_begin();
  pinMode(PIN_LED_RUN, OUTPUT);
  digitalWrite(PIN_LED_RUN, LOW);
  apply_speed();
}

void loop() {
  controls_poll();

  if (press_33()) base_rpm = KRRL_RPM_33;
  if (press_45()) base_rpm = KRRL_RPM_45;
  if (press_78()) base_rpm = KRRL_RPM_78;
  if (press_start()) running = true;
  if (press_stop()) running = false;
  pitch_pct = controls_pitch_pct();

  apply_speed();
  platter_poll();
  update_led();
}

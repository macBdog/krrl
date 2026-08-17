#include "platter.h"

/* Live step rate consumed by the ISR (signed; player only drives forward). */
static volatile int32_t rate_sps;
static volatile uint32_t acc;
static volatile uint8_t pulse_hi;
static volatile uint8_t dir_fwd;

/* Optical tachometer: one mark per rev, timed by the input-edge ISR. */
static volatile uint32_t tach_us;    /* last mark-to-mark period */
static volatile uint32_t tach_last;  /* micros() of the last mark */
static volatile uint8_t tach_fresh;  /* a new period is waiting */
static volatile uint8_t tach_seen;   /* any mark ever detected */

static float target_rpm;
static int32_t ff_sps;               /* open-loop feedforward rate */
static int32_t base_sps;             /* slewed feedforward (spin-up/down) */
static float measured_rpm;
static uint32_t last_slew_ms;

/* Same phase-accumulator stepping as the lathe: every tick add |rate| to the
 * accumulator and emit one step whenever it crosses ISR_HZ. A step is a short
 * HIGH pulse cleared on the next tick. */
ISR(TIMER1_COMPA_vect) {
  if (pulse_hi) {
    digitalWrite(PIN_PLATTER_STEP, LOW);
    pulse_hi = 0;
    return;
  }
  int32_t r = rate_sps;
  if (r == 0) return;
  uint32_t ar = (uint32_t)(r < 0 ? -r : r);
  acc += ar;
  if (acc >= ISR_HZ) {
    acc -= ISR_HZ;
    uint8_t fwd = r > 0;
    if (fwd != dir_fwd) {
      dir_fwd = fwd;
      digitalWrite(PIN_PLATTER_DIR, fwd ? HIGH : LOW);
    }
    digitalWrite(PIN_PLATTER_STEP, HIGH);
    pulse_hi = 1;
  }
}

/* Reject marks closer than 2 ms (edge noise) or slower than 2 s, matching the
 * lathe tach ISR. */
void platter_tach_isr() {
  uint32_t now = micros();
  uint32_t dt = now - tach_last;
  tach_last = now;
  if (dt > 2000UL && dt < 2000000UL) {
    tach_us = dt;
    tach_fresh = 1;
    tach_seen = 1;
  }
}

void platter_begin() {
  pinMode(PIN_PLATTER_STEP, OUTPUT);
  pinMode(PIN_PLATTER_DIR, OUTPUT);
  pinMode(PIN_PLATTER_EN, OUTPUT);
  digitalWrite(PIN_PLATTER_STEP, LOW);
  digitalWrite(PIN_PLATTER_DIR, HIGH);
  digitalWrite(PIN_PLATTER_EN, LOW); /* TMC2209 enable is active LOW */
  rate_sps = 0;
  acc = 0;
  pulse_hi = 0;
  dir_fwd = 1;
  target_rpm = 0;
  ff_sps = 0;
  base_sps = 0;
  measured_rpm = 0;
  last_slew_ms = millis();

  pinMode(PIN_TACH, INPUT_PULLUP);
  tach_last = micros();
  attachInterrupt(digitalPinToInterrupt(PIN_TACH), platter_tach_isr, RISING);

  noInterrupts();
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS11); /* CTC, prescaler /8 */
  OCR1A = (uint16_t)(F_CPU / 8 / ISR_HZ - 1);
  TIMSK1 = _BV(OCIE1A);
  interrupts();
}

void platter_set_target_rpm(float rpm) {
  if (rpm < 0) rpm = 0;
  target_rpm = rpm;
  ff_sps = krrl_rpm_to_sps(rpm);
}

float platter_measured_rpm() { return measured_rpm; }

bool platter_locked() {
  if (target_rpm <= 0.01f) return false;
  float e = measured_rpm - target_rpm;
  if (e < 0) e = -e;
  return e <= RPM_BAND;
}

int32_t platter_rate_sps() {
  noInterrupts();
  int32_t r = rate_sps;
  interrupts();
  return r;
}

static float sps_to_rpm(int32_t sps) {
  return (float)sps * 60.0f / KRRL_PLATTER_STEPS_PER_REV;
}

void platter_poll() {
  uint32_t now = millis();
  uint32_t dt = now - last_slew_ms;
  if (dt == 0) return;
  last_slew_ms = now;

  /* Smoothly ramp the feedforward rate toward the commanded speed. */
  int32_t max_step = (int32_t)(SLEW_SPS_PER_MS * (float)dt);
  base_sps = krrl_slew_toward(base_sps, ff_sps, max_step);

  int32_t cmd = base_sps;

  if (target_rpm > 0.01f) {
    if (tach_fresh) {
      noInterrupts();
      uint32_t p = tach_us;
      tach_fresh = 0;
      interrupts();
      measured_rpm = krrl_tach_rpm(p, KRRL_TACH_PPR);
    } else if (!tach_seen) {
      /* No optical sensor fitted (or not spinning yet): open-loop estimate. */
      measured_rpm = sps_to_rpm(base_sps);
    }
    /* Close the loop only when the tach is actually reporting, and keep the
     * trim a fine correction so it never overpowers the spin-up slew. */
    if (tach_seen) {
      int32_t trim = krrl_tach_trim_sps(target_rpm, measured_rpm,
                                        KRRL_TACH_KP_SPS_PER_RPM);
      int32_t lim = ff_sps / 8;
      if (lim < 1) lim = 1;
      if (trim > lim) trim = lim;
      if (trim < -lim) trim = -lim;
      cmd = base_sps + trim;
    }
    if (cmd < 0) cmd = 0;
  } else {
    measured_rpm = 0;
  }

  noInterrupts();
  rate_sps = cmd;
  interrupts();
}

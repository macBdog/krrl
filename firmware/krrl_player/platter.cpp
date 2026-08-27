#include "platter.h"

/* Live step rate consumed by the ISR (signed; player only drives forward). */
static volatile int32_t rate_sps;
static volatile uint32_t acc;
static volatile uint8_t pulse_hi;
static volatile uint8_t dir_fwd;

static int32_t ff_sps;               /* open-loop feedforward rate */
static int32_t base_sps;             /* slewed feedforward (spin-up/down) */
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
  ff_sps = 0;
  base_sps = 0;
  last_slew_ms = millis();

  noInterrupts();
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS11); /* CTC, prescaler /8 */
  OCR1A = (uint16_t)(F_CPU / 8 / ISR_HZ - 1);
  TIMSK1 = _BV(OCIE1A);
  interrupts();
}

void platter_set_target_rpm(float rpm) {
  if (rpm < 0) rpm = 0;
  ff_sps = krrl_rpm_to_sps(rpm);
}

int32_t platter_rate_sps() {
  noInterrupts();
  int32_t r = rate_sps;
  interrupts();
  return r;
}

/* Open-loop "at speed": the slewed feedforward has reached the commanded rate. */
bool platter_at_speed() {
  return ff_sps > 0 && platter_rate_sps() == ff_sps;
}

void platter_poll() {
  uint32_t now = millis();
  uint32_t dt = now - last_slew_ms;
  if (dt == 0) return;
  last_slew_ms = now;

  /* Open-loop feedforward: smoothly ramp the commanded step rate toward the
   * target. Absolute speed is set by calibration (see docs/CALIBRATION.md). */
  int32_t max_step = (int32_t)(SLEW_SPS_PER_MS * (float)dt);
  base_sps = krrl_slew_toward(base_sps, ff_sps, max_step);

  noInterrupts();
  rate_sps = base_sps;
  interrupts();
}

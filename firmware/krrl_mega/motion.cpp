#include "motion.h"
#include "safety.h"

extern State g_state;

static const uint8_t STEP_PIN[3] = {PIN_PLATTER_STEP, PIN_X_STEP, PIN_Z_STEP};
static const uint8_t DIR_PIN[3]  = {PIN_PLATTER_DIR, PIN_X_DIR, PIN_Z_DIR};
static const uint8_t EN_PIN[3]   = {PIN_PLATTER_EN, PIN_X_EN, PIN_Z_EN};

volatile int32_t pos_steps[3];
volatile int32_t rate_sps[3];   // signed steps / s
volatile uint32_t acc[3];
volatile uint8_t pulse_hi[3];
volatile uint8_t dir_fwd[3];

static int32_t target_steps[3];
static uint8_t has_target[3];
static float target_rpm;
static float measured_rpm;
static uint8_t x_homed, z_homed;
static uint8_t home_mask;
static uint8_t home_phase;
static volatile uint32_t tach_us;
static volatile uint32_t tach_last;
static volatile uint8_t tach_fresh;

static int32_t x_mm_to_steps(float mm) { return (int32_t)(mm * X_STEPS_PER_MM + 0.5f); }
static int32_t z_mm_to_steps(float mm) { return (int32_t)(mm * Z_STEPS_PER_MM + 0.5f); }

ISR(TIMER1_COMPA_vect) {
  for (uint8_t i = 0; i < 3; i++) {
    if (pulse_hi[i]) {
      digitalWrite(STEP_PIN[i], LOW);
      pulse_hi[i] = 0;
      continue;
    }
    int32_t r = rate_sps[i];
    if (r == 0) continue;
    uint32_t ar = (uint32_t)(r < 0 ? -r : r);
    acc[i] += ar;
    if (acc[i] >= ISR_HZ) {
      acc[i] -= ISR_HZ;
      uint8_t fwd = r > 0;
      if (fwd != dir_fwd[i]) {
        dir_fwd[i] = fwd;
        digitalWrite(DIR_PIN[i], fwd ? HIGH : LOW);
      }
      digitalWrite(STEP_PIN[i], HIGH);
      pulse_hi[i] = 1;
      pos_steps[i] += fwd ? 1 : -1;
    }
  }
}

void motion_tach_isr() {
  uint32_t now = micros();
  uint32_t dt = now - tach_last;
  tach_last = now;
  if (dt > 2000UL && dt < 2000000UL) {
    tach_us = dt;
    tach_fresh = 1;
  }
}

void motion_begin() {
  for (uint8_t i = 0; i < 3; i++) {
    pinMode(STEP_PIN[i], OUTPUT);
    pinMode(DIR_PIN[i], OUTPUT);
    pinMode(EN_PIN[i], OUTPUT);
    digitalWrite(EN_PIN[i], LOW);
    digitalWrite(STEP_PIN[i], LOW);
    pos_steps[i] = 0;
    rate_sps[i] = 0;
    acc[i] = 0;
    pulse_hi[i] = 0;
    dir_fwd[i] = 1;
    has_target[i] = 0;
  }
  pos_steps[AXIS_X] = x_mm_to_steps(X_MAX_MM);
  pinMode(PIN_TACH, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_TACH), motion_tach_isr, RISING);

  noInterrupts();
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS11); /* CTC, /8 */
  OCR1A = (uint16_t)(F_CPU / 8 / ISR_HZ - 1);
  TIMSK1 = _BV(OCIE1A);
  interrupts();
}

static void set_rate(uint8_t i, int32_t sps) { rate_sps[i] = sps; }

static void servo_to_target(uint8_t i, int32_t max_sps) {
  if (!has_target[i]) return;
  int32_t err = target_steps[i] - pos_steps[i];
  if (err > -2 && err < 2) {
    set_rate(i, 0);
    has_target[i] = 0;
    return;
  }
  int32_t sps = err;
  if (sps > max_sps) sps = max_sps;
  if (sps < -max_sps) sps = -max_sps;
  if (sps > 0 && sps < 50) sps = 50;
  if (sps < 0 && sps > -50) sps = -50;
  set_rate(i, sps);
}

void motion_set_rpm(float rpm) {
  if (rpm < 0) rpm = 0;
  if (rpm > 80) rpm = 80;
  target_rpm = rpm;
  if (rpm <= 0.01f) {
    set_rate(AXIS_P, 0);
    measured_rpm = 0;
  }
}

void motion_set_xvel_mm_s(float mm_s) {
  has_target[AXIS_X] = 0;
  int32_t sps = (int32_t)(mm_s * X_STEPS_PER_MM);
  set_rate(AXIS_X, sps);
}

void motion_set_x_mm(float mm) {
  if (mm < X_MIN_MM) mm = X_MIN_MM;
  if (mm > X_MAX_MM) mm = X_MAX_MM;
  target_steps[AXIS_X] = x_mm_to_steps(mm);
  has_target[AXIS_X] = 1;
}

void motion_set_z_mm(float mm) {
  if (mm < 0) mm = 0;
  if (mm > Z_MAX_MM) mm = Z_MAX_MM;
  target_steps[AXIS_Z] = z_mm_to_steps(mm);
  has_target[AXIS_Z] = 1;
}

void motion_jog_x_mm(float d) { motion_set_x_mm(motion_x_mm() + d); }
void motion_jog_z_mm(float d) { motion_set_z_mm(motion_z_mm() + d); }

void motion_stop_feed() {
  has_target[AXIS_X] = 0;
  set_rate(AXIS_X, 0);
}

void motion_zero_x() {
  pos_steps[AXIS_X] = x_mm_to_steps(X_MAX_MM);
  x_homed = 1;
}

void motion_abort_move() {
  has_target[AXIS_X] = 0;
  set_rate(AXIS_X, 0);
  motion_set_rpm(0);
  motion_set_z_mm(0);
}

void motion_home(uint8_t axis_mask) {
  home_mask = axis_mask;
  home_phase = 1;
  g_state = ST_HOMING;
  if (axis_mask & 1) {
    x_homed = 0;
    has_target[AXIS_X] = 0;
    set_rate(AXIS_X, (int32_t)(4.0f * X_STEPS_PER_MM)); /* + toward outer */
  }
  if (axis_mask & 2) {
    z_homed = 0;
    has_target[AXIS_Z] = 0;
    set_rate(AXIS_Z, (int32_t)(-2.0f * Z_STEPS_PER_MM)); /* - toward up */
  }
}

float motion_rpm() { return measured_rpm; }
float motion_x_mm() { return pos_steps[AXIS_X] / X_STEPS_PER_MM; }
float motion_z_mm() { return pos_steps[AXIS_Z] / Z_STEPS_PER_MM; }
bool motion_x_homed() { return x_homed; }
bool motion_z_homed() { return z_homed; }
bool motion_at_speed() {
  if (target_rpm <= 0.01f) return true;
  float e = measured_rpm - target_rpm;
  if (e < 0) e = -e;
  return e <= RPM_BAND;
}
bool motion_busy() { return has_target[AXIS_X] || has_target[AXIS_Z] || home_phase; }

void motion_poll() {
  if (safety_estop() || safety_aborting()) {
    set_rate(AXIS_P, 0);
    set_rate(AXIS_X, 0);
    return;
  }

  if (digitalRead(PIN_X_MAX) == LOW && rate_sps[AXIS_X] > 0) set_rate(AXIS_X, 0);
  if (digitalRead(PIN_X_MIN) == LOW && rate_sps[AXIS_X] < 0) set_rate(AXIS_X, 0);
  if (digitalRead(PIN_Z_MAX) == LOW && rate_sps[AXIS_Z] > 0) set_rate(AXIS_Z, 0);
  if (digitalRead(PIN_Z_MIN) == LOW && rate_sps[AXIS_Z] < 0) set_rate(AXIS_Z, 0);

  if (home_phase) {
    if ((home_mask & 1) && digitalRead(PIN_X_MAX) == LOW) {
      set_rate(AXIS_X, 0);
      pos_steps[AXIS_X] = x_mm_to_steps(X_MAX_MM);
      x_homed = 1;
      home_mask &= ~1;
    }
    if ((home_mask & 2) && digitalRead(PIN_Z_MIN) == LOW) {
      set_rate(AXIS_Z, 0);
      pos_steps[AXIS_Z] = 0;
      z_homed = 1;
      home_mask &= ~2;
    }
    if (home_mask == 0) {
      home_phase = 0;
      g_state = ST_READY;
    }
  }

  servo_to_target(AXIS_X, (int32_t)(8.0f * X_STEPS_PER_MM));
  servo_to_target(AXIS_Z, (int32_t)(4.0f * Z_STEPS_PER_MM));

  if (target_rpm > 0.01f) {
    /* Open-loop feedforward with an optional ppm calibration trim. The optical
     * tach is a passive speed monitor (telemetry + at-speed interlock); it does
     * not steer the step rate. See docs/CALIBRATION.md. */
    float cal = 1.0f + (float)PLATTER_CAL_PPM / 1000000.0f;
    int32_t open_sps = (int32_t)(target_rpm / 60.0f * PLATTER_STEPS_PER_REV * cal);
    if (tach_fresh) {
      noInterrupts();
      uint32_t dt = tach_us;
      tach_fresh = 0;
      interrupts();
      measured_rpm = 60000000.0f / (float)dt;
    } else {
      measured_rpm = target_rpm; /* no tach: assume feedforward speed */
    }
    if (open_sps < 0) open_sps = 0;
    set_rate(AXIS_P, open_sps);
  } else {
    measured_rpm = 0;
  }
}

#include "heater.h"
#include "config.h"
#include "safety.h"
#include <math.h>

static float target_c;
static float meas_c;
static float i_term;
static float prev_err;
static uint32_t last_ms;

void heater_begin() {
  pinMode(PIN_HEATER, OUTPUT);
  analogWrite(PIN_HEATER, 0);
  target_c = 0;
  meas_c = 25;
  i_term = 0;
  last_ms = millis();
}

static float ntc_c() {
  int raw = analogRead(PIN_THERM);
  if (raw < 8 || raw > 1015) return -100; /* open / short */
  float r = 10000.0f * (float)raw / (1023.0f - (float)raw);
  const float b = 3950.0f;
  float t = 1.0f / (1.0f / 298.15f + log(r / 10000.0f) / b);
  return t - 273.15f;
}

void heater_set_c(float c) {
  if (c < 0) c = 0;
  if (c > HEATER_MAX_C) c = HEATER_MAX_C;
  target_c = c;
  if (c <= 0.5f) {
    analogWrite(PIN_HEATER, 0);
    i_term = 0;
  }
}

float heater_c() { return meas_c; }
float heater_target_c() { return target_c; }

bool heater_in_band() {
  if (target_c <= 0.5f) return true;
  float e = meas_c - target_c;
  if (e < 0) e = -e;
  return e <= HEAT_BAND_C;
}

void heater_off() { heater_set_c(0); }

void heater_poll() {
  uint32_t now = millis();
  float dt = (now - last_ms) / 1000.0f;
  if (dt < 0.02f) return;
  last_ms = now;
  meas_c = ntc_c();

  if (safety_estop() || target_c <= 0.5f) {
    analogWrite(PIN_HEATER, 0);
    i_term = 0;
    return;
  }
  if (meas_c < -50) {
    analogWrite(PIN_HEATER, 0);
    return;
  }

  float err = target_c - meas_c;
  i_term += err * dt;
  if (i_term > 40) i_term = 40;
  if (i_term < -40) i_term = -40;
  float d = (err - prev_err) / dt;
  prev_err = err;
  float u = HEATER_KP * err + HEATER_KI * i_term + HEATER_KD * d;
  int pwm = (int)u;
  if (pwm < 0) pwm = 0;
  if (pwm > 255) pwm = 255;
  analogWrite(PIN_HEATER, pwm);
}

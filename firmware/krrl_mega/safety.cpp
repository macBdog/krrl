#include "safety.h"
#include "motion.h"
#include "heater.h"

static volatile uint8_t estop_flag;
static uint8_t aborting;
static uint32_t host_kick_ms;
static uint8_t host_seen;

static void estop_isr() { estop_flag = (digitalRead(PIN_ESTOP) == LOW); }

void safety_begin() {
  pinMode(PIN_ESTOP, INPUT_PULLUP);
  pinMode(PIN_X_MIN, INPUT_PULLUP);
  pinMode(PIN_X_MAX, INPUT_PULLUP);
  pinMode(PIN_Z_MIN, INPUT_PULLUP);
  pinMode(PIN_Z_MAX, INPUT_PULLUP);
  pinMode(PIN_VACUUM, OUTPUT);
  digitalWrite(PIN_VACUUM, LOW);
  attachInterrupt(digitalPinToInterrupt(PIN_ESTOP), estop_isr, CHANGE);
  estop_flag = (digitalRead(PIN_ESTOP) == LOW);
  host_kick_ms = millis();
}

void safety_kick_host() {
  host_kick_ms = millis();
  host_seen = 1;
}

bool safety_estop() { return estop_flag; }
bool safety_aborting() { return aborting; }

bool safety_limit_hit() {
  return digitalRead(PIN_X_MIN) == LOW || digitalRead(PIN_X_MAX) == LOW ||
         digitalRead(PIN_Z_MIN) == LOW || digitalRead(PIN_Z_MAX) == LOW;
}

void safety_request_abort() { aborting = 1; }

void safety_clear_abort() {
  if (!estop_flag) aborting = 0;
}

void safety_poll() {
  estop_flag = (digitalRead(PIN_ESTOP) == LOW);
  if (estop_flag) aborting = 1;

  if (host_seen && heater_target_c() > 0.5f) {
    if (millis() - host_kick_ms > HOST_WATCHDOG_MS) aborting = 1;
  }

  if (aborting) {
    motion_abort_move();
    heater_off();
  }
}

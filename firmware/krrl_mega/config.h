#pragma once

#include <Arduino.h>

#define KRRL_PROTO "KRRL/1"

#define PIN_PLATTER_STEP 22
#define PIN_PLATTER_DIR  23
#define PIN_PLATTER_EN   24
#define PIN_X_STEP       25
#define PIN_X_DIR        26
#define PIN_X_EN         27
#define PIN_Z_STEP       28
#define PIN_Z_DIR        29
#define PIN_Z_EN         30
#define PIN_VACUUM       31
#define PIN_X_MIN        32
#define PIN_X_MAX        33
#define PIN_Z_MIN        34
#define PIN_Z_MAX        35
#define PIN_HEATER       6
#define PIN_THERM        A0
#define PIN_ESTOP        2
#define PIN_TACH         3

#define AXIS_P 0
#define AXIS_X 1
#define AXIS_Z 2

#define ISR_HZ 20000UL
#define TEL_MS 50
#define HOST_WATCHDOG_MS 2000

#define X_STEPS_PER_MM 400.0f
#define Z_STEPS_PER_MM 2000.0f
#define PLATTER_STEPS_PER_REV 3200.0f
#define X_MIN_MM 52.0f
#define X_MAX_MM 160.0f
#define Z_RETRACT_MM 2.0f
#define Z_MAX_MM 4.0f

#define HEATER_MAX_C 220.0f
#define HEATER_KP 12.0f
#define HEATER_KI 0.4f
#define HEATER_KD 8.0f

#define RPM_BAND 0.3f
#define HEAT_BAND_C 5.0f

enum State : uint8_t {
  ST_IDLE = 0,
  ST_HOMING,
  ST_READY,
  ST_SPINUP,
  ST_CUT,
  ST_ABORT,
  ST_FAULT
};

const char *state_name(State s);

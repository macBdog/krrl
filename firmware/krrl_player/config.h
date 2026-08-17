#pragma once

#include <Arduino.h>
#include "playspeed.h"

/* KRRL-01 Player: minimal belt-drive turntable on an Arduino Nano.
 *
 * Playback only: 33/45/78 with a +/-8% pitch trim, start/stop. No host, no
 * serial protocol, no cutter/heater/vacuum. It shares the lathe's belt drive
 * motor, TMC2209 driver and the open-loop step-rate motion core. */

/* ---- Platter driver (single TMC2209, STEP/DIR) ---- */
#define PIN_PLATTER_STEP 3
#define PIN_PLATTER_DIR  4
#define PIN_PLATTER_EN   5   /* active LOW enable */

/* ---- Front-panel controls (momentary to GND, INPUT_PULLUP) ---- */
#define PIN_BTN_33       6
#define PIN_BTN_45       7
#define PIN_BTN_78       8
#define PIN_BTN_START    9
#define PIN_BTN_STOP     10
#define PIN_BTN_PITCH_UP 11
#define PIN_BTN_PITCH_DN 12

/* ---- Status LED (onboard) ---- */
#define PIN_LED_RUN      13  /* on = platter running, blink = pitch trimmed */

/* ---- Step generation ---- */
#define ISR_HZ 20000UL       /* Timer1 step tick, matches the lathe core */

/* ---- Control tunables ---- */
#define DEBOUNCE_MS      25
#define PITCH_REPEAT_MS  120   /* auto-repeat while a pitch button is held */
#define PITCH_STEP_PCT   0.2f  /* trim change per press/repeat */

/* Platter acceleration as a step-rate slew, so the belt starts and changes
 * speed smoothly instead of stalling. sps added/removed per control poll. */
#define SLEW_SPS_PER_MS  6.0f

/* Default speed selected at power-on. */
#define DEFAULT_RPM KRRL_RPM_33

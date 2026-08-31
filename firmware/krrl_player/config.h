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

/* ---- Front-panel buttons (momentary to GND, INPUT_PULLUP) ---- */
#define PIN_BTN_33       6
#define PIN_BTN_45       7
#define PIN_BTN_78       8
#define PIN_BTN_START    9
#define PIN_BTN_STOP     10

/* ---- Pitch fader (10k linear pot: ends to 5V/GND, wiper here) ---- */
#define PIN_PITCH_POT    A0

/* ---- Status LED (onboard) ---- */
#define PIN_LED_RUN      13  /* off = stopped, blink = spinning up, solid = at speed */

/* ---- Step generation ---- */
#define ISR_HZ 20000UL       /* Timer1 step tick, matches the lathe core */

/* ---- Control tunables ---- */
#define DEBOUNCE_MS      25
#define POT_MAX_COUNTS   1023  /* 10-bit ADC full scale */
#define POT_DEADBAND     10    /* +/- counts around centre that read as 0% */
#define POT_EMA_ALPHA    0.20f /* fader smoothing (0..1); higher = snappier */

/* Platter acceleration as a step-rate slew, so the belt starts and changes
 * speed smoothly instead of stalling. sps added/removed per control poll. */
#define SLEW_SPS_PER_MS  6.0f

/* Default speed selected at power-on. */
#define DEFAULT_RPM KRRL_RPM_33

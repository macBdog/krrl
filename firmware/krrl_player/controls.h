#pragma once

#include "config.h"

/* Debounced front-panel buttons plus the analog pitch fader. press_*() latch a
 * single event on the falling edge; controls_pitch_pct() returns the smoothed
 * fader position as a trim percent (centre = 0%, ends = +/-8%). */

void controls_begin();
void controls_poll();

bool press_33();
bool press_45();
bool press_78();
bool press_start();
bool press_stop();
float controls_pitch_pct();

#pragma once

#include "config.h"

/* Front-panel input: one debounced momentary mode button and the analog speed
 * pot. press_mode() latches a single event on the button's falling edge;
 * controls_pitch_pct() returns the smoothed pot position as a trim percent
 * (centre = 0%, ends = +/-8%). */

void controls_begin();
void controls_poll();

bool press_mode();
float controls_pitch_pct();

#pragma once

#include "config.h"

/* Debounced front-panel buttons. press_*() latch a single event on the
 * falling edge; hold_pitch_*() report the level for auto-repeat. */

void controls_begin();
void controls_poll();

bool press_33();
bool press_45();
bool press_78();
bool press_start();
bool press_stop();
bool hold_pitch_up();
bool hold_pitch_dn();

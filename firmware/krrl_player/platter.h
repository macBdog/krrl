#pragma once

#include "config.h"

/* Single-axis belt-drive platter, derived from the lathe motion core
 * (firmware/krrl_mega/motion.cpp, AXIS_P). Timer1 CTC ISR generates STEP
 * pulses from a phase accumulator; platter_poll() slews the feedforward rate
 * so the belt spins up and down smoothly. Speed is open-loop feedforward;
 * absolute speed is set by calibration, not runtime feedback (see
 * docs/CALIBRATION.md). */

void platter_begin();
void platter_poll();                 /* call often; slews the commanded rate */
void platter_set_target_rpm(float rpm);   /* 0 = stop */

bool platter_at_speed();             /* commanded rate has reached the target */
int32_t platter_rate_sps();          /* current commanded step rate */

#pragma once

#include "config.h"

/* Single-axis belt-drive platter, derived from the lathe motion core
 * (firmware/krrl_mega/motion.cpp, AXIS_P). Timer1 CTC ISR generates STEP
 * pulses from a phase accumulator; platter_poll() slews the live rate toward
 * the commanded rate so the belt spins up and down smoothly. */

void platter_begin();
void platter_poll();                 /* call often; slews rate toward target */
void platter_set_target_sps(int32_t sps);
int32_t platter_target_sps();
int32_t platter_rate_sps();          /* current (slewed) rate */
bool platter_at_target();

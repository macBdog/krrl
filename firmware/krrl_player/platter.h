#pragma once

#include "config.h"

/* Single-axis belt-drive platter, derived from the lathe motion core
 * (firmware/krrl_mega/motion.cpp, AXIS_P). Timer1 CTC ISR generates STEP
 * pulses from a phase accumulator; platter_poll() slews the feedforward rate
 * so the belt spins up and down smoothly. Speed is open-loop feedforward; the
 * optical tach (if fitted) is a passive at-speed monitor, not a control input.
 * Calibrate absolute speed per docs/CALIBRATION.md. */

void platter_begin();
void platter_poll();                 /* call often; slews rate, reads the tach */
void platter_set_target_rpm(float rpm);   /* 0 = stop */

float platter_measured_rpm();        /* passive: optical tach, or open-loop est. */
bool platter_locked();               /* running and within RPM_BAND of target */
int32_t platter_rate_sps();          /* current commanded step rate */

/* Optical-tach edge handler; attach in platter_begin(). Optional sensor. */
void platter_tach_isr();

#pragma once

#include "config.h"

/* Single-axis belt-drive platter, derived from the lathe motion core
 * (firmware/krrl_mega/motion.cpp, AXIS_P). Timer1 CTC ISR generates STEP
 * pulses from a phase accumulator; platter_poll() slews the feedforward rate
 * so the belt spins up and down smoothly, then closes the loop on the optical
 * tachometer with the same proportional trim the lathe uses. */

void platter_begin();
void platter_poll();                 /* call often; slews + applies tach trim */
void platter_set_target_rpm(float rpm);   /* 0 = stop */

float platter_measured_rpm();        /* from the optical tach (or open-loop) */
bool platter_locked();               /* running and within RPM_BAND of target */
int32_t platter_rate_sps();          /* current commanded step rate */

/* Optical-tach edge handler; attach in platter_begin(). */
void platter_tach_isr();

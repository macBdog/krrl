#pragma once

/* Optional TMC2209 UART setup for the platter driver. No-op unless the sketch
 * is built with -DTMC_UART (default build is STEP/DIR only, no libraries). */
void tmc_begin();

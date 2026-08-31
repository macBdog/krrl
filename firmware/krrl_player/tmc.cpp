#include "tmc.h"
#include "config.h"

/* Mirrors the lathe's platter driver setup (firmware/krrl_mega/tmc.cpp). The
 * Nano has a single hardware UART on D0/D1; with no host attached it is free
 * to configure the driver at boot. Address 0b00 matches the lathe platter. */
#ifdef TMC_UART
#include <TMCStepper.h>
#define R_SENSE 0.11f
static TMC2209Stepper tmc_p(&Serial, R_SENSE, 0b00);

static void setup_driver(TMC2209Stepper &d, uint16_t ma) {
  d.begin();
  d.pdn_disable(true);
  d.toff(5);
  d.rms_current(ma);
  d.microsteps(16);
  d.en_spreadCycle(false);
  d.pwm_autoscale(true);
  d.mstep_reg_select(true);
}
#endif

void tmc_begin() {
#ifdef TMC_UART
  Serial.begin(115200);
  setup_driver(tmc_p, 800); /* same platter current as the lathe */
#endif
}

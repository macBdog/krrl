#include "tmc.h"
#include "config.h"

#ifdef TMC_UART
#include <TMCStepper.h>
#define R_SENSE 0.11f
static TMC2209Stepper tmc_p(&Serial1, R_SENSE, 0b00);
static TMC2209Stepper tmc_x(&Serial1, R_SENSE, 0b01);
static TMC2209Stepper tmc_z(&Serial1, R_SENSE, 0b10);

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
  Serial1.begin(115200);
  setup_driver(tmc_p, 800);
  setup_driver(tmc_x, 900);
  setup_driver(tmc_z, 600);
#endif
}

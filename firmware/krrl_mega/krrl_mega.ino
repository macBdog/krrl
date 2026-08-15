#include "config.h"
#include "motion.h"
#include "heater.h"
#include "safety.h"
#include "protocol.h"
#include "tmc.h"

void setup() {
  Serial.begin(115200);
  safety_begin();
  motion_begin();
  heater_begin();
  tmc_begin();
  protocol_begin();
}

void loop() {
  protocol_poll();
  safety_poll();
  heater_poll();
  motion_poll();
  protocol_tel();
}

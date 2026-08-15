#pragma once

void heater_begin();
void heater_poll();
void heater_set_c(float c);
float heater_c();
float heater_target_c();
bool heater_in_band();
void heater_off();

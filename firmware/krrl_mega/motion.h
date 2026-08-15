#pragma once

#include "config.h"

void motion_begin();
void motion_poll();
void motion_set_rpm(float rpm);
void motion_set_xvel_mm_s(float mm_s);
void motion_set_x_mm(float mm);
void motion_set_z_mm(float mm);
void motion_jog_x_mm(float d);
void motion_jog_z_mm(float d);
void motion_home(uint8_t axis_mask);
void motion_stop_feed();
void motion_zero_x();
void motion_abort_move();

float motion_rpm();
float motion_x_mm();
float motion_z_mm();
bool motion_x_homed();
bool motion_z_homed();
bool motion_at_speed();
bool motion_busy();
void motion_tach_isr();

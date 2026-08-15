#pragma once

#include "config.h"

void safety_begin();
void safety_poll();
void safety_request_abort();
bool safety_estop();
bool safety_limit_hit();
bool safety_aborting();
void safety_clear_abort();
void safety_kick_host();

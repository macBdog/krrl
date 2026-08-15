#include "protocol.h"
#include "config.h"
#include "motion.h"
#include "heater.h"
#include "safety.h"

State g_state = ST_IDLE;
static uint8_t vac;
static char line[96];
static uint8_t llen;
static uint32_t last_tel;
static uint8_t saw_xhomed, saw_zhomed;

const char *state_name(State s) {
  switch (s) {
    case ST_IDLE: return "IDLE";
    case ST_HOMING: return "HOMING";
    case ST_READY: return "READY";
    case ST_SPINUP: return "SPINUP";
    case ST_CUT: return "CUT";
    case ST_ABORT: return "ABORT";
    case ST_FAULT: return "FAULT";
    default: return "IDLE";
  }
}

static void ok() { Serial.println(F("OK")); }
static void err(const __FlashStringHelper *m) {
  Serial.print(F("ERR "));
  Serial.println(m);
}

static void evt(const __FlashStringHelper *m) {
  Serial.print(F("EVT "));
  Serial.println(m);
}

static uint8_t start_interlocks() {
  if (safety_estop()) { err(F("ESTOP")); return 0; }
  if (!motion_x_homed() || !motion_z_homed()) { err(F("NOT_HOMED")); return 0; }
  if (!motion_at_speed()) { err(F("NOT_AT_SPEED")); return 0; }
  if (!heater_in_band()) { err(F("HEAT_BAND")); return 0; }
  if (heater_target_c() > 0.5f && !vac) { err(F("VAC_REQUIRED")); return 0; }
  return 1;
}

static void handle(char *s) {
  safety_kick_host();
  while (*s == ' ') s++;
  if (!s[0]) return;

  if (!strcmp(s, "HELLO KRRL/1")) {
    Serial.println(F("HELLO MEGA KRRL/1"));
    return;
  }
  if (!strcmp(s, "PING")) { Serial.println(F("PONG")); return; }
  if (!strcmp(s, "ABORT")) {
    safety_request_abort();
    g_state = ST_ABORT;
    ok();
    evt(F("ABORTED"));
    return;
  }

  if (safety_estop() && strncmp(s, "HELLO", 5)) {
    err(F("ESTOP"));
    return;
  }

  if (!strncmp(s, "SET RPM ", 8)) {
    motion_set_rpm(atof(s + 8));
    g_state = atof(s + 8) > 0 ? ST_SPINUP : ST_READY;
    ok();
    return;
  }
  if (!strncmp(s, "SET XVEL ", 9)) {
    motion_set_xvel_mm_s(atof(s + 9));
    ok();
    return;
  }
  if (!strncmp(s, "SET X ", 6)) { motion_set_x_mm(atof(s + 6)); ok(); return; }
  if (!strncmp(s, "SET Z ", 6)) { motion_set_z_mm(atof(s + 6)); ok(); return; }
  if (!strncmp(s, "HEAT ", 5)) { heater_set_c(atof(s + 5)); ok(); return; }
  if (!strncmp(s, "VAC ", 4)) {
    vac = atoi(s + 4) ? 1 : 0;
    digitalWrite(PIN_VACUUM, vac ? HIGH : LOW);
    ok();
    return;
  }
  if (!strncmp(s, "JOG X ", 6)) { motion_jog_x_mm(atof(s + 6)); ok(); return; }
  if (!strncmp(s, "JOG Z ", 6)) { motion_jog_z_mm(atof(s + 6)); ok(); return; }
  if (!strcmp(s, "HOME X")) { motion_home(1); ok(); return; }
  if (!strcmp(s, "HOME Z")) { motion_home(2); ok(); return; }
  if (!strcmp(s, "HOME ALL")) { motion_home(3); ok(); return; }
  if (!strcmp(s, "ZERO X")) { motion_zero_x(); ok(); return; }
  if (!strcmp(s, "START")) {
    if (!start_interlocks()) return;
    g_state = ST_CUT;
    ok();
    return;
  }
  err(F("UNKNOWN"));
}

void protocol_begin() {
  llen = 0;
  Serial.println(F("HELLO MEGA KRRL/1"));
}

void protocol_poll() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      line[llen] = 0;
      llen = 0;
      handle(line);
    } else if (llen < sizeof(line) - 1) {
      line[llen++] = c;
    } else {
      llen = 0;
    }
  }

  if (motion_x_homed() && !saw_xhomed) { saw_xhomed = 1; evt(F("HOMED X")); }
  if (motion_z_homed() && !saw_zhomed) { saw_zhomed = 1; evt(F("HOMED Z")); }
  if (!motion_x_homed()) saw_xhomed = 0;
  if (!motion_z_homed()) saw_zhomed = 0;

  if (safety_estop()) g_state = ST_FAULT;
  else if (safety_aborting()) g_state = ST_ABORT;
  else if (g_state == ST_ABORT && !safety_aborting() && !motion_busy()) {
    safety_clear_abort();
    g_state = motion_x_homed() ? ST_READY : ST_IDLE;
  } else if (g_state == ST_SPINUP && motion_at_speed()) {
    g_state = ST_READY;
    evt(F("AT_SPEED"));
  }
}

void protocol_tel() {
  uint32_t now = millis();
  if (now - last_tel < TEL_MS) return;
  last_tel = now;
  Serial.print(F("TEL rpm="));
  Serial.print(motion_rpm(), 3);
  Serial.print(F(" x="));
  Serial.print(motion_x_mm(), 2);
  Serial.print(F(" z="));
  Serial.print(motion_z_mm(), 3);
  Serial.print(F(" t="));
  Serial.print(heater_c(), 1);
  Serial.print(F(" vac="));
  Serial.print(vac);
  Serial.print(F(" estop="));
  Serial.print(safety_estop() ? 1 : 0);
  Serial.print(F(" state="));
  Serial.print(state_name(g_state));
  Serial.print(F(" xhomed="));
  Serial.print(motion_x_homed() ? 1 : 0);
  Serial.print(F(" zhomed="));
  Serial.println(motion_z_homed() ? 1 : 0);
}

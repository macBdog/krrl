#include "controls.h"

enum { B_33, B_45, B_78, B_START, B_STOP, B_PUP, B_PDN, N_BTN };

static const uint8_t PIN[N_BTN] = {
  PIN_BTN_33, PIN_BTN_45, PIN_BTN_78,
  PIN_BTN_START, PIN_BTN_STOP,
  PIN_BTN_PITCH_UP, PIN_BTN_PITCH_DN,
};

static uint8_t stable[N_BTN];      /* 1 = pressed (debounced) */
static uint8_t raw_last[N_BTN];
static uint32_t changed_ms[N_BTN];
static uint8_t edge[N_BTN];        /* set on falling edge, cleared on read */

void controls_begin() {
  for (uint8_t i = 0; i < N_BTN; i++) {
    pinMode(PIN[i], INPUT_PULLUP);
    stable[i] = 0;
    raw_last[i] = 1;               /* pulled up = released */
    changed_ms[i] = millis();
    edge[i] = 0;
  }
}

void controls_poll() {
  uint32_t now = millis();
  for (uint8_t i = 0; i < N_BTN; i++) {
    uint8_t raw = digitalRead(PIN[i]) == LOW ? 1 : 0;
    if (raw != raw_last[i]) {
      raw_last[i] = raw;
      changed_ms[i] = now;
    } else if (now - changed_ms[i] >= DEBOUNCE_MS && raw != stable[i]) {
      stable[i] = raw;
      if (raw) edge[i] = 1;        /* just became pressed */
    }
  }
}

static bool take_edge(uint8_t i) {
  if (edge[i]) { edge[i] = 0; return true; }
  return false;
}

bool press_33() { return take_edge(B_33); }
bool press_45() { return take_edge(B_45); }
bool press_78() { return take_edge(B_78); }
bool press_start() { return take_edge(B_START); }
bool press_stop() { return take_edge(B_STOP); }
bool hold_pitch_up() { return stable[B_PUP]; }
bool hold_pitch_dn() { return stable[B_PDN]; }

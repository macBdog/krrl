#include "controls.h"

/* Debounced mode button. */
static uint8_t stable;       /* 1 = pressed (debounced) */
static uint8_t raw_last;
static uint32_t changed_ms;
static uint8_t edge;         /* set on the press edge, cleared on read */

static float pot_ema;        /* smoothed pot ADC reading */

void controls_begin() {
  pinMode(PIN_BTN_MODE, INPUT_PULLUP);
  stable = 0;
  raw_last = 1;              /* pulled up = released */
  changed_ms = millis();
  edge = 0;
  pinMode(PIN_PITCH_POT, INPUT);
  pot_ema = (float)analogRead(PIN_PITCH_POT);
}

void controls_poll() {
  uint32_t now = millis();
  uint8_t raw = digitalRead(PIN_BTN_MODE) == LOW ? 1 : 0;
  if (raw != raw_last) {
    raw_last = raw;
    changed_ms = now;
  } else if (now - changed_ms >= DEBOUNCE_MS && raw != stable) {
    stable = raw;
    if (raw) edge = 1;        /* just became pressed */
  }
}

bool press_mode() {
  if (edge) { edge = 0; return true; }
  return false;
}

float controls_pitch_pct() {
  int raw = analogRead(PIN_PITCH_POT);
  pot_ema += ((float)raw - pot_ema) * POT_EMA_ALPHA;
  return krrl_pot_to_pitch_pct((int)(pot_ema + 0.5f), POT_MAX_COUNTS, POT_DEADBAND);
}

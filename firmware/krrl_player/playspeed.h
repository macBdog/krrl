#pragma once

/* Belt-drive platter speed math, shared with the lathe.
 *
 * The player turntable reuses the KRRL-01 belt drive, TMC2209 driver and the
 * open-loop step-rate motion core. The platter gearing is identical, so
 * KRRL_PLATTER_STEPS_PER_REV mirrors PLATTER_STEPS_PER_REV in the Mega
 * firmware (firmware/krrl_mega/config.h) and platter.steps_per_rev in
 * config/machine.yaml. Keep the three in sync if the drive ratio changes.
 *
 * Pure C, no Arduino dependency, so the same functions compile into the
 * sketch and into the native host test (firmware/krrl_player/test).
 */

#include <stdint.h>

#define KRRL_PLATTER_STEPS_PER_REV 3200.0f

/* Nominal record speeds (RPM). */
#define KRRL_RPM_33 33.333f
#define KRRL_RPM_45 45.0f
#define KRRL_RPM_78 78.0f

/* Pitch fader travel in percent, classic turntable +/-8%. */
#define KRRL_PITCH_MAX_PCT 8.0f

/* Clamp a pitch trim (percent) to the fader travel. */
static inline float krrl_clamp_pitch_pct(float pitch_pct) {
  if (pitch_pct > KRRL_PITCH_MAX_PCT) return KRRL_PITCH_MAX_PCT;
  if (pitch_pct < -KRRL_PITCH_MAX_PCT) return -KRRL_PITCH_MAX_PCT;
  return pitch_pct;
}

/* Apply a pitch trim (percent, +/-) to a nominal speed. */
static inline float krrl_pitched_rpm(float base_rpm, float pitch_pct) {
  return base_rpm * (1.0f + krrl_clamp_pitch_pct(pitch_pct) / 100.0f);
}

/* Fine open-loop speed calibration, parts-per-million. Set after measuring the
 * true platter speed against a calibrated reference (see docs/CALIBRATION.md).
 * 0 = use steps_per_rev as-is. */
#define KRRL_SPEED_TRIM_PPM 0

/* Platter step rate (steps/s) for a platter RPM. Open-loop feedforward:
 * sps = rpm / 60 * steps_per_rev, with the ppm calibration trim applied. */
static inline int32_t krrl_rpm_to_sps(float rpm) {
  if (rpm < 0.0f) rpm = 0.0f;
  rpm *= (1.0f + (float)KRRL_SPEED_TRIM_PPM / 1000000.0f);
  return (int32_t)(rpm / 60.0f * KRRL_PLATTER_STEPS_PER_REV + 0.5f);
}

/* Map a pitch-fader ADC reading to a trim percent. The fader centre detent is
 * 0%; either end is +/-KRRL_PITCH_MAX_PCT. A small deadband around centre keeps
 * the platter at exactly nominal speed when the fader rests in the middle, and
 * the travel each side of the deadband is rescaled so the ends still hit the
 * full +/-8%. Higher reading = pitch up. */
static inline float krrl_pot_to_pitch_pct(int raw, int raw_max, int deadband) {
  int center = raw_max / 2;
  int d = raw - center;
  if (d > -deadband && d < deadband) return 0.0f;
  int span;
  if (d > 0) { d -= deadband; span = (raw_max - center) - deadband; }
  else       { d += deadband; span = center - deadband; }
  if (span < 1) span = 1;
  return krrl_clamp_pitch_pct((float)d / (float)span * KRRL_PITCH_MAX_PCT);
}

/* Step the current rate toward a target by at most max_step, so the belt
 * accelerates and decelerates smoothly instead of stalling. */
static inline int32_t krrl_slew_toward(int32_t cur, int32_t target, int32_t max_step) {
  if (max_step < 1) max_step = 1;
  int32_t err = target - cur;
  if (err > max_step) err = max_step;
  if (err < -max_step) err = -max_step;
  return cur + err;
}

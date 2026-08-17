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

/* Platter step rate (steps/s) for a platter RPM. Matches the lathe's
 * open-loop rate: sps = rpm / 60 * steps_per_rev. */
static inline int32_t krrl_rpm_to_sps(float rpm) {
  if (rpm < 0.0f) rpm = 0.0f;
  return (int32_t)(rpm / 60.0f * KRRL_PLATTER_STEPS_PER_REV + 0.5f);
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

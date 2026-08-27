/* Native host test for the shared belt-drive speed math.
 * Build: c++ -std=c++11 -o test_playspeed test_playspeed.cpp && ./test_playspeed
 * No Arduino toolchain required; playspeed.h is pure C. */

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "../playspeed.h"

static int fails = 0;

static void eq_i(const char *what, int32_t got, int32_t want) {
  if (got != want) {
    printf("FAIL %s: got %ld want %ld\n", what, (long)got, (long)want);
    fails++;
  } else {
    printf("ok   %s = %ld\n", what, (long)got);
  }
}

static void eq_f(const char *what, float got, float want) {
  if (fabsf(got - want) > 1e-3f) {
    printf("FAIL %s: got %.4f want %.4f\n", what, got, want);
    fails++;
  } else {
    printf("ok   %s = %.4f\n", what, got);
  }
}

int main() {
  /* Nominal speeds -> platter step rate (sps = rpm/60 * 3200). */
  eq_i("sps@33", krrl_rpm_to_sps(KRRL_RPM_33), 1778);
  eq_i("sps@45", krrl_rpm_to_sps(KRRL_RPM_45), 2400);
  eq_i("sps@78", krrl_rpm_to_sps(KRRL_RPM_78), 4160);

  /* Pitch trim is multiplicative and clamped to +/-8%. */
  eq_f("pitch +8 @33", krrl_pitched_rpm(KRRL_RPM_33, 8.0f), 35.99964f);
  eq_f("pitch -8 @45", krrl_pitched_rpm(KRRL_RPM_45, -8.0f), 41.4f);
  eq_f("pitch clamp hi", krrl_pitched_rpm(KRRL_RPM_45, 20.0f), 48.6f);
  eq_f("pitch clamp lo", krrl_pitched_rpm(KRRL_RPM_45, -20.0f), 41.4f);
  eq_f("pitch zero", krrl_pitched_rpm(KRRL_RPM_78, 0.0f), 78.0f);

  /* Pitched speeds -> step rate. */
  eq_i("sps@33 +8", krrl_rpm_to_sps(krrl_pitched_rpm(KRRL_RPM_33, 8.0f)), 1920);
  eq_i("sps@45 -8", krrl_rpm_to_sps(krrl_pitched_rpm(KRRL_RPM_45, -8.0f)), 2208);
  eq_i("sps@78 +8", krrl_rpm_to_sps(krrl_pitched_rpm(KRRL_RPM_78, 8.0f)), 4493);

  /* Stopped / negative rpm clamps to zero. */
  eq_i("sps@0", krrl_rpm_to_sps(0.0f), 0);
  eq_i("sps@neg", krrl_rpm_to_sps(-5.0f), 0);

  /* Pitch fader ADC (10-bit, centre 511, deadband 10) -> trim percent. */
  eq_f("pot centre", krrl_pot_to_pitch_pct(511, 1023, 10), 0.0f);
  eq_f("pot in deadband", krrl_pot_to_pitch_pct(517, 1023, 10), 0.0f);
  eq_f("pot full up", krrl_pot_to_pitch_pct(1023, 1023, 10), 8.0f);
  eq_f("pot full down", krrl_pot_to_pitch_pct(0, 1023, 10), -8.0f);
  eq_f("pot just past deadband", krrl_pot_to_pitch_pct(521, 1023, 10), 0.0f);
  eq_f("pot ~3/4 up", krrl_pot_to_pitch_pct(767, 1023, 10), 3.9203f);
  eq_f("pot clamp over-range", krrl_pot_to_pitch_pct(2000, 1023, 10), 8.0f);

  /* Passive optical tach: mark-to-mark period -> rpm (one mark/rev). Readout
   * only; it does not steer the step rate. */
  eq_f("tach 45rpm 1ppr", krrl_tach_rpm(1333333, 1), 45.0f);
  eq_f("tach 78rpm 1ppr", krrl_tach_rpm(769231, 1), 78.0f);
  eq_f("tach 45rpm 2ppr", krrl_tach_rpm(666666, 2), 45.0f);
  eq_f("tach bad period", krrl_tach_rpm(0, 1), 0.0f);
  eq_f("tach bad ppr", krrl_tach_rpm(1000, 0), 0.0f);

  if (fails) { printf("\n%d check(s) FAILED\n", fails); return 1; }
  printf("\nplayspeed: ok\n");
  return 0;
}

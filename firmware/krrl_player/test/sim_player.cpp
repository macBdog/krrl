/* Native simulation of the player control/motion timeline.
 * Composes the shared speed math (krrl_rpm_to_sps / krrl_pitched_rpm), the
 * shared belt slew (krrl_slew_toward) and the shared optical-tach loop
 * (krrl_tach_rpm / krrl_tach_trim_sps) exactly as krrl_player.ino +
 * platter.cpp do, so spin-up/down, pitch and speed-lock can be checked
 * without hardware.
 *
 * Build: g++ -std=c++11 -o sim_player sim_player.cpp && ./sim_player */

#include <cstdio>
#include <cmath>
#include "../playspeed.h"

#define RPM_BAND 0.3f

/* Mirrors config.h SLEW_SPS_PER_MS. */
static const float SLEW_SPS_PER_MS = 6.0f;

static int32_t rate = 0; /* live platter rate (steps/s) */

static float sps_to_rpm(int32_t sps) {
  return (float)sps * 60.0f / KRRL_PLATTER_STEPS_PER_REV;
}

/* Advance the belt for `ms` toward a commanded rate, one 1 ms poll at a time. */
static void run(const char *label, int32_t target_sps, int ms) {
  for (int i = 0; i < ms; i++) {
    int32_t max_step = (int32_t)(SLEW_SPS_PER_MS * 1.0f);
    rate = krrl_slew_toward(rate, target_sps, max_step);
  }
  printf("%-22s target=%5ld sps  rate=%5ld sps  (%.3f rpm)\n",
         label, (long)target_sps, (long)rate, sps_to_rpm(rate));
}

int main() {
  float base = KRRL_RPM_33;
  float pitch = 0.0f;
  bool running = false;
  int fails = 0;

  printf("t0  power-on, stopped\n");
  run("idle", running ? krrl_rpm_to_sps(krrl_pitched_rpm(base, pitch)) : 0, 100);
  if (rate != 0) { printf("FAIL idle not 0\n"); fails++; }

  printf("\nSTART @ 33\n");
  running = true;
  run("spinup 200ms", krrl_rpm_to_sps(krrl_pitched_rpm(base, pitch)), 200);
  run("spinup settle", krrl_rpm_to_sps(krrl_pitched_rpm(base, pitch)), 400);
  if (rate != 1778) { printf("FAIL not at 33 (1778)\n"); fails++; }

  printf("\nSELECT 78 (running)\n");
  base = KRRL_RPM_78;
  run("ramp to 78", krrl_rpm_to_sps(krrl_pitched_rpm(base, pitch)), 200);
  run("settle 78", krrl_rpm_to_sps(krrl_pitched_rpm(base, pitch)), 500);
  if (rate != 4160) { printf("FAIL not at 78 (4160)\n"); fails++; }

  printf("\nPITCH fader to full up (+8%%)\n");
  pitch = krrl_pot_to_pitch_pct(1023, 1023, 10); /* fader hard over = +8% */
  run("ramp to 78+8", krrl_rpm_to_sps(krrl_pitched_rpm(base, pitch)), 200);
  if (rate != 4493) { printf("FAIL not at 78+8 (4493)\n"); fails++; }

  printf("\nSTOP\n");
  running = false;
  run("spindown 300ms", running ? krrl_rpm_to_sps(krrl_pitched_rpm(base, pitch)) : 0, 300);
  run("spindown settle", 0, 600);
  if (rate != 0) { printf("FAIL not stopped\n"); fails++; }

  /* Closed-loop tach: model a belt that slips ~1%, so the platter runs slow
   * unless the loop trims it up. Feed the measured speed back through the same
   * period->rpm and proportional-trim helpers the firmware uses. */
  printf("\nCLOSED-LOOP TACH @ 45 (belt slip 1%%)\n");
  const float slip = 0.01f;
  float ff = KRRL_RPM_45;
  int32_t ff_sps = krrl_rpm_to_sps(ff);
  int32_t cmd = ff_sps;
  int32_t lim = ff_sps / 8;
  float measured = 0.0f;
  bool locked = false;
  for (int rev = 1; rev <= 8; rev++) {
    /* Actual platter speed for this commanded rate, reduced by slip. */
    float actual_rpm = (float)cmd * 60.0f / KRRL_PLATTER_STEPS_PER_REV * (1.0f - slip);
    /* One index mark per rev: period the sensor would report, then back out. */
    uint32_t period_us = (uint32_t)(60000000.0f / actual_rpm + 0.5f);
    measured = krrl_tach_rpm(period_us, KRRL_TACH_PPR);
    int32_t trim = krrl_tach_trim_sps(ff, measured, KRRL_TACH_KP_SPS_PER_RPM);
    if (trim > lim) trim = lim;
    if (trim < -lim) trim = -lim;
    cmd = ff_sps + trim;
    locked = fabsf(measured - ff) <= RPM_BAND;
    printf("rev %d  cmd=%4ld sps  measured=%.3f rpm  err=%+.3f  %s\n",
           rev, (long)cmd, measured, measured - ff, locked ? "LOCKED" : "seek");
  }
  if (!locked) { printf("FAIL tach did not lock\n"); fails++; }

  if (fails) { printf("\n%d check(s) FAILED\n", fails); return 1; }
  printf("\nsim_player: ok\n");
  return 0;
}

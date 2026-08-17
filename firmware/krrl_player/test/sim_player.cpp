/* Native simulation of the player control/motion timeline.
 * Composes the shared speed math (krrl_rpm_to_sps / krrl_pitched_rpm) with the
 * shared belt slew (krrl_slew_toward) exactly as krrl_player.ino + platter.cpp
 * do, so the spin-up/down and pitch behaviour can be checked without hardware.
 *
 * Build: g++ -std=c++11 -o sim_player sim_player.cpp && ./sim_player */

#include <cstdio>
#include <cmath>
#include "../playspeed.h"

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

  printf("\nPITCH +8%%\n");
  pitch = 8.0f;
  run("ramp to 78+8", krrl_rpm_to_sps(krrl_pitched_rpm(base, pitch)), 200);
  if (rate != 4493) { printf("FAIL not at 78+8 (4493)\n"); fails++; }

  printf("\nSTOP\n");
  running = false;
  run("spindown 300ms", running ? krrl_rpm_to_sps(krrl_pitched_rpm(base, pitch)) : 0, 300);
  run("spindown settle", 0, 600);
  if (rate != 0) { printf("FAIL not stopped\n"); fails++; }

  if (fails) { printf("\n%d check(s) FAILED\n", fails); return 1; }
  printf("\nsim_player: ok\n");
  return 0;
}

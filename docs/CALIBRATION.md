# Platter speed calibration

Both KRRL-01 machines run the platter **open-loop**: a stepper turns the belt at
a rate set purely by the step timing, and torque margin (not feedback) keeps it
on speed. There is no runtime speed control loop. That makes speed accuracy a
one-time **calibration** job, plus a check that the mechanicals are good enough.

This applies to both the lathe (Mega) and the player (Nano) — they share the
platter and drive.

## What sets the speed (and what can be wrong)

Platter speed = `step_rate x (1 / steps_per_rev)`, and the step rate comes
straight from the timer (`OCR1A = F_CPU / 8 / ISR_HZ`). So total speed error is
the product of two constant scale factors, plus one thing calibration cannot
fix:

1. **Controller timebase — the dominant error.** Many Nano/Mega clones clock
   from a **ceramic resonator (±0.5%, and it drifts with temperature)**. That is
   ~0.17 rpm at 33⅓ on its own. A real **crystal is ±20–50 ppm**; a **TCXO is
   ±2 ppm**. Open-loop high tolerance *requires* a stable, accurate step clock:
   use a genuine crystal-clocked board, or drive the step source from a TCXO.
2. **Mechanical scale** — `steps_per_rev` = microsteps × motor steps/rev ×
   pulley ratio. A constant factor you null during calibration.
3. **Wow & flutter** — intra-revolution variation (belt, bearing, pulley
   eccentricity, microstep non-uniformity). Open-loop cannot remove it, so
   calibration must **measure** it to confirm the build is good enough.

> **Cardinal rule:** the calibration reference must be *independent of the
> board's clock* and about **10× better** than your target tolerance. You cannot
> calibrate the board's timebase with the board's own timebase.

## Sensor setups, best first

### 1. High-count optical encoder + disciplined timebase (recommended bench rig)

The most versatile: it yields both average speed (to ppm) and a wow/flutter
spectrum from one instrument.

- **Sensor:** a transmissive glass/etched encoder disc or a commercial rotary
  encoder, **≥2000 CPR**, coupled to the spindle. Mind coupling runout — it
  shows up as flutter. (Note the inversion from runtime: for *offline*
  calibration you want as many lines as possible; CPU cost is irrelevant.)
- **Timebase:** capture the A channel on a counter/DAQ referenced to a **TCXO or
  GPS-disciplined 10 MHz** (a lab reciprocal frequency counter with an external
  reference is ideal).
- **Compute:** counts over a long gate (e.g. 100 s) → average speed to ppm;
  edge-to-edge intervals → instantaneous velocity → wow/flutter.

### 2. 3150 Hz test record + wow/flutter analysis (gold standard for the player)

Per **DIN 45507 / IEC 60386**: play a standard 3150 Hz reference LP and measure
the recovered tone with a **W&F meter** or PC audio analysis. This measures
true, *as-heard* speed and W&F through the whole chain including the record. It
is the definitive turntable calibration — but it needs a cartridge/tonearm +
phono preamp + audio interface, i.e. a pickup the motion-only player does not
have by default. Use it if a pickup is fitted (or added for QA).

### 3. Handheld laser photo-tachometer + reflective tape (quick check)

~$20, reads **average RPM to ~±0.05%**, no W&F, and its own timebase is only
moderate. Good for a go/no-go, not for ppm work.

### 4. Calibrated LED strobe disc (quick check)

An LED strobe driven from an **accurate** source (not mains) reads ~0.1% by eye
and makes flutter visible as edge "shimmer". Cheap sanity check.

## Procedure

1. **Fix the timebase.** Confirm the board is crystal/TCXO-clocked (not a
   resonator). Log the actual clock frequency against your reference; a stable
   offset is fine because it folds into step 3.
2. **Measure true speed.** Command 33⅓, 45 and 78 and measure actual platter
   speed with an independent, well-clocked reference (setup 1 or 2). One speed is
   enough if the drive is a fixed ratio; measuring all three confirms it.
3. **Null the error.** The error is a scale factor `k = commanded / measured`.
   Apply it either way:
   - adjust `steps_per_rev` → `steps_per_rev x k`, or
   - set the fine ppm trim: `PLATTER_CAL_PPM` (lathe, `firmware/krrl_mega/config.h`)
     or `KRRL_SPEED_TRIM_PPM` (player, `firmware/krrl_player/playspeed.h`), where
     `ppm = (k - 1) x 1e6`.
   Reflash and re-measure to confirm.
4. **Check wow & flutter.** Capture the spectrum (setup 1 or 2). Good decks are
   roughly **< 0.1% WRMS**. If one line dominates (e.g. once-per-rev), fix it
   mechanically — open-loop won't.
5. **Verify over temperature.** Re-check cold and warm; resonators in particular
   move here. If it drifts, improve the timebase (step 1).

## Where the knobs live

| Machine | steps/rev | fine ppm trim |
|---------|-----------|---------------|
| Lathe (Mega) | `PLATTER_STEPS_PER_REV` in `firmware/krrl_mega/config.h` (and `platter.steps_per_rev` in `config/machine.yaml`) | `PLATTER_CAL_PPM` |
| Player (Nano) | `KRRL_PLATTER_STEPS_PER_REV` in `firmware/krrl_player/playspeed.h` | `KRRL_SPEED_TRIM_PPM` |

The optional once-per-rev optical index (see [TACHOMETER.md](TACHOMETER.md)) is a
passive at-speed monitor; because it samples only once per revolution it is not a
calibration reference. Use the independent, high-resolution setups above.

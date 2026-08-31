# KRRL-01 hardware notes

Engineering notes and bill of materials for the mechanical drive and the player's
phono electronics. These are design notes, not a build manual — dimensions are
targets to design around, and part numbers are representative families, not
mandated SKUs.

## Shared drive architecture

The lathe and the standalone player share one drive: a **raised platter on a
central spindle, belt-driven around the spindle**, with the belt **tensioner
around that central shaft**. Raising the platter leaves the space underneath for
the lathe's **X-axis lead screw and carriage**, which passes below the platter.
The player uses the identical drive minus the X/Z/cutter hardware.

```
                 14" (356 mm) platter
   ┌──────────────────────────────────────────────┐
   └───────────────────────┬────────────────────────┘
              sub-platter / belt pulley (on spindle)
        ╔═══════════════════╪═══════════════════╗
        ║  bearing cartridge (moment-stiff)     ║  <- rigidity lives here
        ╚═══════════════════╪═══════════════════╝
                          spindle
   ── belt ── ( motor pulley )      ( idler tensioner )
   ─────────────── X-axis lead screw runs UNDER platter ───────────────
                          plinth / chassis
```

Design consequences that thread through the rest of these notes:

- **Rigidity is a moment problem.** The cutting stylus and the record rim sit at
  ~178 mm radius, so what matters is the platter's resistance to *tilting*, set
  almost entirely by the spindle bearing arrangement (see `drive.md`).
- **The belt sets the layout, not a big reduction.** The step ISR runs at
  `ISR_HZ = 20 kHz` and a step needs two ticks, so the platter step rate must
  stay below ~10 kHz (keep < 8 kHz for margin). At the base `steps_per_rev` of
  3200 and 78 rpm that is 4160 steps/s, so the belt ratio must stay near **1:1
  (≤ ~2:1)**. The belt is there for the raised-platter geometry and vibration
  isolation, not gearing.
- **Speed is open-loop.** There is no runtime speed feedback; absolute accuracy
  comes from calibrating `steps_per_rev` (see
  [`docs/CALIBRATION.md`](../docs/CALIBRATION.md)). The player has no speed
  sensor; the lathe may optionally fit a once-per-rev optical index as a passive
  at-speed monitor ([`docs/TACHOMETER.md`](../docs/TACHOMETER.md)).

## Contents

| File | Covers |
|------|--------|
| [`drive.md`](drive.md) | Platter bearings, belt spec, stepper + driver, belt tensioner |
| [`phono.md`](phono.md) | Minimal-component phono amplification + RIAA EQ (player) |
| [`BOM.csv`](BOM.csv) | Consolidated bill of materials (lathe / player / both) |
| [`player_pcb/`](player_pcb) | Player single-board KiCad design (Nano + TMC2209 + RIAA + I/O), fab outputs |

## Related

- Firmware: [`firmware/krrl_mega/`](../firmware/krrl_mega) (lathe),
  [`firmware/krrl_player/`](../firmware/krrl_player) (player).
- Speed calibration: [`docs/CALIBRATION.md`](../docs/CALIBRATION.md).
- Optical index marking/machining: [`docs/TACHOMETER.md`](../docs/TACHOMETER.md).

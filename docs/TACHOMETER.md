# Closed-loop optical tachometer

Both KRRL-01 machines — the lathe (Mega) and the player (Nano) — drive the same
belt-driven platter with a stepper. A stepper is nearly synchronous, so the
commanded step rate is an accurate **feedforward** speed. The optical tachometer
closes the loop on top of that: it measures the platter's *actual* revolutions
and trims the step rate to cancel belt stretch, slip and load, so 33⅓/45/78 stay
locked.

This document applies to **both** builds. They share one platter, so they share
one marking scheme.

## How it works

1. **One index mark per revolution** on the platter passes an optical sensor
   mounted on the chassis. The sensor outputs one clean pulse per rev.
2. The pulse edge fires an interrupt (`platter_tach_isr` on the player,
   `motion_tach_isr` on the lathe). The handler timestamps the edge with
   `micros()` and records the **mark-to-mark period**. Edges closer than 2 ms
   (noise) or slower than 2 s are rejected.
3. Measured speed is `rpm = 60 000 000 / (period_us × marks_per_rev)`. With one
   mark per rev (`marks_per_rev = 1`) the period in microseconds is the whole
   revolution. This is `krrl_tach_rpm()` in
   [`firmware/krrl_player/playspeed.h`](../firmware/krrl_player/playspeed.h); the
   lathe computes the same thing inline.
4. Each poll the firmware sets the platter rate to
   `feedforward + Kp × (target_rpm − measured_rpm)`, where
   `feedforward = target_rpm / 60 × steps_per_rev` and `Kp = 40` steps/s per rpm
   of error (`krrl_tach_trim_sps()`). The trim is clamped so it fine-tunes speed
   without overpowering the spin-up ramp.
5. When `|measured − target| ≤ RPM_BAND` (0.3 rpm) the platter is **locked**.
   On the player the onboard LED goes solid at lock (blinking while seeking); on
   the lathe the interlock/telemetry (`state`, `rpm`) reports it and `START`
   requires the platter to be at speed.

**Open-loop fallback:** if no sensor is fitted, no pulse ever arrives and the
firmware runs purely on feedforward (the player reports the open-loop estimate
and still locks once spun up). Fitting the sensor later needs no firmware
change.

Marks per rev is fixed at **1** in firmware (`KRRL_TACH_PPR`, and `tach_ppr` in
[`config/machine.yaml`](../config/machine.yaml)). Use exactly one index feature.

## Sensor and wiring

Use a 5 V digital optical sensor with a clean (ideally Schmitt-triggered) output:

- **Through-beam / slotted photo-interrupter** (e.g. a slot-type opto): the
  platter index feature (a slot or hole in a depending rim) passes through the
  slot. Most robust — immune to surface finish and ambient light. **Preferred.**
- **Reflective sensor** (e.g. TCRT5000-class IR reflectance): reads a single
  high/low-contrast mark on a platter face. Simpler to mount; needs a good
  contrast mark and a fixed standoff.

| Signal | Lathe (Mega) | Player (Nano) |
|--------|--------------|---------------|
| Tach input | pin `3` (`PIN_TACH`) | pin `2` (`PIN_TACH`, INT0) |
| Sensor Vcc / GND | 5 V / GND | 5 V / GND |

The input uses `INPUT_PULLUP` and triggers on the **rising** edge. Keep the lead
short or shielded; add a 100 nF cap at the sensor if the output is noisy. Aim
for one crisp pulse per rev with fast edges — mount the sensor so the mark fully
clears it between revolutions.

## How to mark the platter

Pick the option that matches your sensor. Either way, the rule is **one index
feature per revolution**, at a fixed radius, cleared by the sensor each turn.

### Option A — index slot/hole in the rim (through-beam / slotted sensor)

Best for a platter with a **depending skirt** (a vertical cylindrical wall on the
underside). Drill **one** radial hole (or mill one slot) through the skirt so it
lines up with the slot sensor.

- One hole/slot only.
- Width ≥ 1.5× the sensor's effective beam so it triggers reliably; a
  3–4 mm hole suits most slot optos.
- Put it on a dedicated band clear of screws, spokes and other holes so nothing
  else breaks the beam.

### Option B — single index mark on a face (reflective sensor)

Best for a flat platter with no skirt. Create **one** high-contrast mark on the
underside at a set radius, on an otherwise uniform annular **tach track**:

- Bright bare-aluminium track with **one matte-black mark** (paint/anodise), or
  a matte/black track with **one reflective mark** (foil tape).
- Mark ~6–8 mm long (arc) × the track width; only one, at one radius.
- Hold the sensor standoff the sensor datasheet calls for (TCRT5000 ≈ 2–5 mm).

## Machining the tach feature on a manual lathe

You are turning the platter anyway; add the tach feature in the same setup so it
is concentric with the spindle. Below assumes an aluminium platter.

### Tooling

- Manual lathe with a 3- or 4-jaw chuck (or faceplate) able to hold the platter.
- Facing and turning tools to true the underside and, for Option B, to turn a
  clean annular **tach track**.
- A **parting/grooving tool** if you prefer a turned groove as the reflective
  track boundary.
- For Option A: a **slotting/index hole** — a centre/spotting drill plus a
  3–4 mm drill, run in the tailstock (for an axial hole) or, for a radial hole
  in a skirt, a drill in the tailstock with the platter clamped on its side in a
  fixture, or a milling attachment / drill press. A small end mill if you want a
  slot instead of a hole.
- Layout tools: height gauge or surface gauge + scriber, dividers, dial
  indicator to confirm runout, deburring tool/fine file, and a fine-tip paint
  pen or foil tape for Option B.
- Eye protection; deburr and clean all swarf (it is ferromagnetic-free but still
  fouls optics and bearings).

### Steps

1. **Mount and true.** Hold the platter and indicate it in so radial runout is a
   few hundredths of a millimetre. Face the underside flat.
2. **Turn the tach track (Option B) or verify the skirt (Option A).** For a
   reflective mark, turn a shallow, uniform annular band ~8–10 mm wide at a
   convenient radius (e.g. near the rim, clear of the bearing and drive belt).
   For a slotted sensor, confirm the skirt runs true and pick a radius/height
   where the slot opto can straddle it.
3. **Set the sensor radius.** Decide the radius the sensor will sit at and record
   it — the mark/hole must be at that radius. Keep it clear of the belt path,
   mounting bosses and balance features.
4. **Cut the single index feature.**
   - *Option A:* spot then drill **one** 3–4 mm hole through the skirt (or mill
     one slot). Do not add a second hole in the sensed band.
   - *Option B:* leave the machined band as the uniform background; the actual
     mark is applied after finishing (step 6).
5. **Deburr and check balance.** Remove burrs so edges are crisp (crisp edges =
   clean pulse). A single small hole adds negligible imbalance at 33–78 rpm; if
   you want to balance it, drill a **blind** hole of equal mass **180° opposite**
   on a *different, non-sensed* band so it never enters the beam.
6. **Apply the contrast mark (Option B only).** After final finishing, add
   exactly **one** matte-black mark (or one reflective foil mark on a dark track)
   ~6–8 mm long at the sensor radius. Keep the rest of the track uniform.
7. **Fit and verify.** Mount the sensor at the recorded radius and standoff.
   Spin the platter by hand and confirm the sensor toggles **once per
   revolution** with clean high/low levels. Then power up: the player LED should
   go solid (locked) after spin-up, and the lathe should report `rpm` matching
   the selected speed and reach `READY`/at-speed.

## Verifying without hardware

The tach math and the correction loop are covered by the shared, host-buildable
tests:

```
cd firmware/krrl_player/test
g++ -std=c++11 -o test_playspeed test_playspeed.cpp && ./test_playspeed  # rpm/trim math
g++ -std=c++11 -o sim_player sim_player.cpp && ./sim_player               # loop locks a slipping belt
```

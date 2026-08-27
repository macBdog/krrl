# Drive: bearings, belt, stepper, tensioner

Shared by the lathe and the player. See [`README.md`](README.md) for the
raised-platter, belt-around-the-spindle architecture and the constraints it
imposes.

## 1. Platter spindle bearings (rigidity at the 14" rim)

The platter is 356 mm (14") diameter, so the load that matters is the **tilting
moment** at the ~178 mm rim — from the cutting stylus on the lathe and from any
off-centre load on either machine. A bearing that is stiff radially but weak in
*moment* lets the rim nod by microns, which reads as runout, wow and (on the
lathe) a wandering cut. Maximise **moment (tilt) stiffness**, then radial, then
axial; keep friction and noise low for the audio path.

Target: platter face and rim runout **< 5 µm TIR**; no perceptible rock at the
rim by hand.

### Recommended: preloaded angular-contact pair, back-to-back (baseline)

Two precision angular-contact ball bearings (7205/7206-class, **bore 25–30 mm**,
**P5/ABEC-5 or better**) mounted **back-to-back (DB / "O")** in a rigid cartridge.
Back-to-back places the contact lines' apexes *outboard*, which widens the
effective support span and is what buys the tilt stiffness at the rim. Set a
**light-to-medium axial preload** (ground spacer or a preload nut + Belleville
washer) to remove all play — preload is what makes it feel solid.

- Spindle: hardened, ground steel, **Ø25–30 mm**, seats to h6; housing bores H7.
- Space the two bearings as far apart on the spindle as the raised height allows;
  span dominates tilt stiffness.
- Angular-contact (point/line contact, low drag) runs quieter than tapered
  roller — preferred for the player.

### Alternative: thin-section crossed-roller bearing (most compact/stiff)

A single **crossed-roller** (e.g. THK RA/RB, IKO CRB, Kaydon) or four-point-contact
thin-section bearing takes radial, axial **and moment** load in one race. Highest
tilt stiffness per height and ideal for a raised platter with a large central
boss, at higher cost and more sensitivity to housing flatness. Size the bore to
the sub-platter boss (typically Ø50–90 mm).

### Alternative: preloaded tapered-roller pair (max stiffness, more rumble)

Back-to-back tapered rollers (machine-tool spindle style) give the highest
stiffness and load capacity — attractive for heavy cutting — but have more drag
and running noise, so they are a poorer fit for the player. Use only if lathe
cutting loads demand it.

### Avoid

A single deep-groove ball bearing + a flat thrust washer is cheap but has almost
no moment stiffness — the 14" rim will nod. Not acceptable for this goal.

## 2. Drive belt

The belt exists for **layout and vibration isolation**, not reduction (ratio must
stay near 1:1 — see the step-rate ceiling in `README.md`).

- **Type:** endless **flat ground belt** (polyurethane/neoprene, ~**6 mm wide ×
  0.5–0.65 mm**) or a **round belt Ø3–4 mm**. Both mechanically low-pass motor
  vibration, keeping rumble/flutter down — the classic turntable choice.
  A **GT2/6 mm timing belt** is the positive-drive alternative (exact ratio, no
  creep) but couples motor cogging into the platter; only choose it if belt creep
  under lathe cutting load is a problem, and rely on TMC stealthChop to keep the
  motor smooth.
- **Pulleys:** **crowned** (slightly barrelled) so a flat/round belt self-tracks
  — no flanges, cleaner look. The sub-platter/spindle is the driven pulley; the
  motor pulley diameter is chosen to keep ratio ≈ 1:1.
- **Wrap:** aim for **≥ 180°** contact on the drive pulley; the idler tensioner
  (below) is what increases wrap and holds it constant.
- **Tension:** low and constant — of order **2–5 N** for a round/flat isolation
  belt. Enough to avoid slip under acceleration/cut load, no more (over-tension
  loads the spindle bearing and adds rumble).

## 3. Stepper and driver

- **Motor:** **NEMA 17, high-torque** as the shared baseline — 42 mm frame,
  **≥ 0.5 N·m holding**, ~**1.5 A/phase**, 200 full-steps/rev (1.8°), e.g. a
  48 mm-stack `17HS19-2004S1`-class. Ample for the player and light cuts; the
  platter's large inertia is handled by the firmware's rate slew, and open-loop
  torque margin resists cut load without losing steps.
- **Heavy cutting option:** **NEMA 23** (57 mm, 1.2–1.9 N·m) if lathe cutting
  loads are high. Note a torquey NEMA 23 can exceed the TMC2209's practical
  ~1.7 A RMS — pair it with a **TMC5160** or an external step/dir driver
  (DM542-class) instead.
- **Driver:** **TMC2209** (already in firmware — STEP/DIR at runtime, UART config
  at boot; `firmware/*/tmc.cpp`). Run **stealthChop** for a silent platter on the
  player; **spreadCycle** is available when the lathe needs peak torque. Set RMS
  current to the motor (firmware default 800 mA for the platter). **16
  microsteps** balances smoothness against the step-rate ceiling.
- **Supply:** **24 V** to the driver (better torque headroom, still within the
  TMC2209's ~28 V limit); logic at 5 V. Keep the motor supply electrically clean
  and separated from the phono stage (see `phono.md`).

## 4. Belt tensioner (aesthetic, constant-tension)

A **constant-tension** device beats a fixed one: it takes up belt stretch and
thermal drift and keeps flutter low. Two clean options, both hiding the mechanism
under the raised platter:

### Preferred: pivoting idler arm with concealed preload

A short **polished/anodised aluminium arm** pivots on a bearing post and carries a
small **ball-bearing crowned idler pulley** that rides the slack side of the belt.
Preload the arm with either:

- a **concealed extension spring** tucked inside the arm or behind the pillar, or
- a **magnetic** preload — a pair of small neodymium magnets (one on the arm, one
  on the chassis) tuned to the target belt force — giving a spring-free, wear-free,
  visually clean pull.

The arm both **tensions** and **increases wrap** on the drive pulley. Set the
preload to land the 2–5 N belt tension; a soft rate keeps tension near-constant
over the arm's travel.

### Minimalist: eccentric motor pod

Mount the motor on a **round pod with an eccentric bore** (or on a slotted plate);
rotate/slide to tension, then lock with a single hidden screw. No visible
spring — the tension "mechanism" reads as a clean turned boss. Fixed rather than
constant-tension, so revisit after break-in.

Either way use a **bearing idler** (not a bushing) so it adds no drag or noise,
and crown it so the belt self-centres.

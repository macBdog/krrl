# Phono amplification and RIAA EQ (player)

The standalone player needs to turn a cartridge's tiny signal into a line-level
output with the **RIAA** replay curve applied. This note targets a **moving-magnet
(MM)** cartridge and the **fewest components** that do it well.

> The hard part here is not the circuit — it is **noise**. An MM output is only
> ~5 mV, sitting centimetres from a stepper and its driver. Read the EMI section
> before the topology; it decides whether this works.

## Cartridge and loading

- **Cartridge:** moving-magnet (MM). Output ~**3–5 mV @ 1 kHz, 5 cm/s**.
- **Load:** **47 kΩ** to ground in parallel with **~100–220 pF** (include tonearm
  cable capacitance in the total; many MM carts want ~200–300 pF total). Put the
  47 kΩ and a small load cap right at the preamp input.

## Target response

Apply the inverse-RIAA replay curve, i.e. three time constants:

| Constant | Freq | Role |
|----------|------|------|
| 3180 µs | 50.0 Hz | bass turnover (boost below) |
| 318 µs | 500 Hz | mid |
| 75 µs | 2122 Hz | treble roll-off |

Aim for the curve within **±0.5 dB** (1% metal-film resistors, 1–2.5% film caps).
Midband gain ~**40 dB (×100) @ 1 kHz** brings 5 mV up to ~0.5 V line level.

## Minimal topology: one dual op-amp, active-feedback RIAA

Fewest parts: a single **dual op-amp** does both channels, each in a
**non-inverting, active-feedback RIAA** stage. Per channel that is one gain
device plus a handful of passives — no separate buffers or multi-stage EQ.

- **Op-amp:** low-noise, unity-gain-stable **NE5532** (cheap, quiet), or
  **OPA2134 / LM4562** for lower noise/distortion. One dual package = stereo.
- **Per channel (see values below):** 47 kΩ load + load cap; gain resistor `Rg`
  (with a large DC-blocking cap to ground); feedback network `R_A, C_A` and
  `R_B, C_B` that set the three time constants; input DC-block + a small RF cap.

```
  cart o──┬──[47k]──┬── + in >\
          │        (Cload)     \____ out ──[Rout]──|| Cout ── line out
        (Cin DC)               /
                   ┌── − in ─/
                   │
          Rg ─ Cg(DC) ─ gnd        feedback: out ─ R_A ─┬─ − in
                                                        └─(R_B + C_B)┘,  C_A across R_A
```

### Starting values (verify with a RIAA calculator, then trim)

These land the three breakpoints close; C_A/R_B interact, so verify and trim on
the bench or in a calculator before committing:

- `Cload` ≈ 100 pF (bring total incl. cable to what the cartridge specifies)
- `Rg` ≈ 1 kΩ, `Cg` ≈ 100 µF (sets midband gain, blocks DC)
- `R_A` ≈ 100 kΩ, `C_A` ≈ 33 nF  (anchors the 3180 µs turnover)
- `R_B` ≈ 7.5 kΩ, `C_B` ≈ 10 nF  (`R_B·C_B` = 75 µs, the treble roll-off — exact)
- `Cin` ≈ 1–3.3 µF film (input DC block), plus ~100–470 pF at the input node for RF
- `Rout` ≈ 100 Ω, `Cout` ≈ 2.2–10 µF (output DC block)

### Easier-to-tune alternative

If you have a bench and want the curve nailed without interaction: a **flat
low-noise gain stage → passive RIAA network → buffer**. More parts, but each time
constant is an independent RC, so it is simpler to trim. Choose this only if the
single-stage feedback curve is fighting you.

A **dedicated phono IC** is the even-fewer-parts route if you prefer an
integrated part over the op-amp + passives, at the cost of flexibility.

## Power

- **± 12–15 V dual rail** from a small **linear** regulated supply is simplest and
  quietest. If running single-supply (e.g. 24 V), bias the op-amp to a
  **virtual ground** (rail/2) and AC-couple in and out.
- **Do not share the motor's 24 V rail.** Give the phono stage its own regulated,
  filtered supply; the stepper's switching current on a shared rail will inject
  hum/buzz straight into a 5 mV signal.
- Decouple each op-amp rail with 100 nF + 10 µF at the package.

## EMI, grounding and layout (the part that actually matters)

- **Separate the phono board from the motor and driver** physically and put it in
  a **shielded (steel/mu-metal) enclosure**. Keep step/dir and motor-phase wires
  well away from the cartridge leads; twist the motor pairs.
- **Shielded cartridge leads**, as short as practical; connect the shield at one
  end only (preamp end) to avoid a ground loop.
- **Star ground:** one reference point; separate the motor-supply return from the
  audio ground and join only at the star. No ground loops between the platter
  frame, motor supply, and audio.
- Run the platter motor in **stealthChop** (TMC2209) for the quietest electrical
  and acoustic signature during playback.
- Add the small input RF cap and keep the input node tiny; the 47 kΩ node is a
  perfect antenna otherwise.

## Output

Line level ~**0.3–1 V RMS** at an output impedance of ~100 Ω, AC-coupled. Feed a
line input / integrated amp, not a second phono input.

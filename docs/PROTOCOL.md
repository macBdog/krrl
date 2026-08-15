# KRRL-01 serial protocol

USB CDC, **115200 8N1**, newline-terminated ASCII (`\n`, `\r` ignored).  
Host is the Raspberry Pi. Device is the Arduino Mega 2560.

This file is the contract. Firmware and the host simulator implement the same grammar.

## Handshake

```
> HELLO KRRL/1
< HELLO MEGA KRRL/1
```

If the Mega is silent, the host retries HELLO every 500 ms.

## Host → Mega

| Command | Meaning |
|---------|---------|
| `HELLO KRRL/1` | Protocol hello |
| `PING` | Immediate `PONG` |
| `SET RPM <f>` | Platter target RPM (`0`, `33.333`, `45`, `78`) |
| `SET XVEL <f>` | X lead-screw velocity, mm/s (signed: + toward spindle) |
| `SET X <f>` | X target radius, mm |
| `SET Z <f>` | Z target depth, mm (0 = retracted) |
| `HEAT <f>` | Stylus target °C (`0` = off) |
| `VAC 0` / `VAC 1` | Chip vacuum |
| `HOME X` / `HOME Z` / `HOME ALL` | Home to outer / up limits |
| `JOG X <f>` | Relative X move, mm |
| `JOG Z <f>` | Relative Z move, mm |
| `START` | Arm cut feed (interlocks required) |
| `ABORT` | Firmware abort sequence |
| `ZERO X` | Set current X as reported position (after home) |

Unknown commands: `ERR UNKNOWN`.

## Mega → Host

Telemetry at 10–20 Hz:

```
TEL rpm=33.331 x=142.20 z=0.040 t=179.8 vac=1 estop=0 state=CUT xhomed=1 zhomed=1
```

| Field | Unit |
|-------|------|
| `rpm` | measured platter RPM |
| `x` | mm radius |
| `z` | mm depth |
| `t` | stylus °C |
| `vac` | 0/1 |
| `estop` | 1 = pressed |
| `state` | `IDLE` `HOMING` `READY` `SPINUP` `CUT` `ABORT` `FAULT` |
| `xhomed` `zhomed` | 0/1 |

Events:

```
EVT HOMED X
EVT HOMED Z
EVT AT_SPEED
EVT ABORTED
EVT DONE
```

Errors:

```
ERR ESTOP
ERR HEATER_OPEN
ERR NOT_HOMED
ERR NOT_AT_SPEED
ERR HEAT_BAND
ERR VAC_REQUIRED
ERR LIMIT
ERR UNKNOWN
```

`START` replies `OK` or `ERR …`. Other valid commands reply `OK` after they are accepted (homing completes later via `EVT`).

## Interlocks for START

All must hold:

1. `estop=0`
2. `xhomed=1` and `zhomed=1`
3. Platter RPM within ±0.3 of target (if target > 0)
4. Heater within ±5 °C of target (if target > 0)
5. Vacuum on if the host required it for this cut (host checks; Mega only checks `VAC` if `HEAT` > 0)

## Abort sequence (firmware)

Runs on `ABORT`, E-stop, or host-watchdog timeout (2 s with heater on):

1. Z retract to `z_retract_mm`
2. X velocity 0
3. Platter RPM ramp to 0
4. Heater off
5. Vacuum left as-is (host turns it off after chips settle)

## TMC2209

STEP/DIR at runtime. UART on `Serial1` (Mega pins 18/19) for current and microsteps at boot. Addresses via MS1/MS2: 0 platter, 1 X, 2 Z.

#!/usr/bin/env python3
"""Build the KRRL-01 Player single-board PCB (Nano + TMC2209 + RIAA + I/O).

Generates a placed, fully-netted KiCad 7 board (krrl_player.kicad_pcb) with a
board outline, mounting holes and GND pours on both layers. Routing is done
separately (route.py -> freerouting) and fabrication outputs by fab.py.

Run:  python3 build_pcb.py
This is the source of truth for the board's connections; edit and re-run.
"""

import pcbnew

FP = "/usr/share/kicad/footprints"
MM = pcbnew.FromMM


def vec(x, y):
    return pcbnew.VECTOR2I(MM(x), MM(y))


# ---------------------------------------------------------------------------
# Components: ref -> (footprint lib, footprint, value, x_mm, y_mm, rot_deg)
# Passives with x=None are auto-placed on a grid (see PASSIVE_GRID below).
# ---------------------------------------------------------------------------
FIXED = [
    # ref,        lib,                              footprint,                              value,            x,     y,    rot
    ("J1",  "Connector_PinHeader_2.54mm", "PinHeader_1x15_P2.54mm_Vertical",      "Nano_A",          64.0, 14.0,  0),
    ("J2",  "Connector_PinHeader_2.54mm", "PinHeader_1x15_P2.54mm_Vertical",      "Nano_B",          79.24,14.0,  0),
    ("M1",  "Connector_PinHeader_2.54mm", "PinHeader_2x08_P2.54mm_Vertical",      "TMC2209",         98.0, 14.0,  0),
    ("U2",  "Package_DIP",                "DIP-8_W7.62mm",                        "NE5532",          34.0, 20.0,  0),
    ("J_CART","Connector_PinHeader_2.54mm","PinHeader_1x05_P2.54mm_Vertical",     "CART_IN",          8.0, 18.0,  0),
    ("J_OUT", "Connector_PinHeader_2.54mm","PinHeader_1x03_P2.54mm_Vertical",     "LINE_OUT",         8.0, 92.0,  0),
    ("J_MOT", "TerminalBlock",            "TerminalBlock_bornier-4_P5.08mm",      "MOTOR",          120.0, 55.0,  0),
    ("J_PM",  "TerminalBlock",            "TerminalBlock_bornier-2_P5.08mm",      "PWR_24V",        126.0, 18.0,  0),
    ("J_PL",  "TerminalBlock",            "TerminalBlock_bornier-4_P5.08mm",      "PWR_5V_12V",     120.0, 92.0,  0),
    ("RV1", "Potentiometer_THT",          "Potentiometer_Alps_RK09K_Single_Vertical","10k_SPEED",   24.0,100.0,  0),
    ("SW1", "Button_Switch_THT",          "SW_PUSH_6mm",                          "MODE",            46.0,100.0,  0),
    ("D_RGB","LED_THT",                   "LED_D5.0mm-4_RGB",                     "RGB_MODE",        62.0,102.0,  0),
    ("D_PWR","LED_THT",                   "LED_D5.0mm",                           "POWER",           76.0,102.0,  0),
    ("H1",  "MountingHole",               "MountingHole_3.2mm_M3",                "M3",               6.0,  6.0,  0),
    ("H2",  "MountingHole",               "MountingHole_3.2mm_M3",                "M3",             134.0,  6.0,  0),
    ("H3",  "MountingHole",               "MountingHole_3.2mm_M3",                "M3",               6.0,104.0,  0),
    ("H4",  "MountingHole",               "MountingHole_3.2mm_M3",                "M3",             134.0,104.0,  0),
]

# Passives auto-placed on grids. kind: R=axial resistor, C=disc(film/ceramic),
# E=radial electrolytic. region: "A"=analog left, "D"=digital/power.
RES = "Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal"
CAP = "Capacitor_THT", "C_Disc_D5.0mm_W2.5mm_P5.00mm"
ELE = "Capacitor_THT", "CP_Radial_D6.3mm_P2.50mm"

PASSIVES = [
    # ref,      kind, value,     region
    # --- phono L ---
    ("R_LDL",  RES, "47k",     "A"),
    ("C_LDL",  CAP, "100p",    "A"),
    ("R_AL",   RES, "100k",    "A"),
    ("C_AL",   CAP, "33n",     "A"),
    ("R_BL",   RES, "7k5",     "A"),
    ("C_BL",   CAP, "10n",     "A"),
    ("R_GL",   RES, "1k",      "A"),
    ("C_GL",   ELE, "100u",    "A"),
    ("R_OL",   RES, "100R",    "A"),
    ("C_OL",   ELE, "3u3",     "A"),
    # --- phono R ---
    ("R_LDR",  RES, "47k",     "A"),
    ("C_LDR",  CAP, "100p",    "A"),
    ("R_AR",   RES, "100k",    "A"),
    ("C_AR",   CAP, "33n",     "A"),
    ("R_BR",   RES, "7k5",     "A"),
    ("C_BR",   CAP, "10n",     "A"),
    ("R_GR",   RES, "1k",      "A"),
    ("C_GR",   ELE, "100u",    "A"),
    ("R_OR",   RES, "100R",    "A"),
    ("C_OR",   ELE, "3u3",     "A"),
    # --- op-amp supply decoupling ---
    ("C_VP",   CAP, "100n",    "A"),
    ("C_VN",   CAP, "100n",    "A"),
    ("C_VPB",  ELE, "10u",     "A"),
    ("C_VNB",  ELE, "10u",     "A"),
    # --- digital / power ---
    ("R_R",    RES, "330R",    "D"),
    ("R_G",    RES, "220R",    "D"),
    ("R_B",    RES, "220R",    "D"),
    ("R_PWR",  RES, "1k",      "D"),
    ("R_UART", RES, "1k",      "D"),
    ("C_5V",   CAP, "100n",    "D"),
    ("C_5VB",  ELE, "10u",     "D"),
    ("C_VIO",  CAP, "100n",    "D"),
    ("C_VMOT", ELE, "100u",    "D"),
]

# Grid origins/pitch for auto-placed passives.
GRID = {
    "A": dict(x0=12.0, y0=34.0, dx=12.5, dy=9.0, cols=4),
    "D": dict(x0=64.0, y0=64.0, dx=12.5, dy=9.0, cols=3),
}

# ---------------------------------------------------------------------------
# Netlist: net name -> list of "REF.PAD" (every connection on the board).
# ---------------------------------------------------------------------------
# Nano header pin map (30-pin Nano):
#   J1 (side A) 1..15: D13 3V3 AREF A0 A1 A2 A3 A4 A5 A6 A7 5V RST GND VIN
#   J2 (side B) 1..15: D12 D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 GND RST RX0 TX1
# TMC 2x8 header (StepStick-style), pins 1..16:
#   1 EN   2 VMOT   3 MS1  4 GND   5 MS2  6 2B   7 PDN/UART 8 2A
#   9 nc  10 1A    11 nc  12 1B   13 STEP 14 VIO 15 DIR   16 GND
NETS = {
    "GND": [
        "J1.14", "J2.12",
        "M1.4", "M1.16",
        "J_PM.2", "J_PL.4",
        "J_CART.2", "J_CART.4", "J_CART.5", "J_OUT.2",
        "SW1.2", "RV1.1", "RV1.MP",
        "D_RGB.2",             # RGB common cathode
        "D_PWR.1",             # power LED cathode
        # decoupling / passive returns
        "C_5V.2", "C_5VB.2", "C_VIO.2",
        "C_VP.2", "C_VN.1",    # +12 decoupling to GND (VN cap: + to GND)
        "C_VPB.2", "C_VNB.1",  # bulk (VNB: + to GND)
        "C_GL.2", "C_GR.2",
        "R_LDL.2", "R_LDR.2", "C_LDL.2", "C_LDR.2",
    ],
    "+5V": [
        "J1.12", "J_PL.1",
        "M1.14",               # VIO
        "M1.3", "M1.5",        # MS1/MS2 high -> 1/16 microstep (standalone)
        "RV1.3",
        "R_PWR.1",
        "C_5V.1", "C_5VB.1", "C_VIO.1",
    ],
    "+24V": ["J_PM.1", "M1.2", "C_VMOT.1"],
    "GND_VMOT": ["C_VMOT.2"],   # merged to GND below (star)
    "+12V": ["J_PL.2", "U2.8", "C_VP.1", "C_VPB.1"],
    "-12V": ["J_PL.3", "U2.4", "C_VN.2", "C_VNB.2"],
    # motor phases
    "M2B": ["M1.6", "J_MOT.1"],
    "M2A": ["M1.8", "J_MOT.2"],
    "M1A": ["M1.10", "J_MOT.3"],
    "M1B": ["M1.12", "J_MOT.4"],
    # stepper control
    "STEP": ["J2.10", "M1.13"],
    "DIR":  ["J2.9",  "M1.15"],
    "EN":   ["J2.8",  "M1.1"],
    "TX":   ["J2.15", "R_UART.1"],
    "PDN":  ["R_UART.2", "J2.14", "M1.7"],
    # front panel
    "MODE": ["J2.7", "SW1.1"],
    "POTW": ["RV1.2", "J1.4"],
    "LEDR": ["J2.4", "R_R.1"], "RGB_R": ["R_R.2", "D_RGB.1"],
    "LEDG": ["J2.3", "R_G.1"], "RGB_G": ["R_G.2", "D_RGB.3"],
    "LEDB": ["J2.2", "R_B.1"], "RGB_B": ["R_B.2", "D_RGB.4"],
    "PWRA": ["R_PWR.2", "D_PWR.2"],
    # --- phono L ---
    "INL":  ["J_CART.1", "R_LDL.1", "C_LDL.1", "U2.3"],
    "FBL":  ["U2.2", "R_AL.1", "C_AL.1", "R_BL.2", "R_GL.1"],
    "OUTL": ["U2.1", "R_AL.2", "C_AL.2", "C_BL.1", "R_OL.1"],
    "BML":  ["C_BL.2", "R_BL.1"],
    "GML":  ["R_GL.2", "C_GL.1"],
    "OLL":  ["R_OL.2", "C_OL.1"],
    "LOUTL":["C_OL.2", "J_OUT.1"],
    # --- phono R ---
    "INR":  ["J_CART.3", "R_LDR.1", "C_LDR.1", "U2.5"],
    "FBR":  ["U2.6", "R_AR.1", "C_AR.1", "R_BR.2", "R_GR.1"],
    "OUTR": ["U2.7", "R_AR.2", "C_AR.2", "C_BR.1", "R_OR.1"],
    "BMR":  ["C_BR.2", "R_BR.1"],
    "GMR":  ["R_GR.2", "C_GR.1"],
    "OLR":  ["R_OR.2", "C_OR.1"],
    "LOUTR":["C_OR.2", "J_OUT.3"],
}

# Motor-supply return ties to the single GND star; fold GND_VMOT into GND.
NETS["GND"].extend(NETS.pop("GND_VMOT"))

BOARD_W, BOARD_H = 140.0, 110.0


def add_footprint(board, ref, lib, fp, value, x, y, rot):
    mod = pcbnew.FootprintLoad(f"{FP}/{lib}.pretty", fp)
    if mod is None:
        raise RuntimeError(f"footprint not found: {lib}:{fp}")
    mod.SetReference(ref)
    mod.SetValue(value)
    mod.SetPosition(vec(x, y))
    if rot:
        mod.SetOrientationDegrees(rot)
    board.Add(mod)
    return mod


def main():
    board = pcbnew.CreateEmptyBoard()

    # Two-layer board.
    board.SetCopperLayerCount(2)

    # Place fixed parts.
    placed = {}
    for ref, lib, fp, value, x, y, rot in FIXED:
        placed[ref] = add_footprint(board, ref, lib, fp, value, x, y, rot)

    # Auto-place passives on their region grid.
    counters = {k: 0 for k in GRID}
    for ref, (lib, fp), value, region in PASSIVES:
        g = GRID[region]
        i = counters[region]
        counters[region] += 1
        col = i % g["cols"]
        row = i // g["cols"]
        x = g["x0"] + col * g["dx"]
        y = g["y0"] + row * g["dy"]
        placed[ref] = add_footprint(board, ref, lib, fp, value, x, y, 0)

    # Nets.
    netmap = {}
    for name in NETS:
        n = pcbnew.NETINFO_ITEM(board, name)
        board.Add(n)
        netmap[name] = n

    for name, conns in NETS.items():
        for c in conns:
            ref, pad = c.split(".")
            mod = placed.get(ref)
            if mod is None:
                raise RuntimeError(f"net {name}: unknown ref {ref}")
            hit = False
            for p in mod.Pads():
                if p.GetName() == pad:
                    p.SetNet(netmap[name])
                    hit = True
            if not hit:
                raise RuntimeError(f"net {name}: {ref} has no pad {pad}")

    # Board outline on Edge.Cuts.
    for (x1, y1, x2, y2) in [
        (0, 0, BOARD_W, 0), (BOARD_W, 0, BOARD_W, BOARD_H),
        (BOARD_W, BOARD_H, 0, BOARD_H), (0, BOARD_H, 0, 0),
    ]:
        seg = pcbnew.PCB_SHAPE(board)
        seg.SetShape(pcbnew.SHAPE_T_SEGMENT)
        seg.SetStart(vec(x1, y1))
        seg.SetEnd(vec(x2, y2))
        seg.SetLayer(pcbnew.Edge_Cuts)
        seg.SetWidth(MM(0.15))
        board.Add(seg)

    # GND pours on both copper layers.
    gnd = netmap["GND"]
    inset = 0.5
    corners = [(inset, inset), (BOARD_W - inset, inset),
               (BOARD_W - inset, BOARD_H - inset), (inset, BOARD_H - inset)]
    for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
        zone = pcbnew.ZONE(board)
        zone.SetLayer(layer)
        zone.SetNetCode(gnd.GetNetCode())
        zone.SetLocalClearance(MM(0.3))
        zone.SetMinThickness(MM(0.25))
        outline = zone.Outline()
        outline.NewOutline()
        for (x, y) in corners:
            outline.Append(MM(x), MM(y))
        board.Add(zone)

    pcbnew.SaveBoard("krrl_player.kicad_pcb", board)
    print("wrote krrl_player.kicad_pcb")
    print(f"footprints: {len(list(board.GetFootprints()))}  nets: {board.GetNetCount()}")


if __name__ == "__main__":
    main()

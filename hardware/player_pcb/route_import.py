#!/usr/bin/env python3
"""Import a freerouting Specctra session (.ses) into the board.

KiCad 7's pcbnew.ImportSpecctraSES() only works on the GUI's current board, so
this parses the .ses and injects tracks/vias directly. Then it fills the GND
zones and reports any remaining unrouted connections.

Run after route.sh (which produces krrl_player.ses):
    python3 route_import.py
"""

import sys
import pcbnew

# SES resolution: 1 unit = 100 nm; KiCad y is negated vs Specctra.
U = 100  # nm per SES unit


def to_nm(x, y):
    return pcbnew.VECTOR2I(int(round(float(x) * U)), int(round(-float(y) * U)))


def tokenize(text):
    text = text.replace("(", " ( ").replace(")", " ) ")
    return text.split()


def parse(tokens, i=0):
    """Parse one s-expression starting at tokens[i] == '('. Returns (node, i)."""
    assert tokens[i] == "("
    node = []
    i += 1
    while tokens[i] != ")":
        if tokens[i] == "(":
            child, i = parse(tokens, i)
            node.append(child)
        else:
            node.append(tokens[i])
            i += 1
    return node, i + 1


def find(node, head):
    """Yield child lists whose first element == head."""
    for c in node:
        if isinstance(c, list) and c and c[0] == head:
            yield c


LAYER = {"F.Cu": pcbnew.F_Cu, "B.Cu": pcbnew.B_Cu}


def main():
    board = pcbnew.LoadBoard("krrl_player.kicad_pcb")
    netmap = {}
    ni = board.GetNetInfo()
    for i in range(ni.GetNetCount()):
        n = ni.GetNetItem(i)
        netmap[n.GetNetname()] = n.GetNetCode()

    with open("krrl_player.ses") as f:
        root, _ = parse(tokenize(f.read()), 0)

    routes = next(find(root, "routes"))
    netout = next(find(routes, "network_out"))

    ntrack = nvia = 0
    for net in find(netout, "net"):
        name = net[1].strip('"')
        code = netmap.get(name, 0)
        for wire in find(net, "wire"):
            for path in find(wire, "path"):
                layer = LAYER[path[1]]
                width = int(round(float(path[2]) * U))
                coords = path[3:]
                pts = [(coords[k], coords[k + 1]) for k in range(0, len(coords) - 1, 2)]
                for a, b in zip(pts, pts[1:]):
                    t = pcbnew.PCB_TRACK(board)
                    t.SetStart(to_nm(*a))
                    t.SetEnd(to_nm(*b))
                    t.SetWidth(width)
                    t.SetLayer(layer)
                    t.SetNetCode(code)
                    board.Add(t)
                    ntrack += 1
        for via in find(net, "via"):
            x, y = via[2], via[3]
            v = pcbnew.PCB_VIA(board)
            v.SetPosition(to_nm(x, y))
            v.SetWidth(pcbnew.FromMM(0.8))
            v.SetDrill(pcbnew.FromMM(0.4))
            v.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu)
            v.SetNetCode(code)
            board.Add(v)
            nvia += 1

    # Fill GND pours.
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())
    board.BuildConnectivity()

    print(f"injected tracks: {ntrack}  vias: {nvia}")
    try:
        unrouted = board.GetConnectivity().GetUnconnectedCount()
        print(f"unrouted connections remaining: {unrouted}")
    except Exception as e:
        print("unrouted count unavailable:", e)

    pcbnew.SaveBoard("krrl_player.kicad_pcb", board)
    print("saved routed board")


if __name__ == "__main__":
    main()

#!/usr/bin/env bash
# Export fabrication outputs (Gerbers + drill) and renders from the routed
# board. Requires KiCad 7 kicad-cli and rsvg-convert.
set -euo pipefail
cd "$(dirname "$0")"

rm -rf gerbers && mkdir -p gerbers
kicad-cli pcb export gerbers \
    --layers "F.Cu,B.Cu,F.Mask,B.Mask,F.Silkscreen,B.Silkscreen,F.Paste,B.Paste,Edge.Cuts" \
    --output gerbers/ krrl_player.kicad_pcb
kicad-cli pcb export drill --output gerbers/ krrl_player.kicad_pcb
(cd gerbers && zip -q ../krrl_player_gerbers.zip *)

kicad-cli pcb export svg --layers F.Cu,F.SilkS,F.Fab,Edge.Cuts \
    --page-size-mode 2 --exclude-drawing-sheet -o render_top.svg krrl_player.kicad_pcb
kicad-cli pcb export svg --layers B.Cu,B.SilkS,Edge.Cuts \
    --page-size-mode 2 --exclude-drawing-sheet --mirror -o render_bottom.svg krrl_player.kicad_pcb
rsvg-convert -w 1600 -b white render_top.svg -o render_top.png
rsvg-convert -w 1600 -b white render_bottom.svg -o render_bottom.png
rm -f render_top.svg render_bottom.svg
echo "fab outputs written to gerbers/ + krrl_player_gerbers.zip + render_*.png"

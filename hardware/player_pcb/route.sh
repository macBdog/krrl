#!/usr/bin/env bash
# Regenerate the KRRL-01 Player board end-to-end: build placement + nets,
# autoroute with freerouting (headless via Xvfb), import the session, fill
# pours. Then export fab outputs with fab.sh.
#
# Requirements: KiCad 7 (pcbnew python), Java 21, Xvfb, and freerouting 1.9.0
# (Java-21 compatible). Point FREEROUTING_JAR at the jar.
#
#   FREEROUTING_JAR=/path/to/freerouting-1.9.0.jar ./route.sh
set -euo pipefail
cd "$(dirname "$0")"

python3 build_pcb.py

python3 - <<'PY'
import pcbnew
b = pcbnew.LoadBoard("krrl_player.kicad_pcb")
assert pcbnew.ExportSpecctraDSN(b, "krrl_player.dsn"), "DSN export failed"
print("exported krrl_player.dsn")
PY

xvfb-run -a java -jar "${FREEROUTING_JAR:-freerouting.jar}" \
    -de krrl_player.dsn -do krrl_player.ses -mp 20

python3 route_import.py
echo "routed board written to krrl_player.kicad_pcb"

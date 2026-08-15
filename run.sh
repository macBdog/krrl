#!/bin/sh
# Desktop dry-run. From the KRRL-01 project root:
cd "$(dirname "$0")"
export PYTHONPATH="$PWD/host${PYTHONPATH:+:$PYTHONPATH}"
exec python3 -m krrl --dry-run "$@"

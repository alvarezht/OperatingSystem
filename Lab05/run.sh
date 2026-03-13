#!/usr/bin/env bash
set -euo pipefail

THREADS="${1:-4}"
LOG_FILE="${2:-access.log}"

make clean
make
./log_analyzer_linux "$LOG_FILE" "$THREADS"

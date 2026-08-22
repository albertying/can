#!/bin/bash
set -e
cd "$(dirname "$0")/.."

DBC="tesla_candump/tesla_model3_vehicle.dbc"
LOG="tesla_candump/candump_output.log"
PY="./venv/bin/python"

mkdir -p outputs

./tool "$DBC" "$LOG" 2>/dev/null | sort > outputs/cpp.csv
"$PY" bench_cantools.py "$DBC" "$LOG" 2>/dev/null | sort > outputs/cantools.csv

if diff -q outputs/cpp.csv outputs/cantools.csv > /dev/null; then
    echo "correctness: ok ($(wc -l < outputs/cpp.csv | tr -d ' ') signals)"
else
    echo "FAIL: output mismatch"
    diff outputs/cantools.csv outputs/cpp.csv | head -20
    exit 1
fi

echo ""
echo "=== C++ ==="
time ./tool "$DBC" "$LOG" > /dev/null

echo ""
echo "=== cantools ==="
time "$PY" bench_cantools.py "$DBC" "$LOG" > /dev/null

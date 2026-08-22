#!/bin/sh
set -e
cd "$(dirname "$0")/.."

fail=0
for name in tests/*.dbc; do
    base="${name%.dbc}"
    echo -n "$(basename "$base")... "
    if ./tool "$name" "${base}.log" | diff -u "${base}.expected" - > /dev/null 2>&1; then
        echo "ok"
    else
        echo "FAIL"
        ./tool "$name" "${base}.log" | diff -u "${base}.expected" -
        fail=1
    fi
done

[ $fail -eq 0 ] && echo "all tests passed" || exit 1

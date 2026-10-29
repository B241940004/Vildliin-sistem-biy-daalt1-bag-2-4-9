#!/bin/sh
# Each case: workers queue tasks min max interval seed
cd "$(dirname "$0")/.." || exit 1
fail=0; n=0
check() {
  n=$((n+1))
  out=$(./tpdemo "$@") || { echo "FAIL #$n ($*): program error"; fail=1; return; }
  d=$(echo "$out" | grep -c '"e":"done"')
  if echo "$out" | grep -q '"ok":true' && [ "$d" -eq "$3" ]; then echo "PASS #$n workers=$1 queue=$2 tasks=$3"
  else echo "FAIL #$n ($*)"; fail=1; fi
}
check 1 4 10 10 30 0 1      # single worker
check 4 8 20 10 40 5 2      # normal
check 2 1 15 10 30 0 3      # tiny queue -> back-pressure
check 8 4 50 5 20 0 4       # more workers than queue slots
check 3 10 1 10 10 0 5      # one task only
./tpdemo 0 4 10 10 30 0 >/dev/null 2>&1 && { echo "FAIL invalid input accepted"; fail=1; } || echo "PASS invalid input rejected"
exit $fail

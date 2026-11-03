#!/usr/bin/env bash
# Build the C port and run its unit test against the Lean oracle.
#
#   c/build.sh                 # -> build/c/test_rotation_star, runs it on tests/oracle/oracle.txt
#
# CC defaults to cc. -ffp-contract=off: no fused multiply-add, so the C rounds as Lean's
# compiled Float code does.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
OUT="${OUT:-$ROOT/build/c}"
CC="${CC:-cc}"
mkdir -p "$OUT"
"$CC" -O2 -std=c11 -Wall -Wextra -ffp-contract=off -o "$OUT/test_rotation_star" \
	"$HERE/test_rotation_star.c" "$HERE/rotation_star.c" -lm
"$OUT/test_rotation_star" "$ROOT/tests/oracle/oracle.txt"

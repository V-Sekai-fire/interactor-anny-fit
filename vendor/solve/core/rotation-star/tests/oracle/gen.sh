#!/bin/sh
# Rewrite the Lean oracle the C port and the kernel tests read.
#
#   tests/oracle/gen.sh           # lake exe rotation_star_oracle -> tests/oracle/oracle.txt
#   tests/oracle/gen.sh --check   # exit 1 if the committed oracle differs from Lean's
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
if [ "${1:-}" = --check ]; then
	TMP=$(mktemp -d)
	trap 'rm -rf "$TMP"' EXIT
	( cd "$ROOT/lean" && lake exe rotation_star_oracle "$TMP/oracle.txt" >/dev/null )
	if cmp -s "$TMP/oracle.txt" "$HERE/oracle.txt"; then echo "oracle current"; else echo "stale: tests/oracle/oracle.txt"; exit 1; fi
else
	( cd "$ROOT/lean" && lake exe rotation_star_oracle "$HERE/oracle.txt" )
fi

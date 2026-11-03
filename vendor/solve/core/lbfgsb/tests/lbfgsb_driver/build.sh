#!/bin/sh
# Build and run the host-native L-BFGS-B driver test: guest/drape/lbfgsb.cpp,
# vec_cpu.cpp and lbfgsb_gate.cpp (the sources drape.elf compiles) against
# gates/5-drape/oracle. The log goes to gates/5-drape/lbfgsb/host_native.log.
#
#   tests/lbfgsb_driver/build.sh
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
OUTDIR="$ROOT/build/lbfgsb_driver_test"
LOGS="$ROOT/gates/5-drape/lbfgsb"
CXX=${CXX:-$(ls -d ~/llvm-mingw/llvm-mingw-*-ucrt-x86_64/bin 2>/dev/null | tail -1)/x86_64-w64-mingw32-clang++}
mkdir -p "$OUTDIR" "$LOGS"
# -ffp-contract=off: rv64gc has FMA and clang would contract there too; the
# guest builds these files the same way (CMakeLists.txt).
"$CXX" -O2 -std=c++17 -static -ffp-contract=off -Wall -Wno-unused-function \
  -I"$ROOT/guest/drape" -I"$ROOT/guest/avbd/slang-rt" \
  "$HERE/test.cpp" "$ROOT/guest/drape/lbfgsb.cpp" "$ROOT/guest/drape/lbfgsb_gate.cpp" \
  "$ROOT/guest/drape/vec_cpu.cpp" -o "$OUTDIR/lbfgsb_driver_test.exe"
rc=0
"$OUTDIR/lbfgsb_driver_test.exe" "$ROOT/gates/5-drape/oracle" > "$LOGS/host_native.log" || rc=1
grep -E "^G1|^G2" "$LOGS/host_native.log"
exit $rc

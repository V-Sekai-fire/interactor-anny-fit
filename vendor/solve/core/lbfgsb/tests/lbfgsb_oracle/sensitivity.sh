#!/bin/sh
# G2's flat control (sensitivity.cpp): LBFGSpp (double) from float32-rounded
# inputs against the traces. Writes gates/5-drape/lbfgsb/sensitivity.log.
#
#   tests/lbfgsb_oracle/sensitivity.sh      (ARIA=<clone> to reuse a checkout)
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
ARIA=${ARIA:-$HERE/.deps/interactor-aria-lbfgspp}
LBFGSPP_REV=10086b6b2022802d7942ba16842c43063b3cff91
if [ ! -d "$ARIA/.git" ]; then
  mkdir -p "$(dirname "$ARIA")"
  git clone -q --no-checkout https://github.com/V-Sekai-fire/interactor-aria-lbfgspp.git "$ARIA"
  git -C "$ARIA" config core.longpaths true
  git -C "$ARIA" checkout -q -f "$LBFGSPP_REV"
fi
EIGEN=${EIGEN:-$ARIA/thirdparty/eigen}
CXX=${CXX:-$(ls -d ~/llvm-mingw/llvm-mingw-*-ucrt-x86_64/bin 2>/dev/null | tail -1)/x86_64-w64-mingw32-clang++}
mkdir -p "$ROOT/build" "$ROOT/gates/5-drape/lbfgsb"
"$CXX" -O2 -std=c++17 -static -I"$ARIA/thirdparty/LBFGSpp/include" -I"$EIGEN" \
  "$HERE/sensitivity.cpp" -o "$ROOT/build/lbfgsb_sensitivity.exe"
"$ROOT/build/lbfgsb_sensitivity.exe" "$ROOT/gates/5-drape/oracle" | tee "$ROOT/gates/5-drape/lbfgsb/sensitivity.log"

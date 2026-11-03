#!/bin/sh
# Build and run the L-BFGS-B oracle generator (host-native, clang++ -O2 -std=c++17).
# Writes gates/5-drape/oracle/{components,problems,traces} and gen.log.
#
#   tests/lbfgsb_oracle/build.sh               clone deps if missing, build, generate
#   OUT=<dir> tests/lbfgsb_oracle/build.sh     generate somewhere else (e.g. to diff)
#
# LBFGSpp 0.3.0 and Eigen 3.4.90 both come from V-Sekai-fire/interactor-aria-lbfgspp
# @ 10086b6b (thirdparty/LBFGSpp, thirdparty/eigen), cloned into .deps/ (gitignored).
# LBFGSpp's include/ is byte-identical to cloth-dynamics-standalone/external/LBFGSpp.
# EIGEN=<dir> overrides the Eigen include root. Neither library reaches any guest ELF.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
DEPS="$HERE/.deps"
ARIA="$DEPS/interactor-aria-lbfgspp"
LBFGSPP_REV=10086b6b2022802d7942ba16842c43063b3cff91
if [ ! -d "$ARIA/.git" ]; then
  mkdir -p "$DEPS"
  git clone -q --no-checkout https://github.com/V-Sekai-fire/interactor-aria-lbfgspp.git "$ARIA"
fi
# Eigen's doc/ has paths past Windows MAX_PATH from a deep checkout: long paths
# are enabled for this one repo only (repo-local config, not the machine's).
git -C "$ARIA" config core.longpaths true
git -C "$ARIA" checkout -q -f "$LBFGSPP_REV"
EIGEN=${EIGEN:-$ARIA/thirdparty/eigen}
CXX=${CXX:-$(ls -d ~/llvm-mingw/llvm-mingw-*-ucrt-x86_64/bin 2>/dev/null | tail -1)/x86_64-w64-mingw32-clang++}
"$CXX" -O2 -std=c++17 -static -I"$ARIA/thirdparty/LBFGSpp/include" -I"$EIGEN" \
  "$HERE/gen.cpp" -o "$DEPS/gen.exe"
OUT=${OUT:-$ROOT/gates/5-drape/oracle}
mkdir -p "$OUT/components" "$OUT/problems" "$OUT/traces"
"$DEPS/gen.exe" "$OUT" | tee "$OUT/gen.log"

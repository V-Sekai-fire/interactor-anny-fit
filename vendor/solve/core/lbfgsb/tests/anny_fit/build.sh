#!/bin/sh
# Build fit_anny with the in-guest L-BFGS-B on the CPU path and the Slang-to-CPU phenotype host.
#   tests/anny_fit/build.sh <dir holding the baked soma_pheno.h>
# With SLANG_DIR (the .slang the Lean emitter wrote: `cd core/spec && lake exe emit_shaders slang`, then spec/slang) and the
# Vulkan headers, it also compiles the five phenotype kernels to SPIR-V and builds the GPU comparison (gate G4); the SPIR-V is
# in $OUT/spv. Without them the build is CPU only and G4 is skipped.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)        # core/lbfgsb
CORE=$(cd "$ROOT/.." && pwd)           # core
BAKED=${1:?the directory with the baked soma_pheno.h}
OUT=${OUT:-$HERE/build}
mkdir -p "$OUT"
INC="-I$BAKED -I$CORE -I$CORE/../ports -I$ROOT/guest/drape -I$CORE/slang_rt -I$ROOT/kernels/drape/cpp"
COMMON="-O2 -std=c++20 -ffp-contract=off -Wno-unused-function"
SRC="$HERE/fit_anny.cpp $ROOT/guest/drape/lbfgsb.cpp $ROOT/guest/drape/vec_cpu.cpp"
if [ -n "${SLANG_DIR:-}" ] && [ -f /opt/homebrew/include/vulkan/vulkan.h -o -f /usr/include/vulkan/vulkan.h ]; then
  mkdir -p "$OUT/spv"
  for k in blendshape bary_tet bary_gather rbf skeleton_fit; do
    # scalar layout: float3 is 12 bytes in a StructuredBuffer, as in the CPU path; precise: the same rounding order as -ffp-contract=off
    slangc "$SLANG_DIR/pheno_$k.slang" -entry $k -stage compute -target spirv -force-glsl-scalar-layout -fp-mode precise -o "$OUT/spv/pheno_$k.spv"
  done
  c++ $COMMON $INC -I/opt/homebrew/include -DANNY_GPU $SRC "$HERE/gpu_pheno.cpp" -L/opt/homebrew/lib -lvulkan -Wl,-rpath,/opt/homebrew/lib -o "$OUT/fit_anny"
else
  c++ $COMMON $INC $SRC "$CORE/soma_pheno_slang.cpp" -o "$OUT/fit_anny"
fi
echo "$OUT/fit_anny"

#!/usr/bin/env bash
# Regenerate the phenotype Slang kernels, compile each to the CPU (C++) target,
# build the host harness, and run it against the SOMA golden.  Verifies
# Sinew.SlangCodegen.Pheno end to end on CPU (blendshape + barycentric + RBF).
# Needs the baked assets: viz_native/soma_pheno.{h,bin} + soma_pheno_golden.bin
# (regenerate with: pixi run -e export python make_pheno.py).
set -e
here="$(cd "$(dirname "$0")" && pwd)"
spec="$here/../.."
viz="$spec/../viz_native"

(cd "$spec" && lake exe emit_shaders slang)

# slangc -target cpp for each kernel (entry = the Slang [shader("compute")] name).
slangc "$spec/slang/pheno_blendshape.slang"  -entry blendshape  -stage compute -target cpp -o "$here/pheno_blendshape.gen.cpp"
slangc "$spec/slang/pheno_bary_tet.slang"    -entry bary_tet    -stage compute -target cpp -o "$here/pheno_bary_tet.gen.cpp"
slangc "$spec/slang/pheno_bary_gather.slang" -entry bary_gather -stage compute -target cpp -o "$here/pheno_bary_gather.gen.cpp"
slangc "$spec/slang/pheno_rbf.slang"         -entry rbf         -stage compute -target cpp -o "$here/pheno_rbf.gen.cpp"
slangc "$spec/slang/pheno_skeleton_fit.slang" -entry skeleton_fit -stage compute -target cpp -o "$here/pheno_skeleton_fit.gen.cpp"

# Force-include the slang prelude once globally so the per-namespace re-includes
# in test.cpp are header-guarded out (one set of prelude types, shared).  The
# absolute path is whatever slangc baked into its output's first #include line.
prelude="$(sed -n '1s/.*"\(.*\)".*/\1/p' "$here/pheno_blendshape.gen.cpp")"
g++ -std=c++17 -I "$viz" -include "$prelude" "$here/test.cpp" -o "$here/pheno_cpu_test"

"$here/pheno_cpu_test" "$viz/soma_pheno.bin" "$viz/soma_pheno_golden.bin"

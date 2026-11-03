-- SPDX-License-Identifier: MIT
-- SinewSolve — the body-solve spec: the SO(3)/least-squares math (Math, folded
-- in as solve is its only consumer), vector alignment (Align), bone-length
-- discovery (Anthropometry), rig rest pose (RigRest), and the Slang codegen for
-- the LBS + phenotype kernels (SlangCodegen.{Lbs,Pheno}).  Self-contained.
--   lake exe align_test     — sanity-check align_vectors against a known rotation
--   lake exe emit_shaders   — dump the SlangCodegen kernels to .slang
import Lake
open Lake DSL

-- Property-based testing: Plausible (QuickCheck for Lean4), pinned to our toolchain.
require plausible from git "https://github.com/leanprover-community/plausible" @ "v4.30.0-rc2"

package "SinewSolve" where
  version := v!"0.1.0"

lean_lib SinewSolve where
  srcDir := "."
  globs  := #[Glob.one `Sinew.Math, Glob.one `Sinew.Align, Glob.one `Sinew.Anthropometry,
              Glob.one `Sinew.RigRest, Glob.one `Sinew.ImuToBody, Glob.submodules `Sinew.SlangCodegen]

lean_exe align_test where
  root := `AlignTest

lean_exe emit_shaders where
  root := `EmitShaders

lean_exe imu_to_body_test where
  root := `ImuToBodyTest

-- Property tests over the body-solve spec (build fails on a counterexample).
lean_exe proptest where
  root := `Tests

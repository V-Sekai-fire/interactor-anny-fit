-- SPDX-License-Identifier: MIT
-- Copyright (c) 2026-present K. S. Ernest (iFire) Lee
--
-- emit_shaders — dump every Sinew.SlangCodegen.* kernel to disk as .slang, the
-- input to slangc (CPU + GPU targets).  Lean is the source of truth for the
-- deform; this exe materialises the shaders.
--
--   lake exe emit_shaders [outDir]      (default: ./slang)
import Sinew.SlangCodegen.Lbs
import Sinew.SlangCodegen.Pheno

def kernels : List (String × String) :=
  [ ("lbs", Sinew.SlangCodegen.Lbs.shader)
  , ("pheno_blendshape", Sinew.SlangCodegen.Pheno.blendshape)
  , ("pheno_bary_tet", Sinew.SlangCodegen.Pheno.baryTet)
  , ("pheno_bary_gather", Sinew.SlangCodegen.Pheno.baryGather)
  , ("pheno_rbf", Sinew.SlangCodegen.Pheno.rbf)
  , ("pheno_skeleton_fit", Sinew.SlangCodegen.Pheno.skeletonFit)
  , ("pheno_se3_inverse", Sinew.SlangCodegen.Pheno.se3Inverse) ]

def main (args : List String) : IO Unit := do
  let outDir := (args[0]?).getD "slang"
  IO.FS.createDirAll outDir
  for (name, src) in kernels do
    let path := s!"{outDir}/{name}.slang"
    IO.FS.writeFile path src
    IO.println s!"wrote {path} ({src.length} bytes)"

import Lake
open Lake DSL

package Drape where

-- Every dependency is pinned in a V-Sekai-fire repo or fork.
require LeanSlang from git
  "https://github.com/V-Sekai-fire/contract-lean-slang.git" @ "60532aef8ed70cc669ecab481182d0636c9e1ac3"

-- drape.elf's L-BFGS-B kernels (Cut 5). A default target, so a bare
-- `lake build` checks their native_decide pins as well as Cloth's.
@[default_target] lean_lib Drape

lean_exe emit_drape where
  root := `EmitDrape

-- SPDX-License-Identifier: MIT
import LeanSlang
import RotationStar.Slang.StarKabsch
import RotationStar.Slang.SwingTwist

/-!
# `emit_rotation_star` — write the kernels as Slang

    lake exe emit_rotation_star <outDir>

`kernels/gen.sh` takes it from there (slangc to C++ and SPIR-V).
-/

open LeanSlang

def kernels : List (String × SlangShaderModule) :=
  [ ("star_kabsch", RotationStar.Slang.StarKabsch.shader)
  , ("swing_twist", RotationStar.Slang.SwingTwist.shader) ]

def main (args : List String) : IO UInt32 := do
  let outDir := args.headD "."
  IO.FS.createDirAll outDir
  for (name, m) in kernels do
    let path := outDir ++ "/" ++ name ++ ".slang"
    IO.FS.writeFile path (LeanSlang.emit m ++ "\n")
    IO.println s!"wrote {path}"
  return 0

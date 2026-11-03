import Lake
open Lake DSL

package RotationStar where

-- Every dependency is pinned by SHA in a V-Sekai-fire repo or fork. LeanSlang is the
-- emit-fp commit contract-anny-kernels and lbfgsb pin, so a consumer resolves one copy.
require LeanSlang from git
  "https://github.com/V-Sekai-fire/contract-lean-slang.git" @ "60532aef8ed70cc669ecab481182d0636c9e1ac3"

-- Plausible v4.34.0 (the V-Sekai-fire fork), for the property checks in RotationStar.Props.
require plausible from git
  "https://github.com/V-Sekai-fire/plausible.git" @ "118aa17ee84656b8bd727fef7c458ee8c833385c"

-- The solver: star, Kabsch fit, swing-twist split, the sinew align recipe, the spec proofs and
-- the Slang emitters with their native_decide pins. A default target, so a bare `lake build`
-- checks every proof and pin.
@[default_target] lean_lib RotationStar

-- Property checks with planted controls; exits 1 if a check fails or a control passes.
lean_exe rotation_star_test where
  root := `Tests

-- Writes the oracle the C port and the kernel tests compare against.
lean_exe rotation_star_oracle where
  root := `Oracle

-- Writes the kernels as Slang for kernels/gen.sh.
lean_exe emit_rotation_star where
  root := `EmitKernels

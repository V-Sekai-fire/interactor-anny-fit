-- SPDX-License-Identifier: MIT
-- Property tests over the body-solve spec, using Plausible (QuickCheck for Lean4).
-- The #test lines run at build time and fail the build on a counterexample.
import Plausible
import Sinew.Anthropometry
open Plausible Sinew.Anthro

-- The bone table is correctly indexed: allBones[i].idx = i for every slot, so the
-- 7 symmetric bone-length unknowns address 0..numBones-1 bijectively.  (Native Nat
-- generator — no custom Bone instance needed.)
#test (∀ (n : Nat), n < numBones → (allBones[n]?.map Bone.idx) = some n)
#test (allBones.length = numBones)

def main : IO Unit :=
  IO.println "solve/proptest: Anthropometry properties run at build time"

-- SPDX-License-Identifier: MIT
-- Checks the executable `align_vectors` recipe recovers a known rotation, so the
-- Lean port matches the Slang `finishAlign` it mirrors.
import Sinew.Align
import Sinew.Math

open Sinew.Math Sinew.Align

private def maxErr (R : M3) (Rrec : Array Float) (srcs : Array V3) : Float := Id.run do
  let mut e := 0.0
  for b in srcs do
    e := max e ((((R.mul b).sub (apply9 Rrec b))).norm)
  return e

def main : IO Unit := do
  -- A known rotation (unit quaternion ½(1,1,1,1) — a 120° turn) and a spread of
  -- source vectors; align over (R·b, b) pairs must return R back.
  let R := quatToMat 0.5 0.5 0.5 0.5
  let srcs : Array V3 := #[⟨1,0,0⟩, ⟨0,1,0⟩, ⟨0,0,1⟩, ⟨1,1,0⟩, ⟨0.3,-0.7,0.5⟩]
  let pairs := srcs.map (fun b => (R.mul b, b))
  let e := maxErr R (align pairs) srcs
  IO.println s!"align (N={pairs.size}): max recovery error = {e}"

  -- Two-pair path (covariance → ns30) and the single-pair rodrigues path.
  let two := srcs.extract 0 2 |>.map (fun b => (R.mul b, b))
  IO.println s!"align (N=2): max recovery error = {maxErr R (align two) (srcs.extract 0 2)}"
  let one := #[(R.mul ⟨0,1,0⟩, (⟨0,1,0⟩ : V3))]
  IO.println s!"rodrigues (N=1): error = {maxErr R (align one) #[⟨0,1,0⟩]}"

  if e < 1e-4 then IO.println "OK" else IO.println "FAIL"

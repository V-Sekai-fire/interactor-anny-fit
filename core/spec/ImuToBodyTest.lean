-- SPDX-License-Identifier: MIT
-- Checks the executable imu_to_body recipe: multi-pose calibration recovers a fresh pose
-- exactly, one reference pose is underdetermined (heading free), and a swapped tracker is
-- caught. Mirrors the controls in adapters/imu_to_body.py's --self-test.
import Sinew.ImuToBody
import Sinew.Math

open Sinew.Math Sinew.ImuToBody

/-- A deterministic spread of distinct rotations (stands in for random poses). -/
def rotN (i : Nat) : M3 :=
  let a := Float.ofNat i * 0.7 + 0.11
  let h := a * 0.5
  let s := Float.sin h
  quatToMat (Float.cos h) (s * Float.sin (a*1.3)) (s * Float.cos (a*0.9)) (s * Float.sin (a*0.5+1.0))

def Strue : M3 := rotN 7
def Mtrue (n : Nat) : M3 := rotN (50 + n)

/-- 24 local rotations for reference pose `p` (or the test pose at p = 9). -/
def localPose (p : Nat) : Array M3 := (Array.range 24).map (fun j => rotN (p*100 + j + 1))

/-- Synthesize the 15 tracker readings for a pose: sensor n = Strue * jointGlobal j * Mtrue n. -/
def synth (localRot : Array M3) : Array M3 :=
  let g := fk localRot
  (Array.range node2joint.size).map (fun n => compose (compose Strue g[node2joint[n]!]!) (Mtrue n))

/-- Max geodesic error (rad) over the mapped joints' global orientations vs the true globals. -/
def mappedErr (gotGlobals truthGlobals : Array M3) : Float := Id.run do
  let mut e := 0.0
  for n in [0:node2joint.size] do
    let j := node2joint[n]!
    e := max e (M3.rotAngle gotGlobals[j]! truthGlobals[j]!)
  return e

def main : IO Unit := do
  let truth := localPose 9
  let truthG := fk truth
  let refs3 : Array RefFrame :=
    #[⟨synth (localPose 1), localPose 1⟩, ⟨synth (localPose 2), localPose 2⟩,
      ⟨synth (localPose 3), localPose 3⟩]

  -- 1. Multi-pose calibration recovers a fresh pose exactly.
  let (S, M) := calibrate refs3
  let e1 := mappedErr (globals (synth truth) S M) truthG
  IO.println s!"multi-pose calibration, fresh-pose max error = {rad2deg e1} deg"

  -- 2. One reference pose is underdetermined: S falls back to identity, so it must NOT recover.
  let (S1, M1) := calibrate (refs3.extract 0 1)
  let e2 := mappedErr (globals (synth truth) S1 M1) truthG
  IO.println s!"single-pose (underdetermined) max error = {rad2deg e2} deg"

  -- 3. Negative control: swap one tracker's reading with another's; its joint must break.
  let bad := (synth truth).set! 8 ((synth truth)[0]!)
  let badG := globals bad S M
  let e3 := M3.rotAngle badG[node2joint[8]!]! truthG[node2joint[8]!]!
  IO.println s!"swapped-tracker control, that joint error = {rad2deg e3} deg"

  if rad2deg e1 < 0.1 ∧ rad2deg e2 > 5.0 ∧ rad2deg e3 > 5.0 then
    IO.println "OK"
  else
    IO.println "FAIL"

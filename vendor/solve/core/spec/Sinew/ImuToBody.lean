-- SPDX-License-Identifier: MIT
-- Copyright (c) 2026-present K. S. Ernest (iFire) Lee
--
-- Layer 2 of the rebocap split: per-tracker orientation offsets -> 24-joint body pose.
-- (Layer 1, the raw-sensor -> per-tracker fusion, is the tracker firmware's job and is
-- recovered elsewhere; this replaces the vendor IK/body model.)
--
-- Model, for a tracker n worn on body joint j = node2joint[n], all rotations M3:
--
--     sensor n = S * jointGlobal j * M n
--
-- S is one world rotation shared by every tracker; M n is the constant sensor-to-bone mount.
-- A single reference pose cannot separate S from the mounts (a shared yaw cancels), so
-- `calibrate` takes >= 2 reference poses whose local rotations are known: the relative rotation
-- between poses cancels M (dSensor = S * dJoint * S^-1), so S is `Align.align` on the delta axes,
-- and each M then follows from one reference frame. `solve` inverts the model and walks the tree
-- to local rotations. Mirrors adapters/imu_to_body.py.
import Sinew.Math
import Sinew.Align

namespace Sinew.ImuToBody
open Sinew.Math

/-- The 24-joint body skeleton's kinematic tree; -1 is the root (pelvis). -/
def parent : Array Int :=
  #[-1, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 9, 12, 13, 14, 16, 17, 18, 19, 20, 21]

/-- rebocap's 15 trackers to their body joint (node index -> joint index); root tracker is 0. -/
def node2joint : Array Nat := #[0, 1, 2, 4, 5, 7, 8, 9, 15, 16, 17, 18, 19, 20, 21]

@[inline] def inv (m : M3) : M3 := m.transpose          -- rotation inverse = transpose
@[inline] def compose (a b : M3) : M3 := M3.mulM a b     -- (compose a b).mul v = a.mul (b.mul v)
def ident : M3 := ⟨⟨1,0,0⟩,⟨0,1,0⟩,⟨0,0,1⟩⟩

/-- Axis * angle of a rotation, canonical (angle in [0, pi]); zero for the identity. -/
def rotVec (m : M3) : V3 :=
  let (w, x, y, z) := matToQuat m
  let (w, x, y, z) := if w < 0 then (-w, -x, -y, -z) else (w, x, y, z)
  let s := Float.sqrt (x*x + y*y + z*z)
  if s < 1e-9 then ⟨0,0,0⟩
  else let ang := 2.0 * Float.acos (min 1.0 w); ⟨x/s*ang, y/s*ang, z/s*ang⟩

def m3OfRows (r : Array Float) : M3 :=
  ⟨⟨r[0]!, r[1]!, r[2]!⟩, ⟨r[3]!, r[4]!, r[5]!⟩, ⟨r[6]!, r[7]!, r[8]!⟩⟩

/-- Forward kinematics: 24 local rotations -> 24 global (compose down the tree). -/
def fk (localRot : Array M3) : Array M3 := Id.run do
  let mut g := Array.replicate 24 ident
  for j in [0:24] do
    let p := parent[j]!
    g := g.set! j (if p < 0 then localRot[j]! else compose g[p.toNat]! localRot[j]!)
  return g

/-- Global rotations -> local (inverse of `fk`). -/
def toLocal (g : Array M3) : Array M3 := Id.run do
  let mut l := Array.replicate 24 ident
  for j in [0:24] do
    let p := parent[j]!
    l := l.set! j (if p < 0 then g[j]! else compose (inv g[p.toNat]!) g[j]!)
  return l

/-- A reference frame: the 15 tracker readings and the known 24 local rotations at that pose. -/
structure RefFrame where
  sensors  : Array M3    -- indexed by node 0..14
  localRot : Array M3    -- 24 body-joint local rotations
deriving Inhabited

/-- Solve the shared world S (row-major) and per-node mounts M from >= 2 reference frames. -/
def calibrate (refs : Array RefFrame) : (Array Float) × (Array M3) := Id.run do
  let bones := refs.map (fun f => fk f.localRot)
  -- S: axis(dSensor) = S * axis(dJoint) over every pose pair and tracker.
  let mut pairs : Array (V3 × V3) := #[]
  for a in [0:refs.size] do
    for b in [a+1:refs.size] do
      for n in [0:node2joint.size] do
        let j := node2joint[n]!
        let dB := rotVec (compose (bones[b]!)[j]! (inv (bones[a]!)[j]!))
        let dS := rotVec (compose refs[b]!.sensors[n]! (inv refs[a]!.sensors[n]!))
        if dB.norm > 0.03 ∧ dS.norm > 0.03 then     -- ~2 deg: real motion only
          pairs := pairs.push (dS, dB)
  let S := if pairs.size < 3 then #[1,0,0,0,1,0,0,0,1] else Align.align pairs
  let Sm := m3OfRows S
  -- M n = jointGlobal j ^-1 * S^-1 * sensor n, from the first reference frame (exact when rigid).
  let mut M := Array.replicate node2joint.size ident
  for n in [0:node2joint.size] do
    let j := node2joint[n]!
    M := M.set! n (compose (inv (bones[0]!)[j]!) (compose (inv Sm) refs[0]!.sensors[n]!))
  return (S, M)

/-- Trackers this frame -> 24 joint global orientations. jointGlobal j = S^-1 * sensor n * M n^-1
    for a mapped joint (the directly recoverable quantity); joints with no tracker inherit their
    parent. Only the mapped joints' globals carry information. -/
def globals (sensors : Array M3) (S : Array Float) (M : Array M3) : Array M3 := Id.run do
  let Sm := m3OfRows S
  let mut present := Array.replicate 24 false
  let mut g := Array.replicate 24 ident
  for n in [0:node2joint.size] do
    let j := node2joint[n]!
    g := g.set! j (compose (inv Sm) (compose sensors[n]! (inv M[n]!)))
    present := present.set! j true
  for j in [0:24] do
    if ¬ present[j]! then
      let p := parent[j]!
      g := g.set! j (if p < 0 then ident else g[p.toNat]!)
  return g

/-- Trackers this frame -> 24 body-joint local rotations (the global chain walked to local).
    A mapped joint whose parent is also mapped has an exact local rotation; where the parent is
    unmapped the local rotation is only as good as the parent's inherited global. -/
def solve (sensors : Array M3) (S : Array Float) (M : Array M3) : Array M3 :=
  toLocal (globals sensors S M)

end Sinew.ImuToBody

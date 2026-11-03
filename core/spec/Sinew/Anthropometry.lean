-- SPDX-License-Identifier: MIT
-- Copyright (c) 2026-present K. S. Ernest (iFire) Lee
--
-- Bone-length discovery. With segment orientations known (9-axis), a joint
-- position is linear in the bone lengths: p = Σ Lᵢ·dᵢ. Anchoring a foot on the
-- floor (y=0) and reading head height from the HMD gives one scalar equation per
-- pose (head.y = Σ L_s·d_s.y; the horizontal root cancels); stacking poses →
-- least squares (`Sinew.Math.solveLS`) for the 7 symmetric lengths. Literature:
-- de Leva 1996 (symmetric scaling), McGrath & Stirling 2020 (leg-disc term) —
-- solve/references.bib.
import Sinew.Math

namespace Sinew.Anthro

/-- Symmetric bone-length unknowns (index into the solution vector). -/
inductive Bone where
  | shin | thigh | spine | neck | shoulder | upperArm | foreArm
deriving Repr, DecidableEq

def Bone.idx : Bone → Nat
  | .shin => 0 | .thigh => 1 | .spine => 2 | .neck => 3
  | .shoulder => 4 | .upperArm => 5 | .foreArm => 6

def Bone.name : Bone → String
  | .shin => "shin" | .thigh => "thigh" | .spine => "spine" | .neck => "neck"
  | .shoulder => "shoulder" | .upperArm => "upperArm" | .foreArm => "foreArm"

def numBones : Nat := 7
def allBones : List Bone := [.shin, .thigh, .spine, .neck, .shoulder, .upperArm, .foreArm]

/-- One constraint row from a chain of (bone, vertical direction component). -/
def chainRow (chain : List (Bone × Float)) : Array Float := Id.run do
  let mut row : Array Float := Array.replicate numBones 0.0
  for (b, dy) in chain do row := row.set! b.idx (row[b.idx]! + dy)
  return row

end Sinew.Anthro

-- SPDX-License-Identifier: MIT
-- Copyright (c) 2026-present K. S. Ernest (iFire) Lee
--
-- Shared numerics: 3-vectors, 3×3 matrices, a dense least-squares solver, and
-- formatting. Used by Fusion (orientation) and Anthropometry (bone lengths).
namespace Sinew.Math

structure V3 where
  x : Float
  y : Float
  z : Float
deriving Repr, Inhabited

namespace V3
@[inline] def add (a b : V3) : V3 := ⟨a.x+b.x, a.y+b.y, a.z+b.z⟩
@[inline] def sub (a b : V3) : V3 := ⟨a.x-b.x, a.y-b.y, a.z-b.z⟩
@[inline] def scale (s : Float) (a : V3) : V3 := ⟨s*a.x, s*a.y, s*a.z⟩
@[inline] def dot (a b : V3) : Float := a.x*b.x + a.y*b.y + a.z*b.z
@[inline] def cross (a b : V3) : V3 :=
  ⟨a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x⟩
@[inline] def norm (a : V3) : Float := Float.sqrt (dot a a)
/-- Unit vector, or zero if degenerate. -/
@[inline] def normalize (a : V3) : V3 :=
  let n := norm a; if n < 1e-9 then ⟨0,0,0⟩ else scale (1.0 / n) a
def angle (a b : V3) : Float :=
  Float.acos (max (-1.0) (min 1.0 (dot (normalize a) (normalize b))))
end V3

/-- 3×3 matrix stored by rows; `mul` applies it (rows · v). -/
structure M3 where
  r0 : V3
  r1 : V3
  r2 : V3
deriving Repr, Inhabited

namespace M3
@[inline] def mul (m : M3) (v : V3) : V3 := ⟨V3.dot m.r0 v, V3.dot m.r1 v, V3.dot m.r2 v⟩
def c0 (m : M3) : V3 := ⟨m.r0.x, m.r1.x, m.r2.x⟩
def c1 (m : M3) : V3 := ⟨m.r0.y, m.r1.y, m.r2.y⟩
def c2 (m : M3) : V3 := ⟨m.r0.z, m.r1.z, m.r2.z⟩
def transpose (m : M3) : M3 := ⟨m.c0, m.c1, m.c2⟩
/-- Product: `(mulM a b).mul v = a.mul (b.mul v)`. -/
def mulM (a b : M3) : M3 :=
  let combo (r : V3) := (b.r0.scale r.x).add ((b.r1.scale r.y).add (b.r2.scale r.z))
  ⟨combo a.r0, combo a.r1, combo a.r2⟩
/-- Geodesic angle (rad) between two rotations. -/
def rotAngle (a b : M3) : Float :=
  let d := mulM a (transpose b)
  Float.acos (max (-1.0) (min 1.0 ((d.r0.x + d.r1.y + d.r2.z - 1.0) / 2.0)))
/-- Gram–Schmidt rows into a proper rotation (3rd = 1st×2nd). -/
def orthonormalize (m : M3) : M3 :=
  let a := m.r0.normalize
  let b := (m.r1.sub (a.scale (V3.dot a m.r1))).normalize
  ⟨a, b, a.cross b⟩
/-- Per-row lerp toward `b` by `t`, re-orthonormalised. -/
def blend (t : Float) (a b : M3) : M3 :=
  orthonormalize ⟨a.r0.scale (1.0-t) |>.add (b.r0.scale t),
                  a.r1.scale (1.0-t) |>.add (b.r1.scale t),
                  a.r2.scale (1.0-t) |>.add (b.r2.scale t)⟩
end M3

/-- Quaternion (w,x,y,z) → rotation matrix (sensor→world). -/
def quatToMat (qw qx qy qz : Float) : M3 :=
  let n := Float.sqrt (qw*qw + qx*qx + qy*qy + qz*qz)
  if n < 1e-9 then ⟨⟨1,0,0⟩,⟨0,1,0⟩,⟨0,0,1⟩⟩ else
  let w := qw/n; let x := qx/n; let y := qy/n; let z := qz/n
  ⟨⟨1-2*(y*y+z*z), 2*(x*y-z*w),   2*(x*z+y*w)⟩,
   ⟨2*(x*y+z*w),   1-2*(x*x+z*z), 2*(y*z-x*w)⟩,
   ⟨2*(x*z-y*w),   2*(y*z+x*w),   1-2*(x*x+y*y)⟩⟩

/-- Rotation matrix → quaternion (w,x,y,z) (Shepperd's method, branch on trace). -/
def matToQuat (m : M3) : Float × Float × Float × Float :=
  let tr := m.r0.x + m.r1.y + m.r2.z
  if tr > 0 then
    let s := Float.sqrt (tr + 1.0) * 2.0
    (0.25*s, (m.r2.y - m.r1.z)/s, (m.r0.z - m.r2.x)/s, (m.r1.x - m.r0.y)/s)
  else if m.r0.x > m.r1.y ∧ m.r0.x > m.r2.z then
    let s := Float.sqrt (1.0 + m.r0.x - m.r1.y - m.r2.z) * 2.0
    ((m.r2.y - m.r1.z)/s, 0.25*s, (m.r0.y + m.r1.x)/s, (m.r0.z + m.r2.x)/s)
  else if m.r1.y > m.r2.z then
    let s := Float.sqrt (1.0 + m.r1.y - m.r0.x - m.r2.z) * 2.0
    ((m.r0.z - m.r2.x)/s, (m.r0.y + m.r1.x)/s, 0.25*s, (m.r1.z + m.r2.y)/s)
  else
    let s := Float.sqrt (1.0 + m.r2.z - m.r0.x - m.r1.y) * 2.0
    ((m.r1.x - m.r0.y)/s, (m.r0.z + m.r2.x)/s, (m.r1.z + m.r2.y)/s, 0.25*s)

-- ── Dense least squares (over-determined A x = b) ─────────────────────────────

/-- Normal equations: (AᵀA, Aᵀb) for `n` unknowns. -/
def normalEq (rows : Array (Array Float)) (rhs : Array Float) (n : Nat) :
    (Array (Array Float)) × (Array Float) := Id.run do
  let mut ata : Array (Array Float) := Array.replicate n (Array.replicate n 0.0)
  let mut atb : Array Float := Array.replicate n 0.0
  for k in [0:rows.size] do
    let r := rows[k]!; let y := rhs[k]!
    for i in [0:n] do
      atb := atb.set! i (atb[i]! + r[i]! * y)
      let mut rowi := ata[i]!
      for j in [0:n] do rowi := rowi.set! j (rowi[j]! + r[i]! * r[j]!)
      ata := ata.set! i rowi
  return (ata, atb)

/-- Gaussian elimination with partial pivoting; none if (near-)singular. -/
def solveDense (a0 : Array (Array Float)) (b0 : Array Float) (n : Nat) :
    Option (Array Float) := Id.run do
  let mut m := a0; let mut b := b0
  for col in [0:n] do
    let mut piv := col; let mut best := Float.abs (m[col]![col]!)
    for r in [col+1:n] do
      let v := Float.abs (m[r]![col]!)
      if v > best then best := v; piv := r
    if best < 1e-12 then return none
    if piv != col then
      let tr := m[col]!; m := m.set! col m[piv]!; m := m.set! piv tr
      let tb := b[col]!; b := b.set! col b[piv]!; b := b.set! piv tb
    let pivRow := m[col]!; let pivB := b[col]!
    for r in [col+1:n] do
      let f := m[r]![col]! / pivRow[col]!
      let mut rr := m[r]!
      for j in [col:n] do rr := rr.set! j (rr[j]! - f * pivRow[j]!)
      m := m.set! r rr; b := b.set! r (b[r]! - f * pivB)
  let mut x : Array Float := Array.replicate n 0.0
  for ii in [0:n] do
    let i := n - 1 - ii
    let mut s := b[i]!
    for j in [i+1:n] do s := s - m[i]![j]! * x[j]!
    x := x.set! i (s / m[i]![i]!)
  return some x

/-- Least squares of an over-determined system via the normal equations. -/
def solveLS (rows : Array (Array Float)) (rhs : Array Float) (n : Nat) : Option (Array Float) :=
  let (ata, atb) := normalEq rows rhs n
  solveDense ata atb n

-- ── Formatting ────────────────────────────────────────────────────────────────

def rad2deg (r : Float) : Float := r * 180.0 / 3.14159265358979

/-- Round to `digits` decimal places (for display). -/
def roundTo (digits : Nat) (x : Float) : Float :=
  let p := Float.ofNat (10 ^ digits)
  let s := if x < 0 then -1.0 else 1.0
  s * (Float.ofNat ((Float.abs x * p + 0.5).toUInt64.toNat)) / p

end Sinew.Math

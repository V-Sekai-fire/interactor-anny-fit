-- SPDX-License-Identifier: MIT
/-!
# `RotationStar.Vec` — 3-vectors and row-major 3×3 matrices over `Float`

The shapes of sinew-align's `Sinew.Math` (`V3`, `M3` stored by rows), with the same names and
the same arithmetic, so a sinew consumer can alias them. Added: column access, the identity,
the determinant, `ofCols`, and `unitK`, the normalisation of swing-twist-kusudama's
`KusKabsch.lean` (below a length of 1e-12 it returns its input, where `normalize` returns 0).
-/

namespace RotationStar

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
/-- Unit vector, or zero if degenerate (sinew's floor, 1e-9). -/
@[inline] def normalize (a : V3) : V3 :=
  let n := norm a; if n < 1e-9 then ⟨0,0,0⟩ else scale (1.0 / n) a
/-- Unit vector, or `a` itself below a length of 1e-12 (KusKabsch's `norm`; the GPU split keeps
    the same floor so the kernels and this spec take the same branch). Divides each component by
    the length, as the kernels do, so the two round alike. -/
@[inline] def unitK (a : V3) : V3 :=
  let l := norm a; if l > 1e-12 then ⟨a.x / l, a.y / l, a.z / l⟩ else a
def angle (a b : V3) : Float :=
  Float.acos (max (-1.0) (min 1.0 (dot (normalize a) (normalize b))))
def ex : V3 := ⟨1, 0, 0⟩
def ey : V3 := ⟨0, 1, 0⟩
def ez : V3 := ⟨0, 0, 1⟩
def zero : V3 := ⟨0, 0, 0⟩
/-- The unit axis `ax` (0 = x, 1 = y, otherwise z). -/
def axisUnit (ax : Nat) : V3 := if ax == 0 then ex else if ax == 1 then ey else ez
/-- Component `i` (0 = x, 1 = y, otherwise z). -/
def get (a : V3) (i : Nat) : Float := if i == 0 then a.x else if i == 1 then a.y else a.z
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
/-- The matrix whose columns are `a`, `b`, `c`. -/
def ofCols (a b c : V3) : M3 := ⟨⟨a.x, b.x, c.x⟩, ⟨a.y, b.y, c.y⟩, ⟨a.z, b.z, c.z⟩⟩
def id : M3 := ⟨V3.ex, V3.ey, V3.ez⟩
/-- Product: `(mulM a b).mul v = a.mul (b.mul v)`. -/
def mulM (a b : M3) : M3 :=
  let combo (r : V3) := (b.r0.scale r.x).add ((b.r1.scale r.y).add (b.r2.scale r.z))
  ⟨combo a.r0, combo a.r1, combo a.r2⟩
def det (m : M3) : Float := V3.dot m.r0 (V3.cross m.r1 m.r2)
/-- Entry (i, j). -/
def get (m : M3) (i j : Nat) : Float :=
  (if i == 0 then m.r0 else if i == 1 then m.r1 else m.r2).get j
/-- Largest absolute entry difference. -/
def maxDiff (a b : M3) : Float := Id.run do
  let mut e := 0.0
  for i in [0:3] do
    for j in [0:3] do
      e := max e (Float.abs (a.get i j - b.get i j))
  return e
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
/-- Row-major 9 floats. -/
def toArr (m : M3) : Array Float :=
  #[m.r0.x, m.r0.y, m.r0.z, m.r1.x, m.r1.y, m.r1.z, m.r2.x, m.r2.y, m.r2.z]
def ofArr (a : Array Float) : M3 :=
  ⟨⟨a[0]!, a[1]!, a[2]!⟩, ⟨a[3]!, a[4]!, a[5]!⟩, ⟨a[6]!, a[7]!, a[8]!⟩⟩
/-- The rotation from the 6D pair (the first two columns `a`, `b`): Gram–Schmidt, third column
    `c0 × c1`. The operator's rotation parameterisation; used to make test rotations. -/
def of6D (a b : V3) : M3 :=
  let c0 := a.normalize
  let c1 := (b.sub (c0.scale (V3.dot c0 b))).normalize
  ofCols c0 c1 (c0.cross c1)
end M3

end RotationStar

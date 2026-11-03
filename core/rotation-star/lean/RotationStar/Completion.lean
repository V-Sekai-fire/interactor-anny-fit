-- SPDX-License-Identifier: MIT
import RotationStar.Vec

/-!
# `RotationStar.Completion` — the rank-1 Kabsch completion with the minimal twist

From swing-twist-kusudama `KusKabsch.lean` and entities-godot `ik_kabsch_6d.cpp` (`svd3`). When
the cross-covariance has rank 1 only one correspondence is pinned, `u0 ← v0`; the plane normal
to it is free. Any orthonormal completion of `u0` is a legal rotation, so pick the canonical one:
carry V's free axes `v1`, `v2` through the shortest arc `R0 : v0 → u0`. Then `U = [u0, R0 v1,
R0 v2]`, `V = [v0, v1, v2]` and `R = U Vᵀ = R0`, the swing alone, with no twist about `u0`.

`|v0 × u0| < 1e-9` is the C++ fallback: aligned keeps `(v1, v2)`; antipodal takes Godot's
`u0.get_any_perpendicular()` and `u0 × c1`. The 1e-9 cut is ik_kabsch_6d.cpp's; below it the
arc's axis `k` is rounding noise.

Every step is a 3-vector or a 3×3, in the operation order the kernels emit
(`RotationStar.Slang.SwingTwist`), so the double spec and the float kernels round alike.
-/

namespace RotationStar
open V3

/-- `rod k cs sn w = cs w + sn (k × w) + (k·w)(1 − cs) k`: rotate `w` about the unit axis `k` by
    the angle with cosine `cs` and sine `sn` (Rodrigues). -/
def rod (k : V3) (cs sn : Float) (w : V3) : V3 :=
  let kx := cross k w
  let kw := dot k w
  ⟨cs * w.x + sn * kx.x + kw * (1.0 - cs) * k.x,
   cs * w.y + sn * kx.y + kw * (1.0 - cs) * k.y,
   cs * w.z + sn * kx.z + kw * (1.0 - cs) * k.z⟩

/-- An orthonormal right-handed frame whose first axis is `v0` (KusKabsch `frameOf`):
    `ref = y` if `|v0.y| < 0.9` else `x`, `v1 = unit(v0 × ref)`, `v2 = unit(v0 × v1)`. The 0.9
    cut keeps `ref` at least ~26° from `v0`, so the cross product is well conditioned. -/
def frameOf (v0 : V3) : V3 × V3 × V3 :=
  let ref : V3 := if Float.abs v0.y < 0.9 then ⟨0, 1, 0⟩ else ⟨1, 0, 0⟩
  let v1 := unitK (cross v0 ref)
  let v2 := unitK (cross v0 v1)
  (v0, v1, v2)

/-- Godot's `Vector3::get_any_perpendicular()` direction choice: cross with x when x is the
    smallest component, else with y; normalised. -/
def anyPerp (u0 : V3) : V3 :=
  let px := Float.abs u0.x <= Float.abs u0.y && Float.abs u0.x <= Float.abs u0.z
  let pr : V3 := if px then ⟨1, 0, 0⟩ else ⟨0, 1, 0⟩
  unitK (cross u0 pr)

/-- The canonical completion `(c1, c2) = (R0 v1, R0 v2)`, `R0` the shortest arc `v0 → u0`. -/
def canonCompletion (v0 u0 v1 v2 : V3) : V3 × V3 :=
  let ax := cross v0 u0
  let sn := Float.sqrt (dot ax ax)
  let cs := dot v0 u0
  let m := max sn 1e-9
  let k : V3 := ⟨ax.x / m, ax.y / m, ax.z / m⟩
  if sn < 1e-9 then
    if cs > 0.0 then (v1, v2)
    else
      let p1 := anyPerp u0
      (p1, unitK (cross u0 p1))
  else (rod k cs sn v1, rod k cs sn v2)

/-- The planted wrong completion (a negative control): a fixed perpendicular frame of `u0`, not
    carried from V. It is a rotation and maps `v0` to `u0`, but leaves a twist about `u0`. -/
def anyCompletion (u0 : V3) : V3 × V3 :=
  let (_, p1, p2) := frameOf u0
  (p1, p2)

/-- `R = U Vᵀ` with `U = [u0, c1, c2]`, `V = [v0, v1, v2]`; entry `(i, j)` is
    `u0_i v0_j + c1_i v1_j + c2_i v2_j`. -/
def uvt (u0 c1 c2 v0 v1 v2 : V3) : M3 :=
  let row (i : Nat) : V3 :=
    ⟨u0.get i * v0.x + c1.get i * v1.x + c2.get i * v2.x,
     u0.get i * v0.y + c1.get i * v1.y + c2.get i * v2.y,
     u0.get i * v0.z + c1.get i * v1.z + c2.get i * v2.z⟩
  ⟨row 0, row 1, row 2⟩

/-- The axis vector of `R`, `(R21 − R12, R02 − R20, R10 − R01)` = `2 sin θ · axis`. -/
def axisVec (r : M3) : V3 :=
  ⟨r.r2.y - r.r1.z, r.r0.z - r.r2.x, r.r1.x - r.r0.y⟩

/-- `axis(R) · u0`. Zero for the shortest arc `v0 → u0` (its axis is `v0 × u0`, normal to
    `u0`); nonzero when the completion adds a twist about `u0`. -/
def axisDot (r : M3) (u0 : V3) : Float := dot (axisVec r) u0

end RotationStar

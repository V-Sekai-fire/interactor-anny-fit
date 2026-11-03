-- SPDX-License-Identifier: MIT
import RotationStar.Completion

/-!
# `RotationStar.SwingTwist` — the swing-twist split of a rotation about an axis

The split the rank-1 completion gives: with `v0 = a` (the twist axis) and `u0 = R a / |R a|`,
the swing is the canonical completion's `R0 = U Vᵀ` (`U = [u0, R0 v1, R0 v2]`,
`V = [v0, v1, v2] = frameOf v0`), the shortest arc `a → R a`; the twist is `T = R0ᵀ R`, a
rotation about `a` alone, and `R = R0 T`. The angle is read off `T` in the plane normal to `a`.

This is the standard twist (`2·atan2(q_a, w)` of the rotation's quaternion), from 3×3s only.
It is the computation contract-anny-kernels' `SomaCommon.swingTwistAngle` emits for the GPU,
step for step, which `RotationStar.Slang.SwingTwist` now owns.
-/

namespace RotationStar
open V3

/-- `T = Aᵀ B`, entry `(i, j) = A0i B0j + A1i B1j + A2i B2j` (the kernels' `matTMul`). -/
def tMul (a b : M3) : M3 :=
  let e (i j : Nat) : Float := a.get 0 i * b.get 0 j + a.get 1 i * b.get 1 j + a.get 2 i * b.get 2 j
  ⟨⟨e 0 0, e 0 1, e 0 2⟩, ⟨e 1 0, e 1 1, e 1 2⟩, ⟨e 2 0, e 2 1, e 2 2⟩⟩

/-- The swing `R0` of `r` about the unit vector `a`: the canonical completion of
    `v0 = a ↦ u0 = unit(r a)`. -/
def swingAbout (r : M3) (a : V3) : M3 :=
  let (v0, v1, v2) := frameOf a
  let u0 := unitK (r.mul v0)
  let (c1, c2) := canonCompletion v0 u0 v1 v2
  uvt u0 c1 c2 v0 v1 v2

/-- The twist angle of `t` (a rotation about the coordinate axis `ax`, 0 = x) as the kernels
    read it: `atan2(T21 − T12, T11 + T22)` for x, and cyclically. -/
def twistAngleAx (t : M3) (ax : Nat) : Float :=
  if ax == 0 then Float.atan2 (t.r2.y - t.r1.z) (t.r1.y + t.r2.z)
  else if ax == 1 then Float.atan2 (t.r0.z - t.r2.x) (t.r2.z + t.r0.x)
  else Float.atan2 (t.r1.x - t.r0.y) (t.r0.x + t.r1.y)

/-- The twist angle of `t` about the unit vector `a`: `atan2(a · axisVec t, tr t − aᵀ t a)`.
    For `t` a rotation by `θ` about `a`, `axisVec t = 2 sin θ a` and `tr t − aᵀ t a = 2 cos θ`. -/
def twistAngle (t : M3) (a : V3) : Float :=
  Float.atan2 (dot a (axisVec t)) (t.r0.x + t.r1.y + t.r2.z - dot a (t.mul a))

/-- The split of `r` about the coordinate axis `ax`: `(swing R0, twist T = R0ᵀ r, angle)`, with
    `r = R0 T`. The kernels' `swing_twist`. -/
def swingTwist (r : M3) (ax : Nat) : M3 × M3 × Float :=
  let r0 := swingAbout r (axisUnit ax)
  let t := tMul r0 r
  (r0, t, twistAngleAx t ax)

/-- The split about any unit vector `a`. -/
def swingTwistAbout (r : M3) (a : V3) : M3 × M3 × Float :=
  let r0 := swingAbout r a
  let t := tMul r0 r
  (r0, t, twistAngle t a)

/-- The planted wrong swing (a negative control): `anyCompletion` instead of the canonical one.
    It still maps `a` to `unit(r a)` but carries a twist about it. -/
def swingAboutAny (r : M3) (a : V3) : M3 :=
  let (v0, v1, v2) := frameOf a
  let u0 := unitK (r.mul v0)
  let (c1, c2) := anyCompletion u0
  uvt u0 c1 c2 v0 v1 v2

end RotationStar

-- SPDX-License-Identifier: MIT
import Plausible
import RotationStar.Kabsch
import RotationStar.SwingTwist
import RotationStar.Align

/-!
# `RotationStar.Props` — the properties `rotation_star_test` checks with Plausible

Each property is a `Bool` predicate over a generated case. The positive ones must hold on every
case; each control is a planted wrong claim that must be refuted (Plausible must find a
counterexample), so a test harness that cannot see a difference shows up as a failed control.

`canonOK` and `anyHasMinimalTwist` are swing-twist-kusudama `KusKabsch.lean`'s two checks,
moved here with the same cases (elevation and azimuth in degrees) and tolerances.

Test rotations are made from the 6D pair (two generated vectors, Gram–Schmidt), and twists and
swings for the split tests by Rodrigues about a generated axis: test data only, not solver math.
-/

namespace RotationStar.Props
open RotationStar V3
open Plausible (Gen Arbitrary Shrinkable)

def pi : Float := 3.14159265358979323846
def d2r (x : Float) : Float := x * pi / 180.0
def sph (elev az : Float) : V3 :=
  ⟨Float.sin (d2r elev) * Float.cos (d2r az), Float.cos (d2r elev),
   Float.sin (d2r elev) * Float.sin (d2r az)⟩

/-! ## KusKabsch's cases -/

structure TCase where
  e0 : Nat
  a0 : Nat
  e1 : Nat
  a1 : Nat
deriving Repr

def v0Of (t : TCase) : V3 := unitK (sph (Float.ofNat t.e0) (Float.ofNat t.a0))
def u0Of (t : TCase) : V3 := unitK (sph (Float.ofNat t.e1) (Float.ofNat t.a1))

/-- Skip the near-aligned and near-antipodal cases, as KusKabsch does (the fallback branch is
    pinned separately in `Spec`). -/
def wellSeparated (t : TCase) : Bool :=
  let c := dot (v0Of t) (u0Of t)
  c < 0.98 && c > -0.98

/-- The canonical completion is orthonormal, maps `v0` to `u0`, and has no twist about `u0`.
    1e-3 is KusKabsch's tolerance. -/
def canonOK (t : TCase) : Bool :=
  if !wellSeparated t then true else
  let v0 := v0Of t
  let u0 := u0Of t
  let (_, v1, v2) := frameOf v0
  let (c1, c2) := canonCompletion v0 u0 v1 v2
  let orthonormal :=
    Float.abs (norm c1 - 1.0) < 1e-3 && Float.abs (norm c2 - 1.0) < 1e-3 &&
    Float.abs (dot u0 c1) < 1e-3 && Float.abs (dot u0 c2) < 1e-3 && Float.abs (dot c1 c2) < 1e-3
  let r := uvt u0 c1 c2 v0 v1 v2
  let mapsV0 := norm ((r.mul v0).sub u0) < 1e-3
  orthonormal && mapsV0 && Float.abs (axisDot r u0) < 1e-3

/-- Control: the get_any_perpendicular-style completion claims no twist. Must be refuted. -/
def anyHasMinimalTwist (t : TCase) : Bool :=
  if !wellSeparated t then true else
  let v0 := v0Of t
  let u0 := u0Of t
  let (_, v1, v2) := frameOf v0
  let (c1, c2) := anyCompletion u0
  Float.abs (axisDot (uvt u0 c1 c2 v0 v1 v2) u0) < 1e-3

instance : Shrinkable TCase := ⟨fun _ => []⟩
instance : Arbitrary TCase where
  arbitrary := do
    let e0 ← Gen.chooseNatLt 5 175 (by omega)
    let a0 ← Gen.chooseNatLt 0 360 (by omega)
    let e1 ← Gen.chooseNatLt 5 175 (by omega)
    let a1 ← Gen.chooseNatLt 0 360 (by omega)
    pure { e0 := e0.val, a0 := a0.val, e1 := e1.val, a1 := a1.val }

/-! ## Frames, stars and splits -/

/-- A generated case: two 6D vectors `a`, `b` (a frame), a second pair `c`, `d` (another frame),
    two origins, an axis index, a twist angle and a swing angle in degrees. Integers in
    thousandths keep the generator simple and the values spread over [−1, 1]. -/
structure RCase where
  a : Array Int
  ax : Nat
  twist : Nat
  swing : Nat
deriving Repr

def comp (c : RCase) (i : Nat) : Float := Float.ofInt (c.a[i]!) / 1000.0
def vecAt (c : RCase) (i : Nat) : V3 := ⟨comp c i, comp c (i+1), comp c (i+2)⟩
def frameA (c : RCase) : M3 := M3.of6D (vecAt c 0) (vecAt c 3)
def frameB (c : RCase) : M3 := M3.of6D (vecAt c 6) (vecAt c 9)
def originA (c : RCase) : V3 := (vecAt c 12).scale 10.0
def originB (c : RCase) : V3 := (vecAt c 15).scale 10.0

/-- The 6D pairs are well conditioned: both vectors longer than 0.1 and at least ~6° apart, so
    Gram–Schmidt gives a frame and the generated case is a real rotation. -/
def wellFormed (c : RCase) : Bool :=
  let ok (a b : V3) := norm a > 0.1 && norm b > 0.1 && norm (cross a b) > 0.1 * norm a * norm b
  ok (vecAt c 0) (vecAt c 3) && ok (vecAt c 6) (vecAt c 9)

instance : Shrinkable RCase := ⟨fun _ => []⟩
instance : Arbitrary RCase where
  arbitrary := do
    let mut a : Array Int := #[]
    for _ in [0:18] do
      let k ← Gen.chooseNatLt 0 2001 (by omega)
      a := a.push (Int.ofNat k.val - 1000)
    let ax ← Gen.chooseNatLt 0 3 (by omega)
    let tw ← Gen.chooseNatLt 0 340 (by omega)
    let sw ← Gen.chooseNatLt 1 170 (by omega)
    pure { a := a, ax := ax.val, twist := tw.val, swing := sw.val }

/-- 1e-12 is ~4500 ulps of 1.0: far above the rounding of the few dozen double operations in a
    solve, far below the 0.05 perturbation the control plants. -/
def exactTol : Float := 1e-12

/-- The star of a frame recovers the frame against the identity star. -/
def starRecovers (c : RCase) : Bool :=
  if !wellFormed c then true else
  let r := frameA c
  (starKabsch (star r (originA c)) (star M3.id (originB c))).maxDiff r < exactTol

/-- Two stars give the relative rotation `R Qᵀ`. -/
def starRelative (c : RCase) : Bool :=
  if !wellFormed c then true else
  let r := frameA c
  let q := frameB c
  (starKabsch (star r (originA c)) (star q (originB c))).maxDiff (r.mulM q.transpose) < exactTol

/-- Control: a star with one arm end moved by 0.05 still claims exact recovery. Must be refuted. -/
def perturbedStarRecovers (c : RCase) : Bool :=
  if !wellFormed c then true else
  let r := frameA c
  let s := star r (originA c)
  let bent := { s with px := s.px.add ⟨0.05, 0.0, 0.0⟩ }
  (starKabsch bent (star M3.id (originB c))).maxDiff r < exactTol

/-- The degenerate star (only the ±x arms; the others at the origin) pins one direction: the
    covariance has rank 1, and Kabsch returns the shortest arc `x → R x`: it maps x to `R x`,
    is a rotation, and has no twist about `R x`. -/
def rank1Minimal (c : RCase) : Bool :=
  if !wellFormed c then true else
  let r := frameA c
  let o := originB c
  let src : Star := { star M3.id o with py := o, ny := o, pz := o, nz := o }
  let k := starKabsch (star r (originA c)) src
  let u0 := r.c0
  let mapsX := norm ((k.mul V3.ex).sub u0) < exactTol
  let rot := (k.mulM k.transpose).maxDiff M3.id < exactTol && Float.abs (k.det - 1.0) < exactTol
  -- Antipodal x → −x has a free twist; KusKabsch skips it, so does this check.
  let antipodal := dot V3.ex u0 < -0.98
  antipodal || (mapsX && rot && Float.abs (axisDot k u0) < exactTol)

/-- Control: the same degenerate star, with the rank-1 completion swapped for `anyCompletion`,
    claims no twist. Must be refuted. -/
def rank1AnyMinimal (c : RCase) : Bool :=
  if !wellFormed c then true else
  let r := frameA c
  let u0 := r.c0
  let (_, v1, v2) := frameOf V3.ex
  let (c1, c2) := anyCompletion u0
  let k := uvt u0 c1 c2 V3.ex v1 v2
  dot V3.ex u0 < -0.98 || Float.abs (axisDot k u0) < exactTol

/-- `R = S T` built from a known swing `S` (by `swing` degrees about an axis normal to `a`) and a
    known twist `T` (by `twist − 170` degrees about `a`): the split returns `S`, `T` and the
    angle, and recomposes `R`. -/
def splitRecovers (c : RCase) : Bool :=
  if !wellFormed c then true else
  let a := axisUnit c.ax
  let n := unitK (cross a (frameA c).c0)
  if norm (cross a (frameA c).c0) < 0.1 then true else
  let th := d2r (Float.ofNat c.twist - 170.0)
  let ph := d2r (Float.ofNat c.swing)
  let rotAbout (k : V3) (ang : Float) : M3 :=
    M3.ofCols (rod k (Float.cos ang) (Float.sin ang) V3.ex)
      (rod k (Float.cos ang) (Float.sin ang) V3.ey) (rod k (Float.cos ang) (Float.sin ang) V3.ez)
  let s := rotAbout n ph
  let t := rotAbout a th
  let r := s.mulM t
  let (r0, t', ang) := swingTwist r c.ax
  r0.maxDiff s < 1e-9 && t'.maxDiff t < 1e-9 && Float.abs (ang - th) < 1e-9 &&
    (r0.mulM t').maxDiff r < 1e-9 && norm ((t'.mul a).sub a) < 1e-9

/-- Control: the split with the planted wrong swing claims a pure twist about `a`. Must be
    refuted. -/
def splitAnyPure (c : RCase) : Bool :=
  if !wellFormed c then true else
  let r := frameA c
  let a := axisUnit c.ax
  let t := tMul (swingAboutAny r a) r
  norm ((t.mul a).sub a) < 1e-9 && Float.abs (axisDot (swingAboutAny r a) (unitK (r.mul a))) < 1e-9

/-- A generated 3×3 (the case's first nine entries): `sweeps` one-sided Jacobi sweeps reach the
    same result, bit for bit, as 40 (ik_kabsch_6d.cpp's cap). -/
def svdConverged (c : RCase) : Bool :=
  let h := M3.ofArr ((List.range 9).toArray.map (comp c))
  let (u, v, s) := svd3With sweeps h
  let (u', v', s') := svd3With 40 h
  u.maxDiff u' == 0.0 && v.maxDiff v' == 0.0 && s.x == s'.x && s.y == s'.y && s.z == s'.z

/-- Control: one sweep claims the same. Must be refuted, or `svdConverged` proves nothing. -/
def svdOneSweep (c : RCase) : Bool :=
  let h := M3.ofArr ((List.range 9).toArray.map (comp c))
  let (u, _, _) := svd3With 1 h
  let (u', _, _) := svd3With 40 h
  u.maxDiff u' == 0.0

/-- sinew's align recipe recovers the frame from 5 vector pairs to 1e-4, its own oracle's
    tolerance. -/
def alignRecovers (c : RCase) : Bool :=
  if !wellFormed c then true else
  let r := frameA c
  let srcs : Array V3 := #[⟨1,0,0⟩, ⟨0,1,0⟩, ⟨0,0,1⟩, ⟨1,1,0⟩, ⟨0.3,-0.7,0.5⟩]
  let rr := M3.ofArr (Align.align (srcs.map fun b => (r.mul b, b)))
  rr.maxDiff r < 1e-4

end RotationStar.Props

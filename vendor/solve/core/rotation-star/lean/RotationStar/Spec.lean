-- SPDX-License-Identifier: MIT
import RotationStar.Kabsch
import RotationStar.SwingTwist

/-!
# `RotationStar.Spec` — the cheap proofs

Two kinds, both checked by `lake build`:

* Exact algebra over `Int` (any integer frame, no rounding): the star's points sum to 7 times
  its origin, so its centroid is the origin; and the cross-covariance of the star of `R` against
  the star of `Q` is exactly `2 R Qᵀ`. For orthonormal `R`, `Q` that matrix is twice a rotation,
  so its SVD is `U = R Qᵀ`, `Σ = 2 I`, `V = I` and Kabsch returns `R Qᵀ`: the star of a frame
  recovers the frame, on exact data, by construction rather than by fitting.
* `native_decide` pins of the `Float` solver on the 24 proper signed-permutation frames, whose
  entries are exact in binary: Kabsch on the stars returns each frame bit for bit, and the
  swing-twist split about each axis recomposes each frame bit for bit.
-/

namespace RotationStar.Spec

@[ext] structure IV3 where
  x : Int
  y : Int
  z : Int
deriving DecidableEq, Repr

@[ext] structure IM3 where
  r0 : IV3
  r1 : IV3
  r2 : IV3
deriving DecidableEq, Repr

namespace IV3
def add (a b : IV3) : IV3 := ⟨a.x + b.x, a.y + b.y, a.z + b.z⟩
def sub (a b : IV3) : IV3 := ⟨a.x - b.x, a.y - b.y, a.z - b.z⟩
def dot (a b : IV3) : Int := a.x * b.x + a.y * b.y + a.z * b.z
def zero : IV3 := ⟨0, 0, 0⟩
end IV3

namespace IM3
def c0 (m : IM3) : IV3 := ⟨m.r0.x, m.r1.x, m.r2.x⟩
def c1 (m : IM3) : IV3 := ⟨m.r0.y, m.r1.y, m.r2.y⟩
def c2 (m : IM3) : IV3 := ⟨m.r0.z, m.r1.z, m.r2.z⟩
def add (a b : IM3) : IM3 := ⟨a.r0.add b.r0, a.r1.add b.r1, a.r2.add b.r2⟩
def zero : IM3 := ⟨IV3.zero, IV3.zero, IV3.zero⟩
/-- `a bᵀ`: entry `(i, j)` is row i of `a` dot row j of `b`. -/
def mulT (a b : IM3) : IM3 :=
  ⟨⟨a.r0.dot b.r0, a.r0.dot b.r1, a.r0.dot b.r2⟩,
   ⟨a.r1.dot b.r0, a.r1.dot b.r1, a.r1.dot b.r2⟩,
   ⟨a.r2.dot b.r0, a.r2.dot b.r1, a.r2.dot b.r2⟩⟩
end IM3

/-- The outer product `p qᵀ`. -/
def outer (p q : IV3) : IM3 :=
  ⟨⟨p.x * q.x, p.x * q.y, p.x * q.z⟩,
   ⟨p.y * q.x, p.y * q.y, p.y * q.z⟩,
   ⟨p.z * q.x, p.z * q.y, p.z * q.z⟩⟩

/-- The star of the integer frame `r` at `o` (the same 7 points as `RotationStar.star`). -/
def istar (r : IM3) (o : IV3) : List IV3 :=
  [o, o.add r.c0, o.sub r.c0, o.add r.c1, o.sub r.c1, o.add r.c2, o.sub r.c2]

def vsum (ps : List IV3) : IV3 := ps.foldr IV3.add IV3.zero

/-- `Σ (aᵢ − oa)(bᵢ − ob)ᵀ`. -/
def covAbout (a b : List IV3) (oa ob : IV3) : IM3 :=
  (List.zipWith (fun p q => outer (p.sub oa) (q.sub ob)) a b).foldr IM3.add IM3.zero

/-- The star's points sum to 7·o: its centroid is its origin. -/
theorem star_sum (r : IM3) (o : IV3) : vsum (istar r o) = ⟨7 * o.x, 7 * o.y, 7 * o.z⟩ := by
  obtain ⟨⟨a, b, c⟩, ⟨d, e, f⟩, ⟨g, h, i⟩⟩ := r
  obtain ⟨x, y, z⟩ := o
  simp only [vsum, istar, List.foldr, IV3.add, IV3.sub, IV3.zero, IM3.c0, IM3.c1, IM3.c2,
    IV3.mk.injEq]
  omega

/-- The star of `R` against the star of `Q`, each about its origin, has cross-covariance
    exactly `2 R Qᵀ`. -/
theorem star_cov (r q : IM3) (oa ob : IV3) :
    covAbout (istar r oa) (istar q ob) oa ob = (r.mulT q).add (r.mulT q) := by
  obtain ⟨⟨a, b, c⟩, ⟨d, e, f⟩, ⟨g, h, i⟩⟩ := r
  obtain ⟨⟨a', b', c'⟩, ⟨d', e', f'⟩, ⟨g', h', i'⟩⟩ := q
  obtain ⟨x, y, z⟩ := oa
  obtain ⟨x', y', z'⟩ := ob
  simp only [covAbout, istar, List.zipWith, List.foldr, outer, IV3.add, IV3.sub, IV3.zero,
    IM3.add, IM3.zero, IM3.c0, IM3.c1, IM3.c2, IM3.mulT, IV3.dot, IM3.mk.injEq, IV3.mk.injEq,
    Int.sub_self, Int.mul_zero]
  have ha : ∀ u v : Int, u + v - u = v := by intro u v; omega
  have hs : ∀ u v : Int, u - v - u = -v := by intro u v; omega
  simp only [ha, hs, Int.neg_mul_neg]
  refine ⟨⟨?_, ?_, ?_⟩, ⟨?_, ?_, ?_⟩, ⟨?_, ?_, ?_⟩⟩ <;> omega

/-- So the star of a frame against the identity star is exactly twice the frame. -/
theorem star_cov_id (r : IM3) (oa ob : IV3) :
    covAbout (istar r oa) (istar ⟨⟨1, 0, 0⟩, ⟨0, 1, 0⟩, ⟨0, 0, 1⟩⟩ ob) oa ob = r.add r := by
  rw [star_cov]
  obtain ⟨⟨a, b, c⟩, ⟨d, e, f⟩, ⟨g, h, i⟩⟩ := r
  simp only [IM3.mulT, IV3.dot, IM3.add, IV3.add, IM3.mk.injEq, IV3.mk.injEq]
  omega

/-! ## Pins of the `Float` solver on the signed-permutation frames -/

/-- The 24 proper rotations with entries in {-1, 0, 1}: each permutation of the axes with each
    sign pattern, kept when the determinant is +1. -/
def signedPerms : List M3 := Id.run do
  let perms : List (Nat × Nat × Nat) := [(0,1,2), (0,2,1), (1,0,2), (1,2,0), (2,0,1), (2,1,0)]
  let mut out := []
  for (p0, p1, p2) in perms do
    for s0 in [1.0, -1.0] do
      for s1 in [1.0, -1.0] do
        for s2 in [1.0, -1.0] do
          let r := M3.ofCols ((V3.axisUnit p0).scale s0) ((V3.axisUnit p1).scale s1)
            ((V3.axisUnit p2).scale s2)
          if r.det > 0.0 then out := out ++ [r]
  return out

/-- Exact equality of two `Float` matrices, entry by entry. -/
def same (a b : M3) : Bool := a.maxDiff b == 0.0

theorem signedPerms_count : signedPerms.length = 24 := by native_decide

/-- Kabsch on the star of each frame (origin (3, −2, 5)) against the identity star (origin
    (−1, 4, 2)) returns the frame exactly. Integer origins keep the centroids exact. -/
theorem star_kabsch_exact :
    signedPerms.all (fun r => same (starKabsch (star r ⟨3, -2, 5⟩) (star M3.id ⟨-1, 4, 2⟩)) r)
      = true := by native_decide

/-- Swing then twist recomposes each frame exactly, about each coordinate axis. -/
theorem swing_twist_recompose_exact :
    signedPerms.all (fun r => [0, 1, 2].all (fun ax =>
      let (r0, t, _) := swingTwist r ax
      same (r0.mulM t) r)) = true := by native_decide

end RotationStar.Spec

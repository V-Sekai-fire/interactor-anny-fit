-- SPDX-License-Identifier: MIT
import RotationStar.Completion

/-!
# `RotationStar.Kabsch` — the rotational star and its Kabsch fit

A frame is a star of 7 points: its origin and the 6 unit-axis ends `o ± R e_x`, `o ± R e_y`,
`o ± R e_z`. Two stars are matched as point sets by Kabsch: the cross-covariance
`H = Σ (aᵢ − ā)(bᵢ − b̄)ᵀ` of target `a` against source `b`, its SVD `H = U Σ Vᵀ`, and
`R = U diag(1, 1, d) Vᵀ` with `d = sign det(U Vᵀ)`, so that `aᵢ − ā ≈ R (bᵢ − b̄)`.

The SVD is entities-godot `ik_kabsch_6d.cpp`'s `svd3`: one-sided (Hestenes) Jacobi on the
columns of `H`, no trigonometry, then the rank completions — rank 1 by the canonical
minimal-twist completion (`RotationStar.Completion`), rank 2 by `u2 = u0 × u1`.
-/

namespace RotationStar
open V3

/-- The 7 points of a frame. -/
structure Star where
  o  : V3
  px : V3
  nx : V3
  py : V3
  ny : V3
  pz : V3
  nz : V3
deriving Repr, Inhabited

/-- The star of the frame `R` (its columns are the axes) at origin `o`. -/
def star (r : M3) (o : V3) : Star :=
  ⟨o, o.add r.c0, o.sub r.c0, o.add r.c1, o.sub r.c1, o.add r.c2, o.sub r.c2⟩

def Star.points (s : Star) : Array V3 := #[s.o, s.px, s.nx, s.py, s.ny, s.pz, s.nz]

def Star.ofPoints (a : Array V3) : Star :=
  ⟨a[0]!, a[1]!, a[2]!, a[3]!, a[4]!, a[5]!, a[6]!⟩

/-- The mean of the points, summed in order then divided by the count. -/
def centroid (ps : Array V3) : V3 :=
  let s := ps.foldl V3.add V3.zero
  let n := Float.ofNat ps.size
  ⟨s.x / n, s.y / n, s.z / n⟩

/-- `H = Σ (aᵢ − ā)(bᵢ − b̄)ᵀ`, `a` the target and `b` the source, summed in point order. -/
def cov (a b : Array V3) : M3 := Id.run do
  let ca := centroid a
  let cb := centroid b
  let mut h : Array Float := Array.replicate 9 0.0
  for idx in [0:min a.size b.size] do
    let p := a[idx]!.sub ca
    let q := b[idx]!.sub cb
    for i in [0:3] do
      for j in [0:3] do
        h := h.set! (i*3+j) (h[i*3+j]! + p.get i * q.get j)
  return M3.ofArr h

/-- Sweeps of the one-sided Jacobi. Each sweep rotates the three column pairs, and 3×3 Jacobi
    converges quadratically: over 3000 generated matrices the double result stopped changing
    after at most 4 sweeps. 8 is twice that; `rotation_star_test` checks on its own sample that
    8 sweeps give the same bits as ik_kabsch_6d.cpp's cap of 40. -/
def sweeps : Nat := 8

/-- Columns `p`, `q` count as orthogonal when `|γ| ≤ orthTol·√(αβ)`: DBL_EPSILON (2⁻⁵²), the
    working precision of the double spec. The float kernels use FLT_EPSILON. -/
def orthTol : Float := 2.220446049250313e-16

/-- A singular value at or below `rankTol · σ0` counts as zero. The float kernels round at 2⁻²⁴
    ≈ 6e-8; 1e-6 is ~16 of those, so a column this small is rounding noise in the kernel, and
    the double spec uses the same cut so every path agrees on the rank. -/
def rankTol : Float := 1e-6

/-- `H = U Σ Vᵀ` by one-sided Jacobi (ik_kabsch_6d.cpp `svd3`). Returns `(U, V, σ)`, `σ`
    descending, `U` completed to an orthonormal frame where `H` is rank deficient. -/
def svd3With (nSweeps : Nat) (hm : M3) : M3 × M3 × V3 := Id.run do
  let mut a := hm.toArr
  let mut v := M3.id.toArr
  for _ in [0:nSweeps] do
    for (p, q) in [(0, 1), (0, 2), (1, 2)] do
      let alpha := a[p]! * a[p]! + a[3+p]! * a[3+p]! + a[6+p]! * a[6+p]!
      let beta := a[q]! * a[q]! + a[3+q]! * a[3+q]! + a[6+q]! * a[6+q]!
      let gamma := a[p]! * a[q]! + a[3+p]! * a[3+q]! + a[6+p]! * a[6+q]!
      if Float.abs gamma > orthTol * Float.sqrt (alpha * beta) then
        let zeta := (beta - alpha) / (2.0 * gamma)
        let t := (if zeta >= 0.0 then 1.0 else -1.0) / (Float.abs zeta + Float.sqrt (1.0 + zeta * zeta))
        let c := 1.0 / Float.sqrt (1.0 + t * t)
        let s := c * t
        for k in [0:3] do
          let ap := a[k*3+p]!; let aq := a[k*3+q]!
          a := a.set! (k*3+p) (c * ap - s * aq)
          a := a.set! (k*3+q) (s * ap + c * aq)
          let vp := v[k*3+p]!; let vq := v[k*3+q]!
          v := v.set! (k*3+p) (c * vp - s * vq)
          v := v.set! (k*3+q) (s * vp + c * vq)
  let sig0 := Float.sqrt (a[0]! * a[0]! + a[3]! * a[3]! + a[6]! * a[6]!)
  let sig1 := Float.sqrt (a[1]! * a[1]! + a[4]! * a[4]! + a[7]! * a[7]!)
  let sig2 := Float.sqrt (a[2]! * a[2]! + a[5]! * a[5]! + a[8]! * a[8]!)
  let sig := #[sig0, sig1, sig2]
  let mut ord := #[0, 1, 2]
  for (i, j) in [(0, 1), (0, 2), (1, 2)] do
    if sig[ord[j]!]! > sig[ord[i]!]! then
      let t := ord[i]!; ord := ord.set! i ord[j]!; ord := ord.set! j t
  let col (m : Array Float) (c : Nat) : V3 := ⟨m[c]!, m[3+c]!, m[6+c]!⟩
  let s0 := sig[ord[0]!]!; let s1 := sig[ord[1]!]!; let s2 := sig[ord[2]!]!
  let eps := rankTol * s0
  let ucol (c : Nat) (sc : Float) : V3 :=
    if sc > eps then let w := col a c; ⟨w.x / sc, w.y / sc, w.z / sc⟩ else V3.zero
  let v0 := col v ord[0]!
  let mut v1 := col v ord[1]!
  let mut v2 := col v ord[2]!
  let mut u0 := ucol ord[0]! s0
  let mut u1 := ucol ord[1]! s1
  let mut u2 := ucol ord[2]! s2
  if s0 <= eps then
    u0 := V3.ex; u1 := V3.ey; u2 := V3.ez
  else if s1 <= eps then
    -- Rank 1: V's columns 1 and 2 are any basis of the null space v0⊥, which Jacobi leaves
    -- at the mercy of rounding. Replace them with `frameOf v0`, so the result depends on the
    -- pinned pair (u0, v0) alone: the shortest arc as before, and in the antipodal case (where
    -- the twist is truly free) the same pick on every target, float or double, CPU or GPU.
    let (_, f1, f2) := frameOf v0
    let (c1, c2) := canonCompletion v0 u0 f1 f2
    u1 := c1; u2 := c2; v1 := f1; v2 := f2
  else if s2 <= eps then
    u2 := unitK (cross u0 u1)
  return (M3.ofCols u0 u1 u2, M3.ofCols v0 v1 v2, ⟨s0, s1, s2⟩)

/-- `svd3With sweeps`. -/
def svd3 (hm : M3) : M3 × M3 × V3 := svd3With sweeps hm

/-- `R = U diag(1, 1, d) Vᵀ`, `d = sign det(U Vᵀ)`: the proper rotation nearest `H`. -/
def kabsch (hm : M3) : M3 :=
  let (u, v, _) := svd3 hm
  let uc0 := u.c0; let uc1 := u.c1; let uc2 := u.c2
  let vc0 := v.c0; let vc1 := v.c1; let vc2 := v.c2
  let d := if (uvt uc0 uc1 uc2 vc0 vc1 vc2).det >= 0.0 then 1.0 else -1.0
  uvt uc0 uc1 (uc2.scale d) vc0 vc1 vc2

/-- The rotation taking the source points `b` onto the target points `a` (`aᵢ − ā ≈ R (bᵢ − b̄)`). -/
def fit (a b : Array V3) : M3 := kabsch (cov a b)

/-- Kabsch on two stars. -/
def starKabsch (tgt src : Star) : M3 := fit tgt.points src.points

/-- The frame of a star: its rotation against the identity star at its own origin. -/
def Star.frame (s : Star) : M3 := starKabsch s (star M3.id s.o)

end RotationStar

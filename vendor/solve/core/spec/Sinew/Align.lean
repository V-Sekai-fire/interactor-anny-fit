-- SPDX-License-Identifier: MIT
-- Copyright (c) 2026-present K. S. Ernest (iFire) Lee
--
-- Executable port of the "align two vector sets → rotation" recipe that
-- `spec/Sinew/SlangCodegen/Pheno.lean` (`skeletonFit`) emits as Slang and that the
-- SOMA skeleton fit runs per joint.  Same recipe, line for line: `rodrigues` for a
-- single pair, otherwise a covariance `H = Σ aᵢ bᵢᵀ` orthogonalised by `ns30`
-- (Newton-Schulz, 30 iters) with a `kabsch` (Jacobi-SVD) fallback when the result
-- is not a valid rotation.  `align` returns R with `aᵢ ≈ R bᵢ`.
import Sinew.Math

namespace Sinew.Align
open Sinew.Math

private def EPS : Float := 1e-8

/-- det of a row-major 3×3 stored as a 9-array. -/
def det9 (m : Array Float) : Float :=
  m[0]! * (m[4]! * m[8]! - m[5]! * m[7]!) - m[1]! * (m[3]! * m[8]! - m[5]! * m[6]!) +
    m[2]! * (m[3]! * m[7]! - m[4]! * m[6]!)

/-- Row-major 3×3 product. -/
def mul9 (A B : Array Float) : Array Float := Id.run do
  let mut C := Array.replicate 9 0.0
  for r in [0:3] do
    for c in [0:3] do
      C := C.set! (r*3+c) (A[r*3]! * B[c]! + A[r*3+1]! * B[3+c]! + A[r*3+2]! * B[6+c]!)
  return C

/-- Row-major 3×3 transpose. -/
def transpose9 (m : Array Float) : Array Float := Id.run do
  let mut o := Array.replicate 9 0.0
  for i in [0:3] do
    for j in [0:3] do
      o := o.set! (i*3+j) m[j*3+i]!
  return o

/-- Shortest-arc rotation taking `b` onto `a` (single vector pair). -/
def rodrigues (a b : V3) : Array Float := Id.run do
  let an := a.norm; let bn := b.norm
  let au := a.scale (1.0 / max an EPS)
  let bu := b.scale (1.0 / max bn EPS)
  let d := max (-1.0) (min 1.0 (V3.dot au bu))
  if d < -1.0 + 1e-6 then
    let w : V3 := if Float.abs bu.x > 0.6 then ⟨0,1,0⟩ else ⟨1,0,0⟩
    let x0 := V3.cross bu w
    let x := x0.scale (1.0 / x0.norm)
    let aa := #[x.x, x.y, x.z]
    let mut R := Array.replicate 9 0.0
    for r in [0:3] do
      for c in [0:3] do
        R := R.set! (r*3+c) (2.0 * aa[r]! * aa[c]! - (if r == c then 1.0 else 0.0))
    return R
  let v := V3.cross bu au
  let K := #[0.0, -v.z, v.y, v.z, 0.0, -v.x, -v.y, v.x, 0.0]
  let KK := mul9 K K
  let f := 1.0 / (1.0 + d)
  let mut R := Array.replicate 9 0.0
  for i in [0:9] do
    R := R.set! i ((if i % 4 == 0 then 1.0 else 0.0) + K[i]! + f * KK[i]!)
  return R

/-- Newton-Schulz orthogonalisation (30 iterations) with a determinant-sign fix. -/
def ns30 (H : Array Float) : Array Float := Id.run do
  let mut mrs := 0.0
  for r in [0:3] do
    mrs := max mrs (Float.abs H[r*3]! + Float.abs H[r*3+1]! + Float.abs H[r*3+2]!)
  let mut R := Array.replicate 9 0.0
  for i in [0:9] do
    R := R.set! i (H[i]! / (mrs + EPS))
  for _ in [0:30] do
    let mut RtR := Array.replicate 9 0.0
    for a in [0:3] do
      for b in [0:3] do
        RtR := RtR.set! (a*3+b) (R[a]! * R[b]! + R[3+a]! * R[3+b]! + R[6+a]! * R[6+b]!)
    let mut term := Array.replicate 9 0.0
    for k in [0:9] do
      term := term.set! k ((if k % 4 == 0 then 3.0 else 0.0) - RtR[k]!)
    let Rn := mul9 R term
    for k in [0:9] do
      R := R.set! k (Rn[k]! * 0.5)
  if det9 R < 0 then
    R := R.set! 2 (-R[2]!)
    R := R.set! 5 (-R[5]!)
    R := R.set! 8 (-R[8]!)
  return R

/-- Bump the diagonal of a nearly rank-deficient covariance toward full rank. -/
def regularize (H : Array Float) : Array Float := Id.run do
  let mut scale := 0.0
  for r in [0:3] do
    scale := max scale (Float.abs H[r*3]! + Float.abs H[r*3+1]! + Float.abs H[r*3+2]!)
  if scale < EPS then scale := EPS
  let vol := Float.abs (det9 H) / (scale * scale * scale)
  let rw := max 0.0 (min 1.0 ((1e-6 - vol) / 1e-6))
  let add := 0.05 * rw * scale
  let mut Ho := H
  Ho := Ho.set! 0 (Ho[0]! + add)
  Ho := Ho.set! 4 (Ho[4]! + add)
  Ho := Ho.set! 8 (Ho[8]! + add)
  return Ho

/-- A proper, near-orthonormal rotation? -/
def valid9 (R : Array Float) : Bool := Id.run do
  let d := det9 R
  if !(d > 0.0 && Float.abs (d - 1.0) <= 1e-2) then
    return false
  let mut e := 0.0
  for i in [0:3] do
    for j in [0:3] do
      let v := R[i]! * R[j]! + R[3+i]! * R[3+j]! + R[6+i]! * R[6+j]! - (if i == j then 1.0 else 0.0)
      e := max e (Float.abs v)
  return e <= 1e-2

/-- Symmetric 3×3 eigendecomposition by cyclic Jacobi; returns (eigenvalues, V). -/
def jacobi9 (Ain : Array Float) : Array Float × Array Float := Id.run do
  let mut A := Ain
  let mut V := #[1.0,0.0,0.0, 0.0,1.0,0.0, 0.0,0.0,1.0]
  let pp := #[0,0,1]; let qq := #[1,2,2]
  for _ in [0:50] do
    if Float.abs A[1]! + Float.abs A[2]! + Float.abs A[5]! < 1e-20 then
      break
    for k in [0:3] do
      let p := pp[k]!; let q := qq[k]!
      let apq := A[p*3+q]!
      if Float.abs apq < 1e-20 then
        continue
      let phi := 0.5 * Float.atan2 (2.0 * apq) (A[q*3+q]! - A[p*3+p]!)
      let c := Float.cos phi; let s := Float.sin phi
      for i in [0:3] do
        let aip := A[i*3+p]!; let aiq := A[i*3+q]!
        A := A.set! (i*3+p) (c*aip - s*aiq)
        A := A.set! (i*3+q) (s*aip + c*aiq)
      for i in [0:3] do
        let api := A[p*3+i]!; let aqi := A[q*3+i]!
        A := A.set! (p*3+i) (c*api - s*aqi)
        A := A.set! (q*3+i) (s*api + c*aqi)
      for i in [0:3] do
        let vip := V[i*3+p]!; let viq := V[i*3+q]!
        V := V.set! (i*3+p) (c*vip - s*viq)
        V := V.set! (i*3+q) (s*vip + c*viq)
  return (#[A[0]!, A[4]!, A[8]!], V)

/-- Kabsch rotation from a covariance `H` via the SVD of `HᵀH` (Jacobi). -/
def kabsch (H : Array Float) : Array Float := Id.run do
  let mut S := Array.replicate 9 0.0
  for i in [0:3] do
    for j in [0:3] do
      S := S.set! (i*3+j) (H[i]! * H[j]! + H[3+i]! * H[3+j]! + H[6+i]! * H[6+j]!)
  let (w, Vc) := jacobi9 S
  let mut ord := #[0,1,2]
  for a in [0:3] do
    for b in [a+1:3] do
      if w[ord[b]!]! > w[ord[a]!]! then
        let t := ord[a]!; ord := ord.set! a ord[b]!; ord := ord.set! b t
  let mut V := Array.replicate 9 0.0
  let mut sig := Array.replicate 3 0.0
  for c in [0:3] do
    sig := sig.set! c (Float.sqrt (max w[ord[c]!]! 0.0))
    for r in [0:3] do
      V := V.set! (r*3+c) Vc[r*3+ord[c]!]!
  let mut U := Array.replicate 9 0.0
  for c in [0:3] do
    let hvx := H[0]! * V[c]! + H[1]! * V[3+c]! + H[2]! * V[6+c]!
    let hvy := H[3]! * V[c]! + H[4]! * V[3+c]! + H[5]! * V[6+c]!
    let hvz := H[6]! * V[c]! + H[7]! * V[3+c]! + H[8]! * V[6+c]!
    let sc := if sig[c]! > 1e-12 then 1.0 / sig[c]! else 0.0
    U := U.set! c (hvx*sc); U := U.set! (3+c) (hvy*sc); U := U.set! (6+c) (hvz*sc)
  for c in [0:3] do
    if sig[c]! <= 1e-12 then
      let a := (c+1) % 3; let b := (c+2) % 3
      let ua : V3 := ⟨U[a]!, U[3+a]!, U[6+a]!⟩
      let ub : V3 := ⟨U[b]!, U[3+b]!, U[6+b]!⟩
      let uc := V3.cross ua ub
      U := U.set! c uc.x; U := U.set! (3+c) uc.y; U := U.set! (6+c) uc.z
  let Vt := transpose9 V
  let UVt := mul9 U Vt
  let sgn := if det9 UVt < 0 then -1.0 else 1.0
  let mut Ud := U
  Ud := Ud.set! 2 (Ud[2]! * sgn); Ud := Ud.set! 5 (Ud[5]! * sgn); Ud := Ud.set! 8 (Ud[8]! * sgn)
  return mul9 Ud Vt

/-- The `finishAlign` recipe: rodrigues for one pair, else regularise → `ns30`,
    with a `kabsch` fallback.  `H = Σ aᵢ bᵢᵀ`; `(a0,b0)`/`(a1,b1)` are the first pairs. -/
def finishAlign (Hin : Array Float) (N : Nat) (a0 a1 b0 b1 : V3) : Array Float := Id.run do
  if N == 1 then
    return rodrigues a0 b0
  let mut H := Hin
  let ns := V3.cross a0 a1; let nd := V3.cross b0 b1
  let ln := ns.norm; let ld := nd.norm
  if ln > 1e-9 && ld > 1e-9 then
    let vs := ns.scale (a0.norm / (ln + EPS))
    let vd := nd.scale (b0.norm / (ld + EPS))
    let vsa := #[vs.x, vs.y, vs.z]; let vda := #[vd.x, vd.y, vd.z]
    for i in [0:3] do
      for j in [0:3] do
        H := H.set! (i*3+j) (H[i*3+j]! + vsa[i]! * vda[j]!)
  H := regularize H
  let R := ns30 H
  if !valid9 R then
    return kabsch H
  return R

/-- Rotation R (row-major 3×3) with `aᵢ ≈ R bᵢ` over the given (target, source) pairs. -/
def align (pairs : Array (V3 × V3)) : Array Float := Id.run do
  let n := pairs.size
  let mut H := Array.replicate 9 0.0
  for pr in pairs do
    let a := pr.1; let b := pr.2
    let aa := #[a.x, a.y, a.z]; let bb := #[b.x, b.y, b.z]
    for i in [0:3] do
      for j in [0:3] do
        H := H.set! (i*3+j) (H[i*3+j]! + aa[i]! * bb[j]!)
  let z : V3 := ⟨0,0,0⟩
  let a0 := if n > 0 then pairs[0]!.1 else z
  let b0 := if n > 0 then pairs[0]!.2 else z
  let a1 := if n > 1 then pairs[1]!.1 else z
  let b1 := if n > 1 then pairs[1]!.2 else z
  return finishAlign H n a0 a1 b0 b1

/-- Apply a row-major 3×3 to a vector. -/
def apply9 (R : Array Float) (v : V3) : V3 :=
  ⟨R[0]! * v.x + R[1]! * v.y + R[2]! * v.z,
   R[3]! * v.x + R[4]! * v.y + R[5]! * v.z,
   R[6]! * v.x + R[7]! * v.y + R[8]! * v.z⟩

end Sinew.Align

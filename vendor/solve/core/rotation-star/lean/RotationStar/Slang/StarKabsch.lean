-- SPDX-License-Identifier: MIT
import RotationStar.Slang.Split
import RotationStar.Kabsch

/-!
# `RotationStar.Slang.StarKabsch` — the star Kabsch fit, one thread per pair of stars

`RotationStar.Kabsch.starKabsch` in Slang: centroids, the cross-covariance `H`, the one-sided
Jacobi SVD (`sweeps` sweeps, the spec's count), the sort, the rank-1 minimal-twist completion
and the rank-2 cross completion, the reflection fix. Float32 constants:

* `orthTol` = FLT_EPSILON (2⁻²³, exact as `1.1920929e-7f`): float working precision, where the
  double spec uses DBL_EPSILON.
* `rankTol` = 1e-6, the spec's cut (~16 float roundings of `σ0`).

The GPU does not round every division as the CPU does: on the M2 Pro a collapsed star arm left
~1e-13 in a covariance column that is exactly 0 on the CPU, and Jacobi then turned V's
null-space columns by an arbitrary angle. The rank-1 branch therefore takes them
from `frameOf v0` (as the spec does), never from Jacobi.

Bindings (set 0):

   0  ConstantBuffer<StarKabschParams> { uint N; }
   1  StructuredBuffer<float>   tgt  (N·21)  7 points × xyz: o, +x, −x, +y, −y, +z, −z
   2  StructuredBuffer<float>   src  (N·21)
   3  RWStructuredBuffer<float> rot  (N·9)   row-major R with tgt − t̄ ≈ R (src − s̄)
-/

namespace RotationStar.Slang.StarKabsch

open LeanSlang
open RotationStar.Slang.Dsl
open RotationStar.Slang.Split

/-- FLT_EPSILON. -/
def orthTolF : Float := 1.1920928955078125e-7

/-- The 7 points of buffer `buf`'s star `i` into `<pfx>0 … <pfx>6` (each a 3-vector). -/
def loadStar (pfx buf : String) : List St :=
  (List.range 7).flatMap fun k =>
    vec (pfx ++ toString k ++ "_") (fun c => at_ buf (v "i" * u 21 + u (3 * k + c)))

/-- `<pfx>c` = the centroid of the star `<star>`. -/
def centroid (pfx star : String) : List St :=
  vec pfx fun c =>
    let pt := fun k => s (star ++ toString k ++ "_") c
    (pt 0 + pt 1 + pt 2 + pt 3 + pt 4 + pt 5 + pt 6) / fl 7.0

/-- One Jacobi rotation of columns `p`, `q` of `a` (and of `w`, the accumulated V). -/
def rotPair (pq : Nat × Nat) : List St :=
  let (p, q) := pq
  let n := fun x => x ++ toString p ++ toString q
  let col2 := fun (c d : Nat) => m "a" 0 c * m "a" 0 d + m "a" 1 c * m "a" 1 d + m "a" 2 c * m "a" 2 d
  [ let_ fT (n "al") (col2 p p), let_ fT (n "be") (col2 q q), let_ fT (n "ga") (col2 p q)
  , if_ (gt (fabs (v (n "ga"))) (flx orthTolF * call "sqrt" [v (n "al") * v (n "be")]))
      ([ let_ fT (n "ze") ((v (n "be") - v (n "al")) / (fl 2.0 * v (n "ga")))
       , let_ fT (n "t") (sel (ge (v (n "ze")) (fl 0.0)) (fl 1.0) (fl (-1.0)) /
           (fabs (v (n "ze")) + call "sqrt" [fl 1.0 + v (n "ze") * v (n "ze")]))
       , let_ fT (n "c") (fl 1.0 / call "sqrt" [fl 1.0 + v (n "t") * v (n "t")])
       , let_ fT (n "s") (v (n "c") * v (n "t")) ] ++
       r3.flatMap fun k =>
         let ap := s "a" (3 * k + p); let aq := s "a" (3 * k + q)
         let wp := s "w" (3 * k + p); let wq := s "w" (3 * k + q)
         [ let_ fT (n "ap" ++ toString k) ap, let_ fT (n "aq" ++ toString k) aq
         , setv ("a" ++ toString (3 * k + p)) (v (n "c") * v (n "ap" ++ toString k) - v (n "s") * v (n "aq" ++ toString k))
         , setv ("a" ++ toString (3 * k + q)) (v (n "s") * v (n "ap" ++ toString k) + v (n "c") * v (n "aq" ++ toString k))
         , let_ fT (n "wp" ++ toString k) wp, let_ fT (n "wq" ++ toString k) wq
         , setv ("w" ++ toString (3 * k + p)) (v (n "c") * v (n "wp" ++ toString k) - v (n "s") * v (n "wq" ++ toString k))
         , setv ("w" ++ toString (3 * k + q)) (v (n "s") * v (n "wp" ++ toString k) + v (n "c") * v (n "wq" ++ toString k)) ]) ]

/-- Swap slots `i`, `j` (σ, the column of `a`, the column of `w`) when `σj > σi`. -/
def sortPair (ij : Nat × Nat) : List St :=
  let (i, j) := ij
  let n := fun x => x ++ toString i ++ toString j
  let swap := fun (x y : String) =>
    [ let_ fT (n ("x_" ++ x)) (v x)
    , setv x (sel (v (n "sw")) (v y) (v x))
    , setv y (sel (v (n "sw")) (v (n ("x_" ++ x))) (v y)) ]
  [ let_ bT (n "sw") (gt (s "sg" j) (s "sg" i)) ] ++
  swap ("sg" ++ toString i) ("sg" ++ toString j) ++
  r3.flatMap (fun k => swap ("a" ++ toString (3 * k + i)) ("a" ++ toString (3 * k + j))) ++
  r3.flatMap (fun k => swap ("w" ++ toString (3 * k + i)) ("w" ++ toString (3 * k + j)))

/-- `R` = Kabsch's rotation of the 3×3 `h` (the spec's `kabsch`). Uses the names `a`, `w`,
    `sg`, `kc_*`, `u*`, `P`, `d`. -/
def kabsch (h : String) : List St :=
  mat "a" (fun i j => m h i j) ++
  mat "w" (fun i j => if i == j then fl 1.0 else fl 0.0) ++
  [ for_ "sweep" (u 0) (u RotationStar.sweeps) ([(0, 1), (0, 2), (1, 2)].flatMap rotPair) ] ++
  vec "sg" (fun c => call "sqrt" [m "a" 0 c * m "a" 0 c + m "a" 1 c * m "a" 1 c + m "a" 2 c * m "a" 2 c]) ++
  [(0, 1), (0, 2), (1, 2)].flatMap sortPair ++
  [ let_ fT "eps" (flx RotationStar.rankTol * s "sg" 0) ] ++
  -- U columns: a's column over σ, or 0 at or below eps; V columns: w's.
  (r3.flatMap fun c => vec ("u" ++ toString c ++ "_")
      (fun r => sel (gt (s "sg" c) (v "eps")) (m "a" r c / s "sg" c) (fl 0.0))) ++
  -- the rank-1 completion runs in names kc_*: kc_v0 = V col 0, kc_u0 = U col 0, and
  -- kc_v1, kc_v2 = frameOf kc_v0, which also replace V's columns 1 and 2.
  vec "kc_v0" (fun r => m "w" r 0) ++ vec "kc_u0" (fun r => s "u0_" r) ++
  [ if_ (le (s "sg" 0) (v "eps"))
      (r3.flatMap fun c => r3.map fun r =>
        setv ("u" ++ toString c ++ "_" ++ toString r) (fl (if c == r then 1.0 else 0.0)))
      [ if_ (le (s "sg" 1) (v "eps"))
          (Split.frameOf "kc" ++ Split.canonCompletion "kc" ++
           r3.map (fun r => setv ("u1_" ++ toString r) (s "kc_c1" r)) ++
           r3.map (fun r => setv ("u2_" ++ toString r) (s "kc_c2" r)) ++
           r3.map (fun r => setv ("w" ++ toString (3 * r + 1)) (s "kc_v1" r)) ++
           r3.map (fun r => setv ("w" ++ toString (3 * r + 2)) (s "kc_v2" r)))
          [ if_ (le (s "sg" 2) (v "eps"))
              (cross "kx" "u0_" "u1_" ++ normed "kn" "kx" ++
               r3.map (fun r => setv ("u2_" ++ toString r) (s "kn" r))) ] ] ] ++
  -- d = sign det(U Vᵀ); R = U diag(1, 1, d) Vᵀ.
  mat "P" (fun i j => s "u0_" i * m "w" j 0 + s "u1_" i * m "w" j 1 + s "u2_" i * m "w" j 2) ++
  [ let_ fT "det" (m "P" 0 0 * (m "P" 1 1 * m "P" 2 2 - m "P" 1 2 * m "P" 2 1) +
                   m "P" 0 1 * (m "P" 1 2 * m "P" 2 0 - m "P" 1 0 * m "P" 2 2) +
                   m "P" 0 2 * (m "P" 1 0 * m "P" 2 1 - m "P" 1 1 * m "P" 2 0))
  , let_ fT "d" (sel (ge (v "det") (fl 0.0)) (fl 1.0) (fl (-1.0))) ] ++
  mat "R" (fun i j => s "u0_" i * m "w" j 0 + s "u1_" i * m "w" j 1 + v "d" * s "u2_" i * m "w" j 2)

def body : List St :=
  [ let_ uT "i" (.member (v "tid") "x")
  , if_ (ge (v "i") (p "N")) [ret] ] ++
  loadStar "t" "tgt" ++ loadStar "q" "src" ++
  centroid "ct" "t" ++ centroid "cq" "q" ++
  ((List.range 7).flatMap fun k =>
    vec ("dt" ++ toString k ++ "_") (fun c => s ("t" ++ toString k ++ "_") c - s "ct" c) ++
    vec ("dq" ++ toString k ++ "_") (fun c => s ("q" ++ toString k ++ "_") c - s "cq" c)) ++
  mat "H" (fun i j =>
    let term := fun k => s ("dt" ++ toString k ++ "_") i * s ("dq" ++ toString k ++ "_") j
    term 0 + term 1 + term 2 + term 3 + term 4 + term 5 + term 6) ++
  kabsch "H" ++
  store9 "rot" (v "i" * u 9) "R"

def shader : SlangShaderModule :=
  { structs := [ { name := "StarKabschParams", fields := [fld "N" uT] } ]
  , globals := [ paramsCB "StarKabschParams", roF "tgt" 1, roF "src" 2, rwF "rot" 3 ]
  , functions := [ entry 64 [dtid] body ] }

-- BEGIN PIN
def expected : String := "struct StarKabschParams {\n  uint N;\n};\n\n[[vk::binding(0, 0)]]\nConstantBuffer<StarKabschParams> params;\n[[vk::binding(1, 0)]]\nStructuredBuffer<float> tgt;\n[[vk::binding(2, 0)]]\nStructuredBuffer<float> src;\n[[vk::binding(3, 0)]]\nRWStructuredBuffer<float> rot;\n\n[shader(\"compute\")] [numthreads(64, 1, 1)]\nvoid main(uint3 tid : SV_DispatchThreadID) {\n  uint i = tid.x;\n  if ((i >= params.N)) {\n    return;\n  }\n  float t0_0 = tgt[((i * 21u) + 0u)];\n  float t0_1 = tgt[((i * 21u) + 1u)];\n  float t0_2 = tgt[((i * 21u) + 2u)];\n  float t1_0 = tgt[((i * 21u) + 3u)];\n  float t1_1 = tgt[((i * 21u) + 4u)];\n  float t1_2 = tgt[((i * 21u) + 5u)];\n  float t2_0 = tgt[((i * 21u) + 6u)];\n  float t2_1 = tgt[((i * 21u) + 7u)];\n  float t2_2 = tgt[((i * 21u) + 8u)];\n  float t3_0 = tgt[((i * 21u) + 9u)];\n  float t3_1 = tgt[((i * 21u) + 10u)];\n  float t3_2 = tgt[((i * 21u) + 11u)];\n  float t4_0 = tgt[((i * 21u) + 12u)];\n  float t4_1 = tgt[((i * 21u) + 13u)];\n  float t4_2 = tgt[((i * 21u) + 14u)];\n  float t5_0 = tgt[((i * 21u) + 15u)];\n  float t5_1 = tgt[((i * 21u) + 16u)];\n  float t5_2 = tgt[((i * 21u) + 17u)];\n  float t6_0 = tgt[((i * 21u) + 18u)];\n  float t6_1 = tgt[((i * 21u) + 19u)];\n  float t6_2 = tgt[((i * 21u) + 20u)];\n  float q0_0 = src[((i * 21u) + 0u)];\n  float q0_1 = src[((i * 21u) + 1u)];\n  float q0_2 = src[((i * 21u) + 2u)];\n  float q1_0 = src[((i * 21u) + 3u)];\n  float q1_1 = src[((i * 21u) + 4u)];\n  float q1_2 = src[((i * 21u) + 5u)];\n  float q2_0 = src[((i * 21u) + 6u)];\n  float q2_1 = src[((i * 21u) + 7u)];\n  float q2_2 = src[((i * 21u) + 8u)];\n  float q3_0 = src[((i * 21u) + 9u)];\n  float q3_1 = src[((i * 21u) + 10u)];\n  float q3_2 = src[((i * 21u) + 11u)];\n  float q4_0 = src[((i * 21u) + 12u)];\n  float q4_1 = src[((i * 21u) + 13u)];\n  float q4_2 = src[((i * 21u) + 14u)];\n  float q5_0 = src[((i * 21u) + 15u)];\n  float q5_1 = src[((i * 21u) + 16u)];\n  float q5_2 = src[((i * 21u) + 17u)];\n  float q6_0 = src[((i * 21u) + 18u)];\n  float q6_1 = src[((i * 21u) + 19u)];\n  float q6_2 = src[((i * 21u) + 20u)];\n  float ct0 = (((((((t0_0 + t1_0) + t2_0) + t3_0) + t4_0) + t5_0) + t6_0) / 7.000000);\n  float ct1 = (((((((t0_1 + t1_1) + t2_1) + t3_1) + t4_1) + t5_1) + t6_1) / 7.000000);\n  float ct2 = (((((((t0_2 + t1_2) + t2_2) + t3_2) + t4_2) + t5_2) + t6_2) / 7.000000);\n  float cq0 = (((((((q0_0 + q1_0) + q2_0) + q3_0) + q4_0) + q5_0) + q6_0) / 7.000000);\n  float cq1 = (((((((q0_1 + q1_1) + q2_1) + q3_1) + q4_1) + q5_1) + q6_1) / 7.000000);\n  float cq2 = (((((((q0_2 + q1_2) + q2_2) + q3_2) + q4_2) + q5_2) + q6_2) / 7.000000);\n  float dt0_0 = (t0_0 - ct0);\n  float dt0_1 = (t0_1 - ct1);\n  float dt0_2 = (t0_2 - ct2);\n  float dq0_0 = (q0_0 - cq0);\n  float dq0_1 = (q0_1 - cq1);\n  float dq0_2 = (q0_2 - cq2);\n  float dt1_0 = (t1_0 - ct0);\n  float dt1_1 = (t1_1 - ct1);\n  float dt1_2 = (t1_2 - ct2);\n  float dq1_0 = (q1_0 - cq0);\n  float dq1_1 = (q1_1 - cq1);\n  float dq1_2 = (q1_2 - cq2);\n  float dt2_0 = (t2_0 - ct0);\n  float dt2_1 = (t2_1 - ct1);\n  float dt2_2 = (t2_2 - ct2);\n  float dq2_0 = (q2_0 - cq0);\n  float dq2_1 = (q2_1 - cq1);\n  float dq2_2 = (q2_2 - cq2);\n  float dt3_0 = (t3_0 - ct0);\n  float dt3_1 = (t3_1 - ct1);\n  float dt3_2 = (t3_2 - ct2);\n  float dq3_0 = (q3_0 - cq0);\n  float dq3_1 = (q3_1 - cq1);\n  float dq3_2 = (q3_2 - cq2);\n  float dt4_0 = (t4_0 - ct0);\n  float dt4_1 = (t4_1 - ct1);\n  float dt4_2 = (t4_2 - ct2);\n  float dq4_0 = (q4_0 - cq0);\n  float dq4_1 = (q4_1 - cq1);\n  float dq4_2 = (q4_2 - cq2);\n  float dt5_0 = (t5_0 - ct0);\n  float dt5_1 = (t5_1 - ct1);\n  float dt5_2 = (t5_2 - ct2);\n  float dq5_0 = (q5_0 - cq0);\n  float dq5_1 = (q5_1 - cq1);\n  float dq5_2 = (q5_2 - cq2);\n  float dt6_0 = (t6_0 - ct0);\n  float dt6_1 = (t6_1 - ct1);\n  float dt6_2 = (t6_2 - ct2);\n  float dq6_0 = (q6_0 - cq0);\n  float dq6_1 = (q6_1 - cq1);\n  float dq6_2 = (q6_2 - cq2);\n  float H0 = (((((((dt0_0 * dq0_0) + (dt1_0 * dq1_0)) + (dt2_0 * dq2_0)) + (dt3_0 * dq3_0)) + (dt4_0 * dq4_0)) + (dt5_0 * dq5_0)) + (dt6_0 * dq6_0));\n  float H1 = (((((((dt0_0 * dq0_1) + (dt1_0 * dq1_1)) + (dt2_0 * dq2_1)) + (dt3_0 * dq3_1)) + (dt4_0 * dq4_1)) + (dt5_0 * dq5_1)) + (dt6_0 * dq6_1));\n  float H2 = (((((((dt0_0 * dq0_2) + (dt1_0 * dq1_2)) + (dt2_0 * dq2_2)) + (dt3_0 * dq3_2)) + (dt4_0 * dq4_2)) + (dt5_0 * dq5_2)) + (dt6_0 * dq6_2));\n  float H3 = (((((((dt0_1 * dq0_0) + (dt1_1 * dq1_0)) + (dt2_1 * dq2_0)) + (dt3_1 * dq3_0)) + (dt4_1 * dq4_0)) + (dt5_1 * dq5_0)) + (dt6_1 * dq6_0));\n  float H4 = (((((((dt0_1 * dq0_1) + (dt1_1 * dq1_1)) + (dt2_1 * dq2_1)) + (dt3_1 * dq3_1)) + (dt4_1 * dq4_1)) + (dt5_1 * dq5_1)) + (dt6_1 * dq6_1));\n  float H5 = (((((((dt0_1 * dq0_2) + (dt1_1 * dq1_2)) + (dt2_1 * dq2_2)) + (dt3_1 * dq3_2)) + (dt4_1 * dq4_2)) + (dt5_1 * dq5_2)) + (dt6_1 * dq6_2));\n  float H6 = (((((((dt0_2 * dq0_0) + (dt1_2 * dq1_0)) + (dt2_2 * dq2_0)) + (dt3_2 * dq3_0)) + (dt4_2 * dq4_0)) + (dt5_2 * dq5_0)) + (dt6_2 * dq6_0));\n  float H7 = (((((((dt0_2 * dq0_1) + (dt1_2 * dq1_1)) + (dt2_2 * dq2_1)) + (dt3_2 * dq3_1)) + (dt4_2 * dq4_1)) + (dt5_2 * dq5_1)) + (dt6_2 * dq6_1));\n  float H8 = (((((((dt0_2 * dq0_2) + (dt1_2 * dq1_2)) + (dt2_2 * dq2_2)) + (dt3_2 * dq3_2)) + (dt4_2 * dq4_2)) + (dt5_2 * dq5_2)) + (dt6_2 * dq6_2));\n  float a0 = H0;\n  float a1 = H1;\n  float a2 = H2;\n  float a3 = H3;\n  float a4 = H4;\n  float a5 = H5;\n  float a6 = H6;\n  float a7 = H7;\n  float a8 = H8;\n  float w0 = 1.000000;\n  float w1 = 0.000000;\n  float w2 = 0.000000;\n  float w3 = 0.000000;\n  float w4 = 1.000000;\n  float w5 = 0.000000;\n  float w6 = 0.000000;\n  float w7 = 0.000000;\n  float w8 = 1.000000;\n  for (uint sweep = 0u; sweep < 8u; ++sweep) {\n    float al01 = (((a0 * a0) + (a3 * a3)) + (a6 * a6));\n    float be01 = (((a1 * a1) + (a4 * a4)) + (a7 * a7));\n    float ga01 = (((a0 * a1) + (a3 * a4)) + (a6 * a7));\n    if ((abs(ga01) > (1.1920929e-7f * sqrt((al01 * be01))))) {\n      float ze01 = ((be01 - al01) / (2.000000 * ga01));\n      float t01 = (((ze01 >= 0.000000) ? 1.000000 : -1.000000) / (abs(ze01) + sqrt((1.000000 + (ze01 * ze01)))));\n      float c01 = (1.000000 / sqrt((1.000000 + (t01 * t01))));\n      float s01 = (c01 * t01);\n      float ap010 = a0;\n      float aq010 = a1;\n      a0 = ((c01 * ap010) - (s01 * aq010));\n      a1 = ((s01 * ap010) + (c01 * aq010));\n      float wp010 = w0;\n      float wq010 = w1;\n      w0 = ((c01 * wp010) - (s01 * wq010));\n      w1 = ((s01 * wp010) + (c01 * wq010));\n      float ap011 = a3;\n      float aq011 = a4;\n      a3 = ((c01 * ap011) - (s01 * aq011));\n      a4 = ((s01 * ap011) + (c01 * aq011));\n      float wp011 = w3;\n      float wq011 = w4;\n      w3 = ((c01 * wp011) - (s01 * wq011));\n      w4 = ((s01 * wp011) + (c01 * wq011));\n      float ap012 = a6;\n      float aq012 = a7;\n      a6 = ((c01 * ap012) - (s01 * aq012));\n      a7 = ((s01 * ap012) + (c01 * aq012));\n      float wp012 = w6;\n      float wq012 = w7;\n      w6 = ((c01 * wp012) - (s01 * wq012));\n      w7 = ((s01 * wp012) + (c01 * wq012));\n    }\n    float al02 = (((a0 * a0) + (a3 * a3)) + (a6 * a6));\n    float be02 = (((a2 * a2) + (a5 * a5)) + (a8 * a8));\n    float ga02 = (((a0 * a2) + (a3 * a5)) + (a6 * a8));\n    if ((abs(ga02) > (1.1920929e-7f * sqrt((al02 * be02))))) {\n      float ze02 = ((be02 - al02) / (2.000000 * ga02));\n      float t02 = (((ze02 >= 0.000000) ? 1.000000 : -1.000000) / (abs(ze02) + sqrt((1.000000 + (ze02 * ze02)))));\n      float c02 = (1.000000 / sqrt((1.000000 + (t02 * t02))));\n      float s02 = (c02 * t02);\n      float ap020 = a0;\n      float aq020 = a2;\n      a0 = ((c02 * ap020) - (s02 * aq020));\n      a2 = ((s02 * ap020) + (c02 * aq020));\n      float wp020 = w0;\n      float wq020 = w2;\n      w0 = ((c02 * wp020) - (s02 * wq020));\n      w2 = ((s02 * wp020) + (c02 * wq020));\n      float ap021 = a3;\n      float aq021 = a5;\n      a3 = ((c02 * ap021) - (s02 * aq021));\n      a5 = ((s02 * ap021) + (c02 * aq021));\n      float wp021 = w3;\n      float wq021 = w5;\n      w3 = ((c02 * wp021) - (s02 * wq021));\n      w5 = ((s02 * wp021) + (c02 * wq021));\n      float ap022 = a6;\n      float aq022 = a8;\n      a6 = ((c02 * ap022) - (s02 * aq022));\n      a8 = ((s02 * ap022) + (c02 * aq022));\n      float wp022 = w6;\n      float wq022 = w8;\n      w6 = ((c02 * wp022) - (s02 * wq022));\n      w8 = ((s02 * wp022) + (c02 * wq022));\n    }\n    float al12 = (((a1 * a1) + (a4 * a4)) + (a7 * a7));\n    float be12 = (((a2 * a2) + (a5 * a5)) + (a8 * a8));\n    float ga12 = (((a1 * a2) + (a4 * a5)) + (a7 * a8));\n    if ((abs(ga12) > (1.1920929e-7f * sqrt((al12 * be12))))) {\n      float ze12 = ((be12 - al12) / (2.000000 * ga12));\n      float t12 = (((ze12 >= 0.000000) ? 1.000000 : -1.000000) / (abs(ze12) + sqrt((1.000000 + (ze12 * ze12)))));\n      float c12 = (1.000000 / sqrt((1.000000 + (t12 * t12))));\n      float s12 = (c12 * t12);\n      float ap120 = a1;\n      float aq120 = a2;\n      a1 = ((c12 * ap120) - (s12 * aq120));\n      a2 = ((s12 * ap120) + (c12 * aq120));\n      float wp120 = w1;\n      float wq120 = w2;\n      w1 = ((c12 * wp120) - (s12 * wq120));\n      w2 = ((s12 * wp120) + (c12 * wq120));\n      float ap121 = a4;\n      float aq121 = a5;\n      a4 = ((c12 * ap121) - (s12 * aq121));\n      a5 = ((s12 * ap121) + (c12 * aq121));\n      float wp121 = w4;\n      float wq121 = w5;\n      w4 = ((c12 * wp121) - (s12 * wq121));\n      w5 = ((s12 * wp121) + (c12 * wq121));\n      float ap122 = a7;\n      float aq122 = a8;\n      a7 = ((c12 * ap122) - (s12 * aq122));\n      a8 = ((s12 * ap122) + (c12 * aq122));\n      float wp122 = w7;\n      float wq122 = w8;\n      w7 = ((c12 * wp122) - (s12 * wq122));\n      w8 = ((s12 * wp122) + (c12 * wq122));\n    }\n  }\n  float sg0 = sqrt((((a0 * a0) + (a3 * a3)) + (a6 * a6)));\n  float sg1 = sqrt((((a1 * a1) + (a4 * a4)) + (a7 * a7)));\n  float sg2 = sqrt((((a2 * a2) + (a5 * a5)) + (a8 * a8)));\n  bool sw01 = (sg1 > sg0);\n  float x_sg001 = sg0;\n  sg0 = (sw01 ? sg1 : sg0);\n  sg1 = (sw01 ? x_sg001 : sg1);\n  float x_a001 = a0;\n  a0 = (sw01 ? a1 : a0);\n  a1 = (sw01 ? x_a001 : a1);\n  float x_a301 = a3;\n  a3 = (sw01 ? a4 : a3);\n  a4 = (sw01 ? x_a301 : a4);\n  float x_a601 = a6;\n  a6 = (sw01 ? a7 : a6);\n  a7 = (sw01 ? x_a601 : a7);\n  float x_w001 = w0;\n  w0 = (sw01 ? w1 : w0);\n  w1 = (sw01 ? x_w001 : w1);\n  float x_w301 = w3;\n  w3 = (sw01 ? w4 : w3);\n  w4 = (sw01 ? x_w301 : w4);\n  float x_w601 = w6;\n  w6 = (sw01 ? w7 : w6);\n  w7 = (sw01 ? x_w601 : w7);\n  bool sw02 = (sg2 > sg0);\n  float x_sg002 = sg0;\n  sg0 = (sw02 ? sg2 : sg0);\n  sg2 = (sw02 ? x_sg002 : sg2);\n  float x_a002 = a0;\n  a0 = (sw02 ? a2 : a0);\n  a2 = (sw02 ? x_a002 : a2);\n  float x_a302 = a3;\n  a3 = (sw02 ? a5 : a3);\n  a5 = (sw02 ? x_a302 : a5);\n  float x_a602 = a6;\n  a6 = (sw02 ? a8 : a6);\n  a8 = (sw02 ? x_a602 : a8);\n  float x_w002 = w0;\n  w0 = (sw02 ? w2 : w0);\n  w2 = (sw02 ? x_w002 : w2);\n  float x_w302 = w3;\n  w3 = (sw02 ? w5 : w3);\n  w5 = (sw02 ? x_w302 : w5);\n  float x_w602 = w6;\n  w6 = (sw02 ? w8 : w6);\n  w8 = (sw02 ? x_w602 : w8);\n  bool sw12 = (sg2 > sg1);\n  float x_sg112 = sg1;\n  sg1 = (sw12 ? sg2 : sg1);\n  sg2 = (sw12 ? x_sg112 : sg2);\n  float x_a112 = a1;\n  a1 = (sw12 ? a2 : a1);\n  a2 = (sw12 ? x_a112 : a2);\n  float x_a412 = a4;\n  a4 = (sw12 ? a5 : a4);\n  a5 = (sw12 ? x_a412 : a5);\n  float x_a712 = a7;\n  a7 = (sw12 ? a8 : a7);\n  a8 = (sw12 ? x_a712 : a8);\n  float x_w112 = w1;\n  w1 = (sw12 ? w2 : w1);\n  w2 = (sw12 ? x_w112 : w2);\n  float x_w412 = w4;\n  w4 = (sw12 ? w5 : w4);\n  w5 = (sw12 ? x_w412 : w5);\n  float x_w712 = w7;\n  w7 = (sw12 ? w8 : w7);\n  w8 = (sw12 ? x_w712 : w8);\n  float eps = (1.0e-6f * sg0);\n  float u0_0 = ((sg0 > eps) ? (a0 / sg0) : 0.000000);\n  float u0_1 = ((sg0 > eps) ? (a3 / sg0) : 0.000000);\n  float u0_2 = ((sg0 > eps) ? (a6 / sg0) : 0.000000);\n  float u1_0 = ((sg1 > eps) ? (a1 / sg1) : 0.000000);\n  float u1_1 = ((sg1 > eps) ? (a4 / sg1) : 0.000000);\n  float u1_2 = ((sg1 > eps) ? (a7 / sg1) : 0.000000);\n  float u2_0 = ((sg2 > eps) ? (a2 / sg2) : 0.000000);\n  float u2_1 = ((sg2 > eps) ? (a5 / sg2) : 0.000000);\n  float u2_2 = ((sg2 > eps) ? (a8 / sg2) : 0.000000);\n  float kc_v00 = w0;\n  float kc_v01 = w3;\n  float kc_v02 = w6;\n  float kc_u00 = u0_0;\n  float kc_u01 = u0_1;\n  float kc_u02 = u0_2;\n  if ((sg0 <= eps)) {\n    u0_0 = 1.000000;\n    u0_1 = 0.000000;\n    u0_2 = 0.000000;\n    u1_0 = 0.000000;\n    u1_1 = 1.000000;\n    u1_2 = 0.000000;\n    u2_0 = 0.000000;\n    u2_1 = 0.000000;\n    u2_2 = 1.000000;\n  } else {\n    if ((sg1 <= eps)) {\n      bool kc_fy = (abs(kc_v01) < 0.900000);\n      float kc_ref0 = (kc_fy ? 0.000000 : 1.000000);\n      float kc_ref1 = (kc_fy ? 1.000000 : 0.000000);\n      float kc_ref2 = 0.000000;\n      float kc_v1r0 = ((kc_v01 * kc_ref2) - (kc_v02 * kc_ref1));\n      float kc_v1r1 = ((kc_v02 * kc_ref0) - (kc_v00 * kc_ref2));\n      float kc_v1r2 = ((kc_v00 * kc_ref1) - (kc_v01 * kc_ref0));\n      float kc_v1_l = sqrt((((kc_v1r0 * kc_v1r0) + (kc_v1r1 * kc_v1r1)) + (kc_v1r2 * kc_v1r2)));\n      float kc_v10 = ((kc_v1_l > 1.0e-12f) ? (kc_v1r0 / kc_v1_l) : kc_v1r0);\n      float kc_v11 = ((kc_v1_l > 1.0e-12f) ? (kc_v1r1 / kc_v1_l) : kc_v1r1);\n      float kc_v12 = ((kc_v1_l > 1.0e-12f) ? (kc_v1r2 / kc_v1_l) : kc_v1r2);\n      float kc_v2r0 = ((kc_v01 * kc_v12) - (kc_v02 * kc_v11));\n      float kc_v2r1 = ((kc_v02 * kc_v10) - (kc_v00 * kc_v12));\n      float kc_v2r2 = ((kc_v00 * kc_v11) - (kc_v01 * kc_v10));\n      float kc_v2_l = sqrt((((kc_v2r0 * kc_v2r0) + (kc_v2r1 * kc_v2r1)) + (kc_v2r2 * kc_v2r2)));\n      float kc_v20 = ((kc_v2_l > 1.0e-12f) ? (kc_v2r0 / kc_v2_l) : kc_v2r0);\n      float kc_v21 = ((kc_v2_l > 1.0e-12f) ? (kc_v2r1 / kc_v2_l) : kc_v2r1);\n      float kc_v22 = ((kc_v2_l > 1.0e-12f) ? (kc_v2r2 / kc_v2_l) : kc_v2r2);\n      float kc_ax0 = ((kc_v01 * kc_u02) - (kc_v02 * kc_u01));\n      float kc_ax1 = ((kc_v02 * kc_u00) - (kc_v00 * kc_u02));\n      float kc_ax2 = ((kc_v00 * kc_u01) - (kc_v01 * kc_u00));\n      float kc_sn = sqrt((((kc_ax0 * kc_ax0) + (kc_ax1 * kc_ax1)) + (kc_ax2 * kc_ax2)));\n      float kc_cs = (((kc_v00 * kc_u00) + (kc_v01 * kc_u01)) + (kc_v02 * kc_u02));\n      float kc_k0 = (kc_ax0 / max(kc_sn, 1.0e-9f));\n      float kc_k1 = (kc_ax1 / max(kc_sn, 1.0e-9f));\n      float kc_k2 = (kc_ax2 / max(kc_sn, 1.0e-9f));\n      float kc_c1_kx0 = ((kc_k1 * kc_v12) - (kc_k2 * kc_v11));\n      float kc_c1_kx1 = ((kc_k2 * kc_v10) - (kc_k0 * kc_v12));\n      float kc_c1_kx2 = ((kc_k0 * kc_v11) - (kc_k1 * kc_v10));\n      float kc_c1_kw = (((kc_k0 * kc_v10) + (kc_k1 * kc_v11)) + (kc_k2 * kc_v12));\n      float kc_c10 = (((kc_cs * kc_v10) + (kc_sn * kc_c1_kx0)) + ((kc_c1_kw * (1.000000 - kc_cs)) * kc_k0));\n      float kc_c11 = (((kc_cs * kc_v11) + (kc_sn * kc_c1_kx1)) + ((kc_c1_kw * (1.000000 - kc_cs)) * kc_k1));\n      float kc_c12 = (((kc_cs * kc_v12) + (kc_sn * kc_c1_kx2)) + ((kc_c1_kw * (1.000000 - kc_cs)) * kc_k2));\n      float kc_c2_kx0 = ((kc_k1 * kc_v22) - (kc_k2 * kc_v21));\n      float kc_c2_kx1 = ((kc_k2 * kc_v20) - (kc_k0 * kc_v22));\n      float kc_c2_kx2 = ((kc_k0 * kc_v21) - (kc_k1 * kc_v20));\n      float kc_c2_kw = (((kc_k0 * kc_v20) + (kc_k1 * kc_v21)) + (kc_k2 * kc_v22));\n      float kc_c20 = (((kc_cs * kc_v20) + (kc_sn * kc_c2_kx0)) + ((kc_c2_kw * (1.000000 - kc_cs)) * kc_k0));\n      float kc_c21 = (((kc_cs * kc_v21) + (kc_sn * kc_c2_kx1)) + ((kc_c2_kw * (1.000000 - kc_cs)) * kc_k1));\n      float kc_c22 = (((kc_cs * kc_v22) + (kc_sn * kc_c2_kx2)) + ((kc_c2_kw * (1.000000 - kc_cs)) * kc_k2));\n      if ((kc_sn < 1.0e-9f)) {\n        if ((kc_cs > 0.000000)) {\n          kc_c10 = kc_v10;\n          kc_c11 = kc_v11;\n          kc_c12 = kc_v12;\n          kc_c20 = kc_v20;\n          kc_c21 = kc_v21;\n          kc_c22 = kc_v22;\n        } else {\n          bool kc_px = ((abs(kc_u00) <= abs(kc_u01)) && (abs(kc_u00) <= abs(kc_u02)));\n          float kc_pr0 = (kc_px ? 1.000000 : 0.000000);\n          float kc_pr1 = (kc_px ? 0.000000 : 1.000000);\n          float kc_pr2 = 0.000000;\n          float kc_p10 = ((kc_u01 * kc_pr2) - (kc_u02 * kc_pr1));\n          float kc_p11 = ((kc_u02 * kc_pr0) - (kc_u00 * kc_pr2));\n          float kc_p12 = ((kc_u00 * kc_pr1) - (kc_u01 * kc_pr0));\n          float kc_p1n_l = sqrt((((kc_p10 * kc_p10) + (kc_p11 * kc_p11)) + (kc_p12 * kc_p12)));\n          float kc_p1n0 = ((kc_p1n_l > 1.0e-12f) ? (kc_p10 / kc_p1n_l) : kc_p10);\n          float kc_p1n1 = ((kc_p1n_l > 1.0e-12f) ? (kc_p11 / kc_p1n_l) : kc_p11);\n          float kc_p1n2 = ((kc_p1n_l > 1.0e-12f) ? (kc_p12 / kc_p1n_l) : kc_p12);\n          float kc_p20 = ((kc_u01 * kc_p1n2) - (kc_u02 * kc_p1n1));\n          float kc_p21 = ((kc_u02 * kc_p1n0) - (kc_u00 * kc_p1n2));\n          float kc_p22 = ((kc_u00 * kc_p1n1) - (kc_u01 * kc_p1n0));\n          float kc_p2n_l = sqrt((((kc_p20 * kc_p20) + (kc_p21 * kc_p21)) + (kc_p22 * kc_p22)));\n          float kc_p2n0 = ((kc_p2n_l > 1.0e-12f) ? (kc_p20 / kc_p2n_l) : kc_p20);\n          float kc_p2n1 = ((kc_p2n_l > 1.0e-12f) ? (kc_p21 / kc_p2n_l) : kc_p21);\n          float kc_p2n2 = ((kc_p2n_l > 1.0e-12f) ? (kc_p22 / kc_p2n_l) : kc_p22);\n          kc_c10 = kc_p1n0;\n          kc_c11 = kc_p1n1;\n          kc_c12 = kc_p1n2;\n          kc_c20 = kc_p2n0;\n          kc_c21 = kc_p2n1;\n          kc_c22 = kc_p2n2;\n        }\n      }\n      u1_0 = kc_c10;\n      u1_1 = kc_c11;\n      u1_2 = kc_c12;\n      u2_0 = kc_c20;\n      u2_1 = kc_c21;\n      u2_2 = kc_c22;\n      w1 = kc_v10;\n      w4 = kc_v11;\n      w7 = kc_v12;\n      w2 = kc_v20;\n      w5 = kc_v21;\n      w8 = kc_v22;\n    } else {\n      if ((sg2 <= eps)) {\n        float kx0 = ((u0_1 * u1_2) - (u0_2 * u1_1));\n        float kx1 = ((u0_2 * u1_0) - (u0_0 * u1_2));\n        float kx2 = ((u0_0 * u1_1) - (u0_1 * u1_0));\n        float kn_l = sqrt((((kx0 * kx0) + (kx1 * kx1)) + (kx2 * kx2)));\n        float kn0 = ((kn_l > 1.0e-12f) ? (kx0 / kn_l) : kx0);\n        float kn1 = ((kn_l > 1.0e-12f) ? (kx1 / kn_l) : kx1);\n        float kn2 = ((kn_l > 1.0e-12f) ? (kx2 / kn_l) : kx2);\n        u2_0 = kn0;\n        u2_1 = kn1;\n        u2_2 = kn2;\n      }\n    }\n  }\n  float P0 = (((u0_0 * w0) + (u1_0 * w1)) + (u2_0 * w2));\n  float P1 = (((u0_0 * w3) + (u1_0 * w4)) + (u2_0 * w5));\n  float P2 = (((u0_0 * w6) + (u1_0 * w7)) + (u2_0 * w8));\n  float P3 = (((u0_1 * w0) + (u1_1 * w1)) + (u2_1 * w2));\n  float P4 = (((u0_1 * w3) + (u1_1 * w4)) + (u2_1 * w5));\n  float P5 = (((u0_1 * w6) + (u1_1 * w7)) + (u2_1 * w8));\n  float P6 = (((u0_2 * w0) + (u1_2 * w1)) + (u2_2 * w2));\n  float P7 = (((u0_2 * w3) + (u1_2 * w4)) + (u2_2 * w5));\n  float P8 = (((u0_2 * w6) + (u1_2 * w7)) + (u2_2 * w8));\n  float det = (((P0 * ((P4 * P8) - (P5 * P7))) + (P1 * ((P5 * P6) - (P3 * P8)))) + (P2 * ((P3 * P7) - (P4 * P6))));\n  float d = ((det >= 0.000000) ? 1.000000 : -1.000000);\n  float R0 = (((u0_0 * w0) + (u1_0 * w1)) + ((d * u2_0) * w2));\n  float R1 = (((u0_0 * w3) + (u1_0 * w4)) + ((d * u2_0) * w5));\n  float R2 = (((u0_0 * w6) + (u1_0 * w7)) + ((d * u2_0) * w8));\n  float R3 = (((u0_1 * w0) + (u1_1 * w1)) + ((d * u2_1) * w2));\n  float R4 = (((u0_1 * w3) + (u1_1 * w4)) + ((d * u2_1) * w5));\n  float R5 = (((u0_1 * w6) + (u1_1 * w7)) + ((d * u2_1) * w8));\n  float R6 = (((u0_2 * w0) + (u1_2 * w1)) + ((d * u2_2) * w2));\n  float R7 = (((u0_2 * w3) + (u1_2 * w4)) + ((d * u2_2) * w5));\n  float R8 = (((u0_2 * w6) + (u1_2 * w7)) + ((d * u2_2) * w8));\n  rot[((i * 9u) + 0u)] = R0;\n  rot[((i * 9u) + 1u)] = R1;\n  rot[((i * 9u) + 2u)] = R2;\n  rot[((i * 9u) + 3u)] = R3;\n  rot[((i * 9u) + 4u)] = R4;\n  rot[((i * 9u) + 5u)] = R5;\n  rot[((i * 9u) + 6u)] = R6;\n  rot[((i * 9u) + 7u)] = R7;\n  rot[((i * 9u) + 8u)] = R8;\n}"
example : LeanSlang.emit shader = expected := by native_decide
example : shader.entryPointName = "main" := by native_decide
-- END PIN

end RotationStar.Slang.StarKabsch

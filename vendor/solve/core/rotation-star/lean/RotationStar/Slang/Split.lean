-- SPDX-License-Identifier: MIT
import RotationStar.Slang.Dsl

/-!
# `RotationStar.Slang.Split` — statement generators for the completion and the split

Not a kernel. The Slang of `RotationStar.Completion` and `RotationStar.SwingTwist`, operation for
operation, as statement lists a kernel inlines. Names are `<out>_<x>`, so two uses in one scope
need two `out` prefixes.

`swingTwistAngle` is the generator contract-anny-kernels' `SomaCommon` carried as a local copy
(after swing-twist-kusudama `KusKabsch.lean`); the SOMA-X twist kernel now takes it from here,
and its pins check the emitted text is unchanged.
-/

namespace RotationStar.Slang.Split

open LeanSlang
open RotationStar.Slang.Dsl

/-- `dst = unitK(src)`: divide by the length, or keep `src` below a length of 1e-12. -/
def normed (dst src : String) : List St :=
  [ let_ fT (dst ++ "_l") (call "sqrt" [dot3 src src]) ] ++
  vec dst (fun i => sel (gt (v (dst ++ "_l")) (flx 1e-12)) (s src i / v (dst ++ "_l")) (s src i))

/-- `dst = rod k cs sn w`, with `cs`, `sn` the locals `<out>_cs`, `<out>_sn`. -/
def rod (out dst k w : String) : List St :=
  cross (dst ++ "_kx") k w ++
  [ let_ fT (dst ++ "_kw") (dot3 k w) ] ++
  vec dst (fun i => v (out ++ "_cs") * s w i + v (out ++ "_sn") * s (dst ++ "_kx") i +
                    v (dst ++ "_kw") * (fl 1.0 - v (out ++ "_cs")) * s k i)

/-- `frameOf`: `<out>_v1`, `<out>_v2` from the unit vector `<out>_v0`. -/
def frameOf (out : String) : List St :=
  let n := fun x => out ++ x
  [ let_ bT (n "_fy") (lt (fabs (s (n "_v0") 1)) (fl 0.9)) ] ++
  vec (n "_ref") (fun i => if i == 0 then sel (v (n "_fy")) (fl 0.0) (fl 1.0)
                           else if i == 1 then sel (v (n "_fy")) (fl 1.0) (fl 0.0) else fl 0.0) ++
  cross (n "_v1r") (n "_v0") (n "_ref") ++ normed (n "_v1") (n "_v1r") ++
  cross (n "_v2r") (n "_v0") (n "_v1") ++ normed (n "_v2") (n "_v2r")

/-- `canonCompletion`: `<out>_c1`, `<out>_c2` from `<out>_v0`, `<out>_u0`, `<out>_v1`,
    `<out>_v2`, with the aligned and antipodal fallbacks. -/
def canonCompletion (out : String) : List St :=
  let n := fun x => out ++ x
  cross (n "_ax") (n "_v0") (n "_u0") ++
  [ let_ fT (n "_sn") (call "sqrt" [dot3 (n "_ax") (n "_ax")])
  , let_ fT (n "_cs") (dot3 (n "_v0") (n "_u0")) ] ++
  vec (n "_k") (fun i => s (n "_ax") i / fmax (v (n "_sn")) (flx 1e-9)) ++
  rod out (n "_c1") (n "_k") (n "_v1") ++ rod out (n "_c2") (n "_k") (n "_v2") ++
  [ if_ (lt (v (n "_sn")) (flx 1e-9))
      [ if_ (gt (v (n "_cs")) (fl 0.0))
          (r3.map (fun i => setv (n "_c1" ++ toString i) (s (n "_v1") i)) ++
           r3.map (fun i => setv (n "_c2" ++ toString i) (s (n "_v2") i)))
          ([ let_ bT (n "_px") (and_ (le (fabs (s (n "_u0") 0)) (fabs (s (n "_u0") 1)))
                                     (le (fabs (s (n "_u0") 0)) (fabs (s (n "_u0") 2)))) ] ++
           vec (n "_pr") (fun i => if i == 0 then sel (v (n "_px")) (fl 1.0) (fl 0.0)
                                   else if i == 1 then sel (v (n "_px")) (fl 0.0) (fl 1.0) else fl 0.0) ++
           cross (n "_p1") (n "_u0") (n "_pr") ++ normed (n "_p1n") (n "_p1") ++
           cross (n "_p2") (n "_u0") (n "_p1n") ++ normed (n "_p2n") (n "_p2") ++
           r3.map (fun i => setv (n "_c1" ++ toString i) (s (n "_p1n") i)) ++
           r3.map (fun i => setv (n "_c2" ++ toString i) (s (n "_p2n") i))) ] ]

/-- `out = ` the twist angle of `r` about the coordinate axis `ax` (a runtime uint, 0 = x) by the
    swing-twist split, leaving the swing `<out>_R0` and the twist `<out>_T` in scope:

      v0 = a, u0 = unitK(r a)                  `frameOf v0` gives v1, v2
      (c1, c2) = `canonCompletion`             R0 v1, R0 v2, R0 the shortest arc v0 → u0
      swing  R0 = U Vᵀ,  U = [u0, c1, c2],  V = [v0, v1, v2]
      twist  T  = R0ᵀ · r                      a rotation about a alone
      angle  atan2 of T's two off-diagonal and two diagonal entries in the plane normal to a

    This is the standard twist, `2·atan2(q_a, w)`, from 3×3s only. -/
def swingTwistAngle (out r : String) (ax : E) : List St :=
  let n := fun x => out ++ x
  vec (n "_v0") (fun i => axisUnit ax i) ++
  vec (n "_ru") (fun i => m r i 0 * s (n "_v0") 0 + m r i 1 * s (n "_v0") 1 + m r i 2 * s (n "_v0") 2) ++
  normed (n "_u0") (n "_ru") ++
  frameOf out ++
  canonCompletion out ++
  -- R0 = U Vᵀ: entry (i, j) = u0_i v0_j + c1_i v1_j + c2_i v2_j
  mat (n "_R0") (fun i j => s (n "_u0") i * s (n "_v0") j + s (n "_c1") i * s (n "_v1") j +
                            s (n "_c2") i * s (n "_v2") j) ++
  matTMul (n "_T") (n "_R0") r ++
  [ let_ fT out (call "atan2"
      [ sel (eq ax (u 0)) (m (n "_T") 2 1 - m (n "_T") 1 2)
          (sel (eq ax (u 1)) (m (n "_T") 0 2 - m (n "_T") 2 0) (m (n "_T") 1 0 - m (n "_T") 0 1))
      , sel (eq ax (u 0)) (m (n "_T") 1 1 + m (n "_T") 2 2)
          (sel (eq ax (u 1)) (m (n "_T") 2 2 + m (n "_T") 0 0) (m (n "_T") 0 0 + m (n "_T") 1 1)) ]) ]

end RotationStar.Slang.Split

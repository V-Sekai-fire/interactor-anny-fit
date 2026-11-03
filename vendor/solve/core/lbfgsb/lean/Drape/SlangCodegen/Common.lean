import Drape.SlangCodegen.Dsl

/-!
# `Drape.SlangCodegen.Common` — shared pieces of the L-BFGS-B kernels

Not a kernel. Three kinds of shared material:

1. **df32 helpers** (`two_sum`, `quick_two_sum`, `two_prod`, `df_add`,
   and `df_acc`: `(hi, lo) += a·b`). The first four are the same
   Knuth/Dekker transformations as `Cloth.SlangCodegen.DotReduce`, so the
   long reductions here carry ~48 mantissa bits like `dot_reduce` does.

2. **Buffer layout** of the compact BFGS state, one float buffer `bf`
   and one uint buffer `st`, for a history capacity `mcap ≤ 16`
   (`maxM`). Slots are LBFGSpp's ring slots (`BFGSMat::m_ptr % m`).

   | `bf` offset | contents |
   |---|---|
   | 0 | θ (`yᵀy / sᵀy` of the last accepted pair; 1 after reset) |
   | 1..3 | reserved |
   | 4 | `SS[i][j] = s_iᵀ s_j` (mcap × mcap, row-major) |
   | 4 + mcap² | `SY[i][j] = s_iᵀ y_j` (row i is written when s_i arrives) |
   | 4 + 2·mcap² | `L[i][j] = SY[i][j]` if pair i is newer than pair j, else 0 |
   | 4 + 3·mcap² | lower Cholesky factor of `T = θ·SS + L·D⁻¹·Lᵀ` |
   | 4 + 4·mcap² | `D[j] = s_jᵀ y_j` |

   | `st` index | contents |
   |---|---|
   | 0 | ncorr (retained pairs) |
   | 1 | ptr (LBFGSpp `m_ptr`: newest slot is `ptr − 1`) |
   | 2 | loc (slot of the last accepted pair) |
   | 3 | ok (1 if the last pair passed `sᵀy > ε·yᵀy`) |
   | 4 | chol_ok (1 if every pivot of T was positive) |

3. **`lb_mv`**: `w ← M·w` for a `2·ncorr` vector in LBFGSpp's layout
   `[Y part ; S part]`. LBFGSpp factors the permuted
   `M⁻¹ = [[−D, Lᵀ], [L, θ·SS]]` with Bunch–Kaufman; here the Schur
   complement of the `−D` block is `T`, which is SPD, so
   `b = T⁻¹(v₂ + L·D⁻¹·v₁)` and `a = D⁻¹(Lᵀ·b − v₁)` need only the
   Cholesky factor `lb_compact` leaves in `bf`. The same helper is
   emitted into `lb_apply_m`, `lb_cauchy` and `lb_subspace`.

Plus statement generators (Cholesky, triangular solves, heap sift) that
kernels inline with a variable-name prefix.
-/

namespace Drape.SlangCodegen.Common

open LeanSlang
open Drape.SlangCodegen.Dsl

/-- History capacity the local arrays are sized for. -/
def maxM : Nat := 16

private def fIn (n : String) : SlangBinding := arg n fT
private def fOut (n : String) : SlangBinding := arg n fT .qOut
private def fInOut (n : String) : SlangBinding := arg n fT .qInOut

def twoSum : SlangFunctionDecl :=
  { name := "two_sum", params := [fIn "a", fIn "b", fOut "hi", fOut "lo"]
  , body :=
      [ let_ fT "h" (v "a" + v "b")
      , let_ fT "bb" (v "h" - v "a")
      , let_ fT "ah" (v "h" - v "bb")
      , let_ fT "lo_a" (v "a" - v "ah")
      , let_ fT "lo_b" (v "b" - v "bb")
      , setv "hi" (v "h")
      , setv "lo" (v "lo_a" + v "lo_b")
      , ret ] }

def quickTwoSum : SlangFunctionDecl :=
  { name := "quick_two_sum", params := [fIn "a", fIn "b", fOut "hi", fOut "lo"]
  , body :=
      [ let_ fT "h" (v "a" + v "b")
      , let_ fT "t" (v "h" - v "a")
      , setv "hi" (v "h")
      , setv "lo" (v "b" - v "t")
      , ret ] }

def twoProd : SlangFunctionDecl :=
  { name := "two_prod", params := [fIn "a", fIn "b", fOut "hi", fOut "lo"]
  , body :=
      [ let_ fT "h" (v "a" * v "b")
      , setv "hi" (v "h")
      , setv "lo" (call "fma" [v "a", v "b", -(v "h")])
      , ret ] }

def dfAdd : SlangFunctionDecl :=
  { name := "df_add"
  , params := [fIn "x_hi", fIn "x_lo", fIn "y_hi", fIn "y_lo", fOut "z_hi", fOut "z_lo"]
  , body :=
      [ decl fT "sh"
      , decl fT "sl"
      , do_ (call "two_sum" [v "x_hi", v "y_hi", v "sh", v "sl"])
      , let_ fT "xy_lo" (v "x_lo" + v "y_lo")
      , let_ fT "sl2" (v "sl" + v "xy_lo")
      , do_ (call "quick_two_sum" [v "sh", v "sl2", v "z_hi", v "z_lo"])
      , ret ] }

/-- `(hi, lo) += a·b` in df32. -/
def dfAcc : SlangFunctionDecl :=
  { name := "df_acc", params := [fInOut "hi", fInOut "lo", fIn "a", fIn "b"]
  , body :=
      [ decl fT "p_hi"
      , decl fT "p_lo"
      , do_ (call "two_prod" [v "a", v "b", v "p_hi", v "p_lo"])
      , decl fT "n_hi"
      , decl fT "n_lo"
      , do_ (call "df_add" [v "hi", v "lo", v "p_hi", v "p_lo", v "n_hi", v "n_lo"])
      , setv "hi" (v "n_hi")
      , setv "lo" (v "n_lo")
      , ret ] }

/-- The five df32 helpers, in dependency order. -/
def dfHelpers : List SlangFunctionDecl := [twoSum, quickTwoSum, twoProd, dfAdd, dfAcc]

/-! ## `bf` layout (see the module doc) -/

def ssIx (mc i j : E) : E := u 4 + i * mc + j
def syIx (mc i j : E) : E := u 4 + mc * mc + i * mc + j
def lmIx (mc i j : E) : E := u 4 + u 2 * mc * mc + i * mc + j
def rcIx (mc i j : E) : E := u 4 + u 3 * mc * mc + i * mc + j
def dIx (mc j : E) : E := u 4 + u 4 * mc * mc + j

/-! ## Statement generators -/

/-- In-place lower Cholesky of the leading `k×k` block reached through
    `a row col`. A non-positive pivot clears `okVar` and is replaced by
    ε² so the factor stays finite; callers report `okVar`. -/
def cholStmts (pre : String) (k : E) (a : E → E → E) (okVar : String) : List St :=
  let j := v (pre ++ "j"); let t := v (pre ++ "t"); let i := v (pre ++ "i")
  let s := pre ++ "s"; let d := pre ++ "d"; let r := pre ++ "r"
  [ for_ (pre ++ "j") (u 0) k
      [ let_ fT s (a j j)
      , for_ (pre ++ "t") (u 0) j [ setv s (v s - a j t * a j t) ]
      , if_ (le (v s) (fl 0.0)) [ setv okVar (u 0), setv s (fltEps * fltEps) ]
      , let_ fT d (call "sqrt" [v s])
      , set (a j j) (v d)
      , for_ (pre ++ "i") (j + u 1) k
          [ let_ fT r (a i j)
          , for_ (pre ++ "t") (u 0) j [ setv r (v r - a i t * a j t) ]
          , set (a i j) (v r / v d) ] ] ]

/-- `x ← C⁻¹ x` (forward substitution) with `C` lower from `cholStmts`. -/
def fwdStmts (pre : String) (k : E) (c : E → E → E) (x : E → E) : List St :=
  let i := v (pre ++ "i"); let t := v (pre ++ "t"); let r := pre ++ "r"
  [ for_ (pre ++ "i") (u 0) k
      [ let_ fT r (x i)
      , for_ (pre ++ "t") (u 0) i [ setv r (v r - c i t * x t) ]
      , set (x i) (v r / c i i) ] ]

/-- `x ← C⁻ᵀ x` (back substitution). -/
def bwdStmts (pre : String) (k : E) (c : E → E → E) (x : E → E) : List St :=
  let q := pre ++ "q"; let i := v (pre ++ "i"); let t := v (pre ++ "t"); let r := pre ++ "r"
  [ for_ q (u 0) k
      [ let_ uT (pre ++ "i") (k - u 1 - v q)
      , let_ fT r (x i)
      , for_ (pre ++ "t") (i + u 1) k [ setv r (v r - c t i * x t) ]
      , set (x i) (v r / c i i) ] ]

/-- Sift-down for an ascending heapsort of `ord[base .. base+lim)` by
    `key(ord[·])`. `root` is an expression evaluated once. -/
def siftStmts (pre : String) (root lim : E) (ordAt : E → E) (key : E → E) : List St :=
  let r := pre ++ "r"; let c := pre ++ "c"; let go := pre ++ "go"; let tmp := pre ++ "tmp"
  [ let_ uT r root
  , let_ uT go (u 1)
  , while_ (eq (v go) (u 1))
      [ let_ uT c (u 2 * v r + u 1)
      , if_ (ge (v c) lim)
          [ setv go (u 0) ]
          [ if_ (and_ (lt (v c + u 1) lim) (lt (key (ordAt (v c))) (key (ordAt (v c + u 1)))))
              [ setv c (v c + u 1) ]
          , if_ (lt (key (ordAt (v r))) (key (ordAt (v c))))
              [ let_ uT tmp (ordAt (v r))
              , set (ordAt (v r)) (ordAt (v c))
              , set (ordAt (v c)) (v tmp)
              , setv r (v c) ]
              [ setv go (u 0) ] ] ] ]

/-! ## `lb_mv`: w ← M·w (Schur/Cholesky form) -/

/-- `void lb_mv(uint nc, uint mc, inout float w[32])`, reading `bf`. -/
def lbMv : SlangFunctionDecl :=
  let mc := v "mc"; let nc := v "nc"
  let lm := fun (i j : E) => at_ "bf" (lmIx mc i j)
  let rc := fun (i j : E) => at_ "bf" (rcIx mc i j)
  let dd := fun (j : E) => at_ "bf" (dIx mc j)
  let r := fun (i : E) => at_ "r" i
  { name := "lb_mv"
  -- LeanSlang has no array parameter type; the extent rides on the name.
  , params := [arg "nc" uT, arg "mc" uT, arg "w[32]" fT .qInOut]
  , body :=
      [ arr fT "r" maxM
      , for_ "i" (u 0) nc
          [ let_ fT "acc" (at_ "w" (nc + v "i"))
          , for_ "j" (u 0) nc [ setv "acc" (v "acc" + lm (v "i") (v "j") * at_ "w" (v "j") / dd (v "j")) ]
          , setAt "r" (v "i") (v "acc") ] ]
      ++ fwdStmts "f" nc rc r
      ++ bwdStmts "b" nc rc r
      ++ [ for_ "j" (u 0) nc
            [ let_ fT "acc" (-(at_ "w" (v "j")))
            , for_ "i" (u 0) nc [ setv "acc" (v "acc" + lm (v "i") (v "j") * r (v "i")) ]
            , setAt "w" (v "j") (v "acc" / dd (v "j")) ]
         , for_ "i" (u 0) nc [ setAt "w" (nc + v "i") (r (v "i")) ]
         , ret ] }

end Drape.SlangCodegen.Common

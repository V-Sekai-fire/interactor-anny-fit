import Drape.SlangCodegen.Common

/-!
# `Drape.SlangCodegen.LbSubspace` — primal-dual subspace minimization

One thread. LBFGSpp `SubspaceMin::subspace_minimize` (SubspaceMin.h,
the BOXCQP iteration of Voglis and Lagaris), step for step:

1. `drt = xcp − x`; nothing more if the free set F is empty.
2. `c = FᵀBAAᵀd + Fᵀg` (`BFGSMat::compute_FtBAb`, both of its branches:
   `Wᵀ A Aᵀd` directly when |newact| ≤ |F|, else `Wᵀd − W_Fᵀ Fᵀd`).
3. `y = −(FᵀBF)⁻¹ c`; if y is inside `[lb − x, ub − x]` on F, done.
4. Otherwise up to `maxit` rounds of: split F into L / U / P by y and the
   multipliers (LBFGSpp's tie rules), solve
   `y_P = −(PᵀBP)⁻¹ (c_P + PᵀBL·l_L + PᵀBU·u_U)`, recompute λ on L and
   μ on U, stop when λ ≥ 0, μ ≥ 0 and `y_P` is feasible.
5. Not converged: LBFGSpp's three fallbacks (projected y, projected
   unconstrained y, unconstrained y), each kept only if `gᵀdrt ≤ −ε`.

`(PᵀBP)⁻¹ v` is `BFGSMat::solve_PtBP`'s Sherman–Morrison–Woodbury form
`v/θ + W_P·K⁻¹·W_Pᵀv/θ²` (θ on the S half), with
`K = M⁻¹ − W_PᵀW_P/θ = [[−A11, A21ᵀ], [A21, A22]]`,
`A11 = D + Y_PᵀY_P/θ` (SPD), `A21 = L − S_PᵀY_P`,
`A22 = θ·(SᵀS − S_PᵀS_P)`. LBFGSpp factors K with Bunch–Kaufman; here
it is two m×m Cholesky factorizations, of A11 and of the Schur
complement `A22 + A21·A11⁻¹·A21ᵀ`. `SᵀS − S_PᵀS_P` and the lower part of
`L − S_PᵀY_P` are summed directly over the coordinates outside P rather
than subtracted, which is the same quantity without float cancellation.

The PᵀBL and PᵀBU terms are applied as one `M` product of
`W_Lᵀl_L + W_Uᵀu_U` (LBFGSpp applies two; the sum is linear).

Bindings (set 0):

  0   ConstantBuffer<LbSubspaceParams> { uint n; uint mcap; uint maxit; }
  1-4 StructuredBuffer<float>   x, g, lb, ub
  5-6 StructuredBuffer<float>   S, Y
  7   StructuredBuffer<uint>    st
  8   StructuredBuffer<float>   bf
  9   StructuredBuffer<float>   xcp
  10  StructuredBuffer<float>   vecc        (Wᵀ(xcp − x) from lb_cauchy)
  11  RWStructuredBuffer<uint>  iset        (lb_cauchy's sets; writes [4..6])
  12  RWStructuredBuffer<float> drt         (n)
  13  RWStructuredBuffer<float> wk          (≥ 10n)

`wk` regions, by free index k: l at 0, u at n, c at 2n, y at 3n,
y_fallback at 4n, λ at 5n, μ at 6n, class at 7n (0 L, 1 U, 2 P),
right-hand side at 8n; a per-coordinate P mark at 9n.
`iset[4]` = rounds used, `iset[5]` = exit path (0 F empty,
1 unconstrained y feasible, 2 converged, 3/4/5 fallback 1/2/3),
`iset[6]` = 1 if a Cholesky pivot was non-positive.
-/

namespace Drape.SlangCodegen.LbSubspace

open LeanSlang
open Drape.SlangCodegen.Dsl
open Drape.SlangCodegen.Common

private def n : E := v "n"
private def nc : E := v "nc"
private def yAt (j c : E) : E := at_ "Y" (j * n + c)
private def sAt (j c : E) : E := at_ "S" (j * n + c)
private def fv (k : E) : E := at_ "iset" (u 8 + n + k)
private def na (k : E) : E := at_ "iset" (u 8 + k)
private def wkr (r : Nat) (k : E) : E := at_ "wk" (u r * n + k)
private def vl := wkr 0
private def vu := wkr 1
private def vcc := wkr 2
private def vy := wkr 3
private def yfb := wkr 4
private def lam := wkr 5
private def mu := wkr 6
private def cls := wkr 7
private def rhs := wkr 8
private def mark := wkr 9
private def m16 (a : String) (i j : E) : E := at_ a (i * u maxM + j)

/-- `uint lb_solve_p(uint n, uint mc, uint nc, uint nfree, float theta)`:
    `y_P ← (PᵀBP)⁻¹ rhs_P` for P = {k : class k = 2}; returns 0 if a
    Cholesky pivot was non-positive. -/
def solveP : SlangFunctionDecl :=
  let a11 := m16 "A11"; let a21 := m16 "A21"; let a22 := m16 "A22"; let zz := m16 "Z"
  { name := "lb_solve_p", retType := uT
  , params := [arg "n" uT, arg "mc" uT, arg "nc" uT, arg "nfree" uT, arg "theta" fT]
  , body :=
      [ let_ uT "ok" (u 1)
      , let_ uT "nP" (u 0)
      , for_ "k" (u 0) (v "nfree") [ if_ (eq (cls (v "k")) (fl 2.0)) [ setv "nP" (v "nP" + u 1) ] ]
      , if_ (or_ (lt nc (u 1)) (lt (v "nP") (u 1)))
          [ for_ "k" (u 0) (v "nfree")
              [ if_ (eq (cls (v "k")) (fl 2.0)) [ set (vy (v "k")) (rhs (v "k") / v "theta") ] ]
          , .ret (some (v "ok")) ]
      , let_ uT "oldest" ((at_ "st" (u 1) + v "mc" - nc) % v "mc")
      , arr uT "rk" maxM
      , for_ "i" (u 0) nc [ setAt "rk" (v "i") ((v "i" + v "mc" - v "oldest") % v "mc") ]
      , for_ "c" (u 0) n [ set (mark (v "c")) (fl 0.0) ]
      , for_ "k" (u 0) (v "nfree")
          [ if_ (eq (cls (v "k")) (fl 2.0)) [ set (mark (fv (v "k"))) (fl 1.0) ] ]
      , arr fT "A11" (maxM * maxM)
      , arr fT "A21" (maxM * maxM)
      , arr fT "A22" (maxM * maxM)
      , arr fT "Z" (maxM * maxM)
      , arr fT "r1" maxM
      , arr fT "qv" maxM
      , arr fT "r2" maxM
      , arr fT "tv" maxM
      , for_ "i" (u 0) nc
          [ setAt "r1" (v "i") (fl 0.0), setAt "r2" (v "i") (fl 0.0)
          , for_ "j" (u 0) nc
              [ set (a11 (v "i") (v "j")) (fl 0.0), set (a21 (v "i") (v "j")) (fl 0.0)
              , set (a22 (v "i") (v "j")) (fl 0.0) ] ]
      -- Gram blocks, one pass over the coordinates
      , for_ "c" (u 0) n
          [ if_ (eq (mark (v "c")) (fl 1.0))
              [ for_ "i" (u 0) nc
                  [ let_ fT "yi" (yAt (v "i") (v "c"))
                  , let_ fT "si" (sAt (v "i") (v "c"))
                  , for_ "j" (u 0) nc
                      [ let_ fT "yj" (yAt (v "j") (v "c"))
                      , set (a11 (v "i") (v "j")) (a11 (v "i") (v "j") + v "yi" * v "yj")
                      , if_ (le (at_ "rk" (v "i")) (at_ "rk" (v "j")))
                          [ set (a21 (v "i") (v "j")) (a21 (v "i") (v "j") - v "si" * v "yj") ] ] ] ]
              [ for_ "i" (u 0) nc
                  [ let_ fT "si" (sAt (v "i") (v "c"))
                  , for_ "j" (u 0) nc
                      [ set (a22 (v "i") (v "j")) (a22 (v "i") (v "j") + v "si" * sAt (v "j") (v "c"))
                      , if_ (gt (at_ "rk" (v "i")) (at_ "rk" (v "j")))
                          [ set (a21 (v "i") (v "j")) (a21 (v "i") (v "j") + v "si" * yAt (v "j") (v "c")) ] ] ] ] ]
      , for_ "i" (u 0) nc
          [ for_ "j" (u 0) nc
              [ set (a11 (v "i") (v "j")) (a11 (v "i") (v "j") / v "theta")
              , set (a22 (v "i") (v "j")) (v "theta" * a22 (v "i") (v "j")) ]
          , set (a11 (v "i") (v "i")) (a11 (v "i") (v "i") + at_ "bf" (dIx (v "mc") (v "i"))) ]
      -- r = [Y_Pᵀ rhs ; θ S_Pᵀ rhs]
      , for_ "k" (u 0) (v "nfree")
          [ if_ (eq (cls (v "k")) (fl 2.0))
              [ let_ uT "c" (fv (v "k"))
              , let_ fT "rv" (rhs (v "k"))
              , for_ "j" (u 0) nc
                  [ setAt "r1" (v "j") (at_ "r1" (v "j") + yAt (v "j") (v "c") * v "rv")
                  , setAt "r2" (v "j") (at_ "r2" (v "j") + sAt (v "j") (v "c") * v "rv") ] ] ]
      , for_ "j" (u 0) nc [ setAt "r2" (v "j") (v "theta" * at_ "r2" (v "j")) ] ]
      ++ cholStmts "p" nc a11 "ok"
      ++ [ for_ "i" (u 0) nc
            ([ for_ "j" (u 0) nc [ set (zz (v "i") (v "j")) (a21 (v "i") (v "j")) ] ]
             ++ fwdStmts "z" nc a11 (fun t => zz (v "i") t)) ]
      -- Schur complement A22 + Zᵀ-rows products, lower part
      ++ [ for_ "i" (u 0) nc
            [ for_ "k" (u 0) (v "i" + u 1)
                [ let_ fT "acc" (a22 (v "i") (v "k"))
                , for_ "j" (u 0) nc [ setv "acc" (v "acc" + zz (v "i") (v "j") * zz (v "k") (v "j")) ]
                , set (a22 (v "i") (v "k")) (v "acc") ] ]
         , for_ "j" (u 0) nc [ setAt "qv" (v "j") (at_ "r1" (v "j")) ] ]
      ++ fwdStmts "q" nc a11 (at_ "qv")
      ++ [ for_ "i" (u 0) nc
            [ let_ fT "acc" (at_ "r2" (v "i"))
            , for_ "j" (u 0) nc [ setv "acc" (v "acc" + zz (v "i") (v "j") * at_ "qv" (v "j")) ]
            , setAt "r2" (v "i") (v "acc") ] ]
      ++ cholStmts "s" nc a22 "ok"
      ++ fwdStmts "sf" nc a22 (at_ "r2")
      ++ bwdStmts "sb" nc a22 (at_ "r2")
      ++ [ for_ "j" (u 0) nc
            [ let_ fT "acc" (-(at_ "r1" (v "j")))
            , for_ "i" (u 0) nc [ setv "acc" (v "acc" + a21 (v "i") (v "j") * at_ "r2" (v "i")) ]
            , setAt "tv" (v "j") (v "acc") ] ]
      ++ fwdStmts "tf" nc a11 (at_ "tv")
      ++ bwdStmts "tb" nc a11 (at_ "tv")
      ++ [ for_ "k" (u 0) (v "nfree")
            [ if_ (eq (cls (v "k")) (fl 2.0))
                [ let_ uT "c" (fv (v "k"))
                , let_ fT "acc" (fl 0.0)
                , for_ "j" (u 0) nc
                    [ setv "acc" (v "acc" + yAt (v "j") (v "c") * at_ "tv" (v "j")
                        + sAt (v "j") (v "c") * (v "theta" * at_ "r2" (v "j"))) ]
                , set (vy (v "k")) (rhs (v "k") / v "theta" + v "acc" / (v "theta" * v "theta")) ] ]
         , .ret (some (v "ok")) ] }

/-- `w[0..2nc) ← [Σ Y(j,c)·val ; θ Σ S(j,c)·val]` over free k where
    `pick k` gives `(include?, val)`. -/
private def wtv (w : String) (incl : E → E) (val : E → E) : List St :=
  [ for_ "j" (u 0) nc
      [ let_ fT "dy" (fl 0.0)
      , let_ fT "ds" (fl 0.0)
      , for_ "k" (u 0) (v "nfree")
          [ if_ (incl (v "k"))
              [ let_ uT "c" (fv (v "k"))
              , setv "dy" (v "dy" + yAt (v "j") (v "c") * val (v "k"))
              , setv "ds" (v "ds" + sAt (v "j") (v "c") * val (v "k")) ] ]
      , setAt w (v "j") (v "dy")
      , setAt w (nc + v "j") (v "theta" * v "ds") ] ]

/-- `M·w` then θ on the S half (`apply_PtWMv`'s `Mv.tail *= theta`). -/
private def mThenTheta (w : String) : List St :=
  [ do_ (call "lb_mv" [nc, v "mc", v w])
  , for_ "j" (u 0) nc [ setAt w (nc + v "j") (v "theta" * at_ w (nc + v "j")) ] ]

/-- `Σ_j Y(j,c)·w[j] + S(j,c)·w[nc+j]` into `acc`. -/
private def wRow (w : String) (c : E) : List St :=
  [ let_ fT "acc" (fl 0.0)
  , for_ "j" (u 0) nc
      [ setv "acc" (v "acc" + yAt (v "j") c * at_ w (v "j") + sAt (v "j") c * at_ w (nc + v "j")) ] ]

private def ftbab : List St :=
  [ for_ "k" (u 0) (v "nfree") [ set (vcc (v "k")) (fl 0.0) ]
  , arr fT "rh" (2 * maxM)
  , for_ "j" (u 0) (u (2 * maxM)) [ setAt "rh" (v "j") (fl 0.0) ]
  , if_ (and_ (gt nc (u 0)) (gt (v "nact") (u 0)))
      ([ if_ (le (v "nact") (v "nfree"))
          [ for_ "j" (u 0) nc
              [ let_ fT "dy" (fl 0.0)
              , let_ fT "ds" (fl 0.0)
              , for_ "q" (u 0) (v "nact")
                  [ let_ uT "c" (na (v "q"))
                  , setv "dy" (v "dy" + yAt (v "j") (v "c") * at_ "drt" (v "c"))
                  , setv "ds" (v "ds" + sAt (v "j") (v "c") * at_ "drt" (v "c")) ]
              , setAt "rh" (v "j") (v "dy")
              , setAt "rh" (nc + v "j") (v "theta" * v "ds") ] ]
          [ for_ "j" (u 0) nc
              [ let_ fT "dy" (fl 0.0)
              , let_ fT "ds" (fl 0.0)
              , for_ "q" (u 0) (v "nfree")
                  [ let_ uT "c" (fv (v "q"))
                  , setv "dy" (v "dy" + yAt (v "j") (v "c") * at_ "drt" (v "c"))
                  , setv "ds" (v "ds" + sAt (v "j") (v "c") * at_ "drt" (v "c")) ]
              , setAt "rh" (v "j") (at_ "vecc" (v "j") - v "dy")
              , setAt "rh" (nc + v "j") (at_ "vecc" (nc + v "j") - v "theta" * v "ds") ] ] ]
      ++ mThenTheta "rh" ++
      [ for_ "k" (u 0) (v "nfree") (wRow "rh" (fv (v "k")) ++ [ set (vcc (v "k")) (-(v "acc")) ]) ]) ]

private def classify : List St :=
  [ setv "nL" (u 0), setv "nU" (u 0), setv "nP" (u 0)
  , for_ "k" (u 0) (v "nfree")
      [ let_ fT "yk" (vy (v "k"))
      , let_ fT "lk" (vl (v "k"))
      , let_ fT "uk" (vu (v "k"))
      , if_ (or_ (lt (v "yk") (v "lk")) (and_ (eq (v "yk") (v "lk")) (ge (lam (v "k")) (fl 0.0))))
          [ set (cls (v "k")) (fl 0.0), set (vy (v "k")) (v "lk"), set (mu (v "k")) (fl 0.0)
          , setv "nL" (v "nL" + u 1) ]
          [ if_ (or_ (gt (v "yk") (v "uk")) (and_ (eq (v "yk") (v "uk")) (ge (mu (v "k")) (fl 0.0))))
              [ set (cls (v "k")) (fl 1.0), set (vy (v "k")) (v "uk"), set (lam (v "k")) (fl 0.0)
              , setv "nU" (v "nU" + u 1) ]
              [ set (cls (v "k")) (fl 2.0), set (lam (v "k")) (fl 0.0), set (mu (v "k")) (fl 0.0)
              , setv "nP" (v "nP" + u 1) ] ] ] ]

private def solveRound : List St :=
  classify ++
  [ if_ (gt (v "nP") (u 0))
      ([ arr fT "wq" (2 * maxM) ]
       ++ wtv "wq" (fun k => lt (cls k) (fl 2.0)) (fun k => sel (eq (cls k) (fl 0.0)) (vl k) (vu k))
       ++ mThenTheta "wq" ++
       [ for_ "k" (u 0) (v "nfree")
           [ if_ (eq (cls (v "k")) (fl 2.0))
               (wRow "wq" (fv (v "k")) ++ [ set (rhs (v "k")) (-(vcc (v "k") - v "acc")) ]) ]
       , if_ (eq (call "lb_solve_p" [n, v "mc", nc, v "nfree", v "theta"]) (u 0))
           [ setAt "iset" (u 6) (u 1) ] ])
  , if_ (or_ (gt (v "nL") (u 0)) (gt (v "nU") (u 0)))
      ([ arr fT "fy" (2 * maxM) ]
       ++ wtv "fy" (fun _ => .litBool true) vy
       ++ mThenTheta "fy" ++
       [ for_ "k" (u 0) (v "nfree")
           [ if_ (lt (cls (v "k")) (fl 2.0))
               (wRow "fy" (fv (v "k")) ++
                [ let_ fT "res" (-(v "acc") + vcc (v "k") + v "theta" * vy (v "k"))
                , if_ (eq (cls (v "k")) (fl 0.0))
                    [ set (lam (v "k")) (v "res") ]
                    [ set (mu (v "k")) (-(v "res")) ] ]) ] ])
  , let_ uT "cv" (u 1)
  , for_ "k" (u 0) (v "nfree")
      [ if_ (and_ (eq (cls (v "k")) (fl 0.0)) (lt (lam (v "k")) (fl 0.0))) [ setv "cv" (u 0) ]
      , if_ (and_ (eq (cls (v "k")) (fl 1.0)) (lt (mu (v "k")) (fl 0.0))) [ setv "cv" (u 0) ]
      , if_ (and_ (eq (cls (v "k")) (fl 2.0))
            (or_ (lt (vy (v "k")) (vl (v "k"))) (gt (vy (v "k")) (vu (v "k")))))
          [ setv "cv" (u 0) ] ]
  , if_ (eq (v "cv") (u 1)) [ setv "conv" (u 1) ] [ setv "kk" (v "kk" + u 1) ] ]

/-- `dg = gᵀdrt` over all n, df32. -/
private def dgStmts : List St :=
  [ setv "a_hi" (fl 0.0)
  , setv "a_lo" (fl 0.0)
  , for_ "i" (u 0) n [ do_ (call "df_acc" [v "a_hi", v "a_lo", at_ "drt" (v "i"), at_ "g" (v "i")]) ]
  , setv "dg" (v "a_hi" + v "a_lo") ]

private def assignDrt (val : E → E) : St :=
  for_ "k" (u 0) (v "nfree") [ setAt "drt" (fv (v "k")) (val (v "k")) ]

private def clampK (a : E → E) (k : E) : E := fmin (fmax (a k) (vl k)) (vu k)

private def fallbacks : List St :=
  [ decl fT "a_hi", decl fT "a_lo", let_ fT "dg" (fl 0.0)
  , for_ "k" (u 0) (v "nfree") [ set (vy (v "k")) (clampK vy (v "k")) ]
  , assignDrt vy ]
  ++ dgStmts ++
  [ if_ (le (v "dg") (-dblEps))
      [ setAt "iset" (u 5) (u 3) ]
      ([ assignDrt (clampK yfb) ] ++ dgStmts ++
       [ if_ (le (v "dg") (-dblEps))
           [ setAt "iset" (u 5) (u 4) ]
           [ assignDrt yfb, setAt "iset" (u 5) (u 5) ] ]) ]

def shader : SlangShaderModule :=
  { structs := [ { name := "LbSubspaceParams", fields := [fld "n" uT, fld "mcap" uT, fld "maxit" uT] } ]
  , globals :=
      [ paramsCB "LbSubspaceParams", roF "x" 1, roF "g" 2, roF "lb" 3, roF "ub" 4
      , roF "S" 5, roF "Y" 6, roU "st" 7, roF "bf" 8, roF "xcp" 9, roF "vecc" 10
      , rwU "iset" 11, rwF "drt" 12, rwF "wk" 13 ]
  , functions := dfHelpers ++
      [ lbMv, solveP
      , entry 1 [dtid] (
          [ let_ uT "n" (p "n")
          , let_ uT "mc" (p "mcap")
          , let_ uT "nc" (at_ "st" (u 0))
          , let_ fT "theta" (at_ "bf" (u 0))
          , let_ uT "nact" (at_ "iset" (u 0))
          , let_ uT "nfree" (at_ "iset" (u 1))
          , setAt "iset" (u 4) (u 0)
          , setAt "iset" (u 5) (u 0)
          , setAt "iset" (u 6) (u 0)
          , for_ "i" (u 0) n [ setAt "drt" (v "i") (at_ "xcp" (v "i") - at_ "x" (v "i")) ]
          , if_ (lt (v "nfree") (u 1)) [ ret ] ]
          ++ ftbab ++
          [ for_ "k" (u 0) (v "nfree")
              [ let_ uT "c" (fv (v "k"))
              , set (vl (v "k")) (at_ "lb" (v "c") - at_ "x" (v "c"))
              , set (vu (v "k")) (at_ "ub" (v "c") - at_ "x" (v "c"))
              , set (vcc (v "k")) (vcc (v "k") + at_ "g" (v "c"))
              , set (cls (v "k")) (fl 2.0)
              , set (rhs (v "k")) (-(vcc (v "k"))) ]
          , if_ (eq (call "lb_solve_p" [n, v "mc", nc, v "nfree", v "theta"]) (u 0))
              [ setAt "iset" (u 6) (u 1) ]
          , let_ uT "inb" (u 1)
          , for_ "k" (u 0) (v "nfree")
              [ if_ (or_ (lt (vy (v "k")) (vl (v "k"))) (gt (vy (v "k")) (vu (v "k")))) [ setv "inb" (u 0) ] ]
          , if_ (eq (v "inb") (u 1)) [ assignDrt vy, setAt "iset" (u 5) (u 1), ret ]
          , for_ "k" (u 0) (v "nfree")
              [ set (yfb (v "k")) (vy (v "k")), set (lam (v "k")) (fl 0.0), set (mu (v "k")) (fl 0.0) ]
          , let_ uT "nL" (u 0)
          , let_ uT "nU" (u 0)
          , let_ uT "nP" (u 0)
          , let_ uT "kk" (u 0)
          , let_ uT "conv" (u 0)
          , while_ (and_ (eq (v "conv") (u 0)) (lt (v "kk") (p "maxit"))) solveRound
          , setAt "iset" (u 4) (v "kk")
          , if_ (eq (v "conv") (u 1)) [ assignDrt vy, setAt "iset" (u 5) (u 2), ret ] ]
          ++ fallbacks) ] }

-- BEGIN PIN
def expected : String :=
"struct LbSubspaceParams {
  uint n;
  uint mcap;
  uint maxit;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LbSubspaceParams> params;
[[vk::binding(1, 0)]]
StructuredBuffer<float> x;
[[vk::binding(2, 0)]]
StructuredBuffer<float> g;
[[vk::binding(3, 0)]]
StructuredBuffer<float> lb;
[[vk::binding(4, 0)]]
StructuredBuffer<float> ub;
[[vk::binding(5, 0)]]
StructuredBuffer<float> S;
[[vk::binding(6, 0)]]
StructuredBuffer<float> Y;
[[vk::binding(7, 0)]]
StructuredBuffer<uint> st;
[[vk::binding(8, 0)]]
StructuredBuffer<float> bf;
[[vk::binding(9, 0)]]
StructuredBuffer<float> xcp;
[[vk::binding(10, 0)]]
StructuredBuffer<float> vecc;
[[vk::binding(11, 0)]]
RWStructuredBuffer<uint> iset;
[[vk::binding(12, 0)]]
RWStructuredBuffer<float> drt;
[[vk::binding(13, 0)]]
RWStructuredBuffer<float> wk;

void two_sum(float a, float b, out float hi, out float lo) {
  float h = (a + b);
  float bb = (h - a);
  float ah = (h - bb);
  float lo_a = (a - ah);
  float lo_b = (b - bb);
  hi = h;
  lo = (lo_a + lo_b);
  return;
}

void quick_two_sum(float a, float b, out float hi, out float lo) {
  float h = (a + b);
  float t = (h - a);
  hi = h;
  lo = (b - t);
  return;
}

void two_prod(float a, float b, out float hi, out float lo) {
  float h = (a * b);
  hi = h;
  lo = fma(a, b, (-h));
  return;
}

void df_add(float x_hi, float x_lo, float y_hi, float y_lo, out float z_hi, out float z_lo) {
  float sh;
  float sl;
  two_sum(x_hi, y_hi, sh, sl);
  float xy_lo = (x_lo + y_lo);
  float sl2 = (sl + xy_lo);
  quick_two_sum(sh, sl2, z_hi, z_lo);
  return;
}

void df_acc(inout float hi, inout float lo, float a, float b) {
  float p_hi;
  float p_lo;
  two_prod(a, b, p_hi, p_lo);
  float n_hi;
  float n_lo;
  df_add(hi, lo, p_hi, p_lo, n_hi, n_lo);
  hi = n_hi;
  lo = n_lo;
  return;
}

void lb_mv(uint nc, uint mc, inout float w[32]) {
  float r[16];
  for (uint i = 0u; i < nc; ++i) {
    float acc = w[(nc + i)];
    for (uint j = 0u; j < nc; ++j) {
      acc = (acc + ((bf[(((4u + ((2u * mc) * mc)) + (i * mc)) + j)] * w[j]) / bf[((4u + ((4u * mc) * mc)) + j)]));
    }
    r[i] = acc;
  }
  for (uint fi = 0u; fi < nc; ++fi) {
    float fr = r[fi];
    for (uint ft = 0u; ft < fi; ++ft) {
      fr = (fr - (bf[(((4u + ((3u * mc) * mc)) + (fi * mc)) + ft)] * r[ft]));
    }
    r[fi] = (fr / bf[(((4u + ((3u * mc) * mc)) + (fi * mc)) + fi)]);
  }
  for (uint bq = 0u; bq < nc; ++bq) {
    uint bi = ((nc - 1u) - bq);
    float br = r[bi];
    for (uint bt = (bi + 1u); bt < nc; ++bt) {
      br = (br - (bf[(((4u + ((3u * mc) * mc)) + (bt * mc)) + bi)] * r[bt]));
    }
    r[bi] = (br / bf[(((4u + ((3u * mc) * mc)) + (bi * mc)) + bi)]);
  }
  for (uint j = 0u; j < nc; ++j) {
    float acc = (-w[j]);
    for (uint i = 0u; i < nc; ++i) {
      acc = (acc + (bf[(((4u + ((2u * mc) * mc)) + (i * mc)) + j)] * r[i]));
    }
    w[j] = (acc / bf[((4u + ((4u * mc) * mc)) + j)]);
  }
  for (uint i = 0u; i < nc; ++i) {
    w[(nc + i)] = r[i];
  }
  return;
}

uint lb_solve_p(uint n, uint mc, uint nc, uint nfree, float theta) {
  uint ok = 1u;
  uint nP = 0u;
  for (uint k = 0u; k < nfree; ++k) {
    if ((wk[((7u * n) + k)] == 2.000000)) {
      nP = (nP + 1u);
    }
  }
  if (((nc < 1u) || (nP < 1u))) {
    for (uint k = 0u; k < nfree; ++k) {
      if ((wk[((7u * n) + k)] == 2.000000)) {
        wk[((3u * n) + k)] = (wk[((8u * n) + k)] / theta);
      }
    }
    return ok;
  }
  uint oldest = (((st[1u] + mc) - nc) % mc);
  uint rk[16];
  for (uint i = 0u; i < nc; ++i) {
    rk[i] = (((i + mc) - oldest) % mc);
  }
  for (uint c = 0u; c < n; ++c) {
    wk[((9u * n) + c)] = 0.000000;
  }
  for (uint k = 0u; k < nfree; ++k) {
    if ((wk[((7u * n) + k)] == 2.000000)) {
      wk[((9u * n) + iset[((8u + n) + k)])] = 1.000000;
    }
  }
  float A11[256];
  float A21[256];
  float A22[256];
  float Z[256];
  float r1[16];
  float qv[16];
  float r2[16];
  float tv[16];
  for (uint i = 0u; i < nc; ++i) {
    r1[i] = 0.000000;
    r2[i] = 0.000000;
    for (uint j = 0u; j < nc; ++j) {
      A11[((i * 16u) + j)] = 0.000000;
      A21[((i * 16u) + j)] = 0.000000;
      A22[((i * 16u) + j)] = 0.000000;
    }
  }
  for (uint c = 0u; c < n; ++c) {
    if ((wk[((9u * n) + c)] == 1.000000)) {
      for (uint i = 0u; i < nc; ++i) {
        float yi = Y[((i * n) + c)];
        float si = S[((i * n) + c)];
        for (uint j = 0u; j < nc; ++j) {
          float yj = Y[((j * n) + c)];
          A11[((i * 16u) + j)] = (A11[((i * 16u) + j)] + (yi * yj));
          if ((rk[i] <= rk[j])) {
            A21[((i * 16u) + j)] = (A21[((i * 16u) + j)] - (si * yj));
          }
        }
      }
    } else {
      for (uint i = 0u; i < nc; ++i) {
        float si = S[((i * n) + c)];
        for (uint j = 0u; j < nc; ++j) {
          A22[((i * 16u) + j)] = (A22[((i * 16u) + j)] + (si * S[((j * n) + c)]));
          if ((rk[i] > rk[j])) {
            A21[((i * 16u) + j)] = (A21[((i * 16u) + j)] + (si * Y[((j * n) + c)]));
          }
        }
      }
    }
  }
  for (uint i = 0u; i < nc; ++i) {
    for (uint j = 0u; j < nc; ++j) {
      A11[((i * 16u) + j)] = (A11[((i * 16u) + j)] / theta);
      A22[((i * 16u) + j)] = (theta * A22[((i * 16u) + j)]);
    }
    A11[((i * 16u) + i)] = (A11[((i * 16u) + i)] + bf[((4u + ((4u * mc) * mc)) + i)]);
  }
  for (uint k = 0u; k < nfree; ++k) {
    if ((wk[((7u * n) + k)] == 2.000000)) {
      uint c = iset[((8u + n) + k)];
      float rv = wk[((8u * n) + k)];
      for (uint j = 0u; j < nc; ++j) {
        r1[j] = (r1[j] + (Y[((j * n) + c)] * rv));
        r2[j] = (r2[j] + (S[((j * n) + c)] * rv));
      }
    }
  }
  for (uint j = 0u; j < nc; ++j) {
    r2[j] = (theta * r2[j]);
  }
  for (uint pj = 0u; pj < nc; ++pj) {
    float ps = A11[((pj * 16u) + pj)];
    for (uint pt = 0u; pt < pj; ++pt) {
      ps = (ps - (A11[((pj * 16u) + pt)] * A11[((pj * 16u) + pt)]));
    }
    if ((ps <= 0.000000)) {
      ok = 0u;
      ps = (asfloat(872415232u) * asfloat(872415232u));
    }
    float pd = sqrt(ps);
    A11[((pj * 16u) + pj)] = pd;
    for (uint pi = (pj + 1u); pi < nc; ++pi) {
      float pr = A11[((pi * 16u) + pj)];
      for (uint pt = 0u; pt < pj; ++pt) {
        pr = (pr - (A11[((pi * 16u) + pt)] * A11[((pj * 16u) + pt)]));
      }
      A11[((pi * 16u) + pj)] = (pr / pd);
    }
  }
  for (uint i = 0u; i < nc; ++i) {
    for (uint j = 0u; j < nc; ++j) {
      Z[((i * 16u) + j)] = A21[((i * 16u) + j)];
    }
    for (uint zi = 0u; zi < nc; ++zi) {
      float zr = Z[((i * 16u) + zi)];
      for (uint zt = 0u; zt < zi; ++zt) {
        zr = (zr - (A11[((zi * 16u) + zt)] * Z[((i * 16u) + zt)]));
      }
      Z[((i * 16u) + zi)] = (zr / A11[((zi * 16u) + zi)]);
    }
  }
  for (uint i = 0u; i < nc; ++i) {
    for (uint k = 0u; k < (i + 1u); ++k) {
      float acc = A22[((i * 16u) + k)];
      for (uint j = 0u; j < nc; ++j) {
        acc = (acc + (Z[((i * 16u) + j)] * Z[((k * 16u) + j)]));
      }
      A22[((i * 16u) + k)] = acc;
    }
  }
  for (uint j = 0u; j < nc; ++j) {
    qv[j] = r1[j];
  }
  for (uint qi = 0u; qi < nc; ++qi) {
    float qr = qv[qi];
    for (uint qt = 0u; qt < qi; ++qt) {
      qr = (qr - (A11[((qi * 16u) + qt)] * qv[qt]));
    }
    qv[qi] = (qr / A11[((qi * 16u) + qi)]);
  }
  for (uint i = 0u; i < nc; ++i) {
    float acc = r2[i];
    for (uint j = 0u; j < nc; ++j) {
      acc = (acc + (Z[((i * 16u) + j)] * qv[j]));
    }
    r2[i] = acc;
  }
  for (uint sj = 0u; sj < nc; ++sj) {
    float ss = A22[((sj * 16u) + sj)];
    for (uint st = 0u; st < sj; ++st) {
      ss = (ss - (A22[((sj * 16u) + st)] * A22[((sj * 16u) + st)]));
    }
    if ((ss <= 0.000000)) {
      ok = 0u;
      ss = (asfloat(872415232u) * asfloat(872415232u));
    }
    float sd = sqrt(ss);
    A22[((sj * 16u) + sj)] = sd;
    for (uint si = (sj + 1u); si < nc; ++si) {
      float sr = A22[((si * 16u) + sj)];
      for (uint st = 0u; st < sj; ++st) {
        sr = (sr - (A22[((si * 16u) + st)] * A22[((sj * 16u) + st)]));
      }
      A22[((si * 16u) + sj)] = (sr / sd);
    }
  }
  for (uint sfi = 0u; sfi < nc; ++sfi) {
    float sfr = r2[sfi];
    for (uint sft = 0u; sft < sfi; ++sft) {
      sfr = (sfr - (A22[((sfi * 16u) + sft)] * r2[sft]));
    }
    r2[sfi] = (sfr / A22[((sfi * 16u) + sfi)]);
  }
  for (uint sbq = 0u; sbq < nc; ++sbq) {
    uint sbi = ((nc - 1u) - sbq);
    float sbr = r2[sbi];
    for (uint sbt = (sbi + 1u); sbt < nc; ++sbt) {
      sbr = (sbr - (A22[((sbt * 16u) + sbi)] * r2[sbt]));
    }
    r2[sbi] = (sbr / A22[((sbi * 16u) + sbi)]);
  }
  for (uint j = 0u; j < nc; ++j) {
    float acc = (-r1[j]);
    for (uint i = 0u; i < nc; ++i) {
      acc = (acc + (A21[((i * 16u) + j)] * r2[i]));
    }
    tv[j] = acc;
  }
  for (uint tfi = 0u; tfi < nc; ++tfi) {
    float tfr = tv[tfi];
    for (uint tft = 0u; tft < tfi; ++tft) {
      tfr = (tfr - (A11[((tfi * 16u) + tft)] * tv[tft]));
    }
    tv[tfi] = (tfr / A11[((tfi * 16u) + tfi)]);
  }
  for (uint tbq = 0u; tbq < nc; ++tbq) {
    uint tbi = ((nc - 1u) - tbq);
    float tbr = tv[tbi];
    for (uint tbt = (tbi + 1u); tbt < nc; ++tbt) {
      tbr = (tbr - (A11[((tbt * 16u) + tbi)] * tv[tbt]));
    }
    tv[tbi] = (tbr / A11[((tbi * 16u) + tbi)]);
  }
  for (uint k = 0u; k < nfree; ++k) {
    if ((wk[((7u * n) + k)] == 2.000000)) {
      uint c = iset[((8u + n) + k)];
      float acc = 0.000000;
      for (uint j = 0u; j < nc; ++j) {
        acc = ((acc + (Y[((j * n) + c)] * tv[j])) + (S[((j * n) + c)] * (theta * r2[j])));
      }
      wk[((3u * n) + k)] = ((wk[((8u * n) + k)] / theta) + (acc / (theta * theta)));
    }
  }
  return ok;
}

[shader(\"compute\")] [numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint n = params.n;
  uint mc = params.mcap;
  uint nc = st[0u];
  float theta = bf[0u];
  uint nact = iset[0u];
  uint nfree = iset[1u];
  iset[4u] = 0u;
  iset[5u] = 0u;
  iset[6u] = 0u;
  for (uint i = 0u; i < n; ++i) {
    drt[i] = (xcp[i] - x[i]);
  }
  if ((nfree < 1u)) {
    return;
  }
  for (uint k = 0u; k < nfree; ++k) {
    wk[((2u * n) + k)] = 0.000000;
  }
  float rh[32];
  for (uint j = 0u; j < 32u; ++j) {
    rh[j] = 0.000000;
  }
  if (((nc > 0u) && (nact > 0u))) {
    if ((nact <= nfree)) {
      for (uint j = 0u; j < nc; ++j) {
        float dy = 0.000000;
        float ds = 0.000000;
        for (uint q = 0u; q < nact; ++q) {
          uint c = iset[(8u + q)];
          dy = (dy + (Y[((j * n) + c)] * drt[c]));
          ds = (ds + (S[((j * n) + c)] * drt[c]));
        }
        rh[j] = dy;
        rh[(nc + j)] = (theta * ds);
      }
    } else {
      for (uint j = 0u; j < nc; ++j) {
        float dy = 0.000000;
        float ds = 0.000000;
        for (uint q = 0u; q < nfree; ++q) {
          uint c = iset[((8u + n) + q)];
          dy = (dy + (Y[((j * n) + c)] * drt[c]));
          ds = (ds + (S[((j * n) + c)] * drt[c]));
        }
        rh[j] = (vecc[j] - dy);
        rh[(nc + j)] = (vecc[(nc + j)] - (theta * ds));
      }
    }
    lb_mv(nc, mc, rh);
    for (uint j = 0u; j < nc; ++j) {
      rh[(nc + j)] = (theta * rh[(nc + j)]);
    }
    for (uint k = 0u; k < nfree; ++k) {
      float acc = 0.000000;
      for (uint j = 0u; j < nc; ++j) {
        acc = ((acc + (Y[((j * n) + iset[((8u + n) + k)])] * rh[j])) + (S[((j * n) + iset[((8u + n) + k)])] * rh[(nc + j)]));
      }
      wk[((2u * n) + k)] = (-acc);
    }
  }
  for (uint k = 0u; k < nfree; ++k) {
    uint c = iset[((8u + n) + k)];
    wk[((0u * n) + k)] = (lb[c] - x[c]);
    wk[((1u * n) + k)] = (ub[c] - x[c]);
    wk[((2u * n) + k)] = (wk[((2u * n) + k)] + g[c]);
    wk[((7u * n) + k)] = 2.000000;
    wk[((8u * n) + k)] = (-wk[((2u * n) + k)]);
  }
  if ((lb_solve_p(n, mc, nc, nfree, theta) == 0u)) {
    iset[6u] = 1u;
  }
  uint inb = 1u;
  for (uint k = 0u; k < nfree; ++k) {
    if (((wk[((3u * n) + k)] < wk[((0u * n) + k)]) || (wk[((3u * n) + k)] > wk[((1u * n) + k)]))) {
      inb = 0u;
    }
  }
  if ((inb == 1u)) {
    for (uint k = 0u; k < nfree; ++k) {
      drt[iset[((8u + n) + k)]] = wk[((3u * n) + k)];
    }
    iset[5u] = 1u;
    return;
  }
  for (uint k = 0u; k < nfree; ++k) {
    wk[((4u * n) + k)] = wk[((3u * n) + k)];
    wk[((5u * n) + k)] = 0.000000;
    wk[((6u * n) + k)] = 0.000000;
  }
  uint nL = 0u;
  uint nU = 0u;
  uint nP = 0u;
  uint kk = 0u;
  uint conv = 0u;
  while (((conv == 0u) && (kk < params.maxit))) {
    nL = 0u;
    nU = 0u;
    nP = 0u;
    for (uint k = 0u; k < nfree; ++k) {
      float yk = wk[((3u * n) + k)];
      float lk = wk[((0u * n) + k)];
      float uk = wk[((1u * n) + k)];
      if (((yk < lk) || ((yk == lk) && (wk[((5u * n) + k)] >= 0.000000)))) {
        wk[((7u * n) + k)] = 0.000000;
        wk[((3u * n) + k)] = lk;
        wk[((6u * n) + k)] = 0.000000;
        nL = (nL + 1u);
      } else {
        if (((yk > uk) || ((yk == uk) && (wk[((6u * n) + k)] >= 0.000000)))) {
          wk[((7u * n) + k)] = 1.000000;
          wk[((3u * n) + k)] = uk;
          wk[((5u * n) + k)] = 0.000000;
          nU = (nU + 1u);
        } else {
          wk[((7u * n) + k)] = 2.000000;
          wk[((5u * n) + k)] = 0.000000;
          wk[((6u * n) + k)] = 0.000000;
          nP = (nP + 1u);
        }
      }
    }
    if ((nP > 0u)) {
      float wq[32];
      for (uint j = 0u; j < nc; ++j) {
        float dy = 0.000000;
        float ds = 0.000000;
        for (uint k = 0u; k < nfree; ++k) {
          if ((wk[((7u * n) + k)] < 2.000000)) {
            uint c = iset[((8u + n) + k)];
            dy = (dy + (Y[((j * n) + c)] * ((wk[((7u * n) + k)] == 0.000000) ? wk[((0u * n) + k)] : wk[((1u * n) + k)])));
            ds = (ds + (S[((j * n) + c)] * ((wk[((7u * n) + k)] == 0.000000) ? wk[((0u * n) + k)] : wk[((1u * n) + k)])));
          }
        }
        wq[j] = dy;
        wq[(nc + j)] = (theta * ds);
      }
      lb_mv(nc, mc, wq);
      for (uint j = 0u; j < nc; ++j) {
        wq[(nc + j)] = (theta * wq[(nc + j)]);
      }
      for (uint k = 0u; k < nfree; ++k) {
        if ((wk[((7u * n) + k)] == 2.000000)) {
          float acc = 0.000000;
          for (uint j = 0u; j < nc; ++j) {
            acc = ((acc + (Y[((j * n) + iset[((8u + n) + k)])] * wq[j])) + (S[((j * n) + iset[((8u + n) + k)])] * wq[(nc + j)]));
          }
          wk[((8u * n) + k)] = (-(wk[((2u * n) + k)] - acc));
        }
      }
      if ((lb_solve_p(n, mc, nc, nfree, theta) == 0u)) {
        iset[6u] = 1u;
      }
    }
    if (((nL > 0u) || (nU > 0u))) {
      float fy[32];
      for (uint j = 0u; j < nc; ++j) {
        float dy = 0.000000;
        float ds = 0.000000;
        for (uint k = 0u; k < nfree; ++k) {
          if (true) {
            uint c = iset[((8u + n) + k)];
            dy = (dy + (Y[((j * n) + c)] * wk[((3u * n) + k)]));
            ds = (ds + (S[((j * n) + c)] * wk[((3u * n) + k)]));
          }
        }
        fy[j] = dy;
        fy[(nc + j)] = (theta * ds);
      }
      lb_mv(nc, mc, fy);
      for (uint j = 0u; j < nc; ++j) {
        fy[(nc + j)] = (theta * fy[(nc + j)]);
      }
      for (uint k = 0u; k < nfree; ++k) {
        if ((wk[((7u * n) + k)] < 2.000000)) {
          float acc = 0.000000;
          for (uint j = 0u; j < nc; ++j) {
            acc = ((acc + (Y[((j * n) + iset[((8u + n) + k)])] * fy[j])) + (S[((j * n) + iset[((8u + n) + k)])] * fy[(nc + j)]));
          }
          float res = (((-acc) + wk[((2u * n) + k)]) + (theta * wk[((3u * n) + k)]));
          if ((wk[((7u * n) + k)] == 0.000000)) {
            wk[((5u * n) + k)] = res;
          } else {
            wk[((6u * n) + k)] = (-res);
          }
        }
      }
    }
    uint cv = 1u;
    for (uint k = 0u; k < nfree; ++k) {
      if (((wk[((7u * n) + k)] == 0.000000) && (wk[((5u * n) + k)] < 0.000000))) {
        cv = 0u;
      }
      if (((wk[((7u * n) + k)] == 1.000000) && (wk[((6u * n) + k)] < 0.000000))) {
        cv = 0u;
      }
      if (((wk[((7u * n) + k)] == 2.000000) && ((wk[((3u * n) + k)] < wk[((0u * n) + k)]) || (wk[((3u * n) + k)] > wk[((1u * n) + k)])))) {
        cv = 0u;
      }
    }
    if ((cv == 1u)) {
      conv = 1u;
    } else {
      kk = (kk + 1u);
    }
  }
  iset[4u] = kk;
  if ((conv == 1u)) {
    for (uint k = 0u; k < nfree; ++k) {
      drt[iset[((8u + n) + k)]] = wk[((3u * n) + k)];
    }
    iset[5u] = 2u;
    return;
  }
  float a_hi;
  float a_lo;
  float dg = 0.000000;
  for (uint k = 0u; k < nfree; ++k) {
    wk[((3u * n) + k)] = min(max(wk[((3u * n) + k)], wk[((0u * n) + k)]), wk[((1u * n) + k)]);
  }
  for (uint k = 0u; k < nfree; ++k) {
    drt[iset[((8u + n) + k)]] = wk[((3u * n) + k)];
  }
  a_hi = 0.000000;
  a_lo = 0.000000;
  for (uint i = 0u; i < n; ++i) {
    df_acc(a_hi, a_lo, drt[i], g[i]);
  }
  dg = (a_hi + a_lo);
  if ((dg <= (-asfloat(629145600u)))) {
    iset[5u] = 3u;
  } else {
    for (uint k = 0u; k < nfree; ++k) {
      drt[iset[((8u + n) + k)]] = min(max(wk[((4u * n) + k)], wk[((0u * n) + k)]), wk[((1u * n) + k)]);
    }
    a_hi = 0.000000;
    a_lo = 0.000000;
    for (uint i = 0u; i < n; ++i) {
      df_acc(a_hi, a_lo, drt[i], g[i]);
    }
    dg = (a_hi + a_lo);
    if ((dg <= (-asfloat(629145600u)))) {
      iset[5u] = 4u;
    } else {
      for (uint k = 0u; k < nfree; ++k) {
        drt[iset[((8u + n) + k)]] = wk[((4u * n) + k)];
      }
      iset[5u] = 5u;
    }
  }
}"

example : LeanSlang.emit shader = expected := by native_decide
example : shader.entryPointName = "main" := by native_decide
-- END PIN

end Drape.SlangCodegen.LbSubspace

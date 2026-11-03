import Drape.SlangCodegen.Common

/-!
# `Drape.SlangCodegen.LbCauchy` — the generalized Cauchy point

One thread. LBFGSpp `Cauchy::get_cauchy_point` (Cauchy.h), step for
step: breakpoints and `d = −g` (0 on coordinates at a bound), an
ascending sort of the finite nonzero breakpoints, `p = Wᵀd`,
`fp = −dᵀd`, `fpp = −θ·fp − pᵀMp`, then the sweep over intervals that
crosses a breakpoint group while `Δt_min ≥ Δt`, updating `c`, `p`, `fp`,
`fpp` with `Wb(act)` and `M·Wb(act)` per newly active coordinate, and
the final step on the free set. LBFGSpp's `crossed_all` branch and its
`fpp < ε` guard are kept.

Differences, all forced by the target:

- `std::sort` becomes an in-kernel heapsort of `ord` by `brk`. Neither is
  stable, so the order inside a tie may differ; sets are compared, not
  sequences.
- `+inf` is FLT_MAX: a zero gradient, or a gradient pointing at an absent
  bound (magnitude ≥ 1e30; the host maps ±inf to ±FLT_MAX), gives
  breakpoint FLT_MAX and puts the coordinate in the free set first.
- `break` is a loop flag; `search_greater` is a flagged scan.
- Long sums (`Wᵀd`, `dᵀd`) accumulate in df32; M·w is `lb_mv`.
- When nothing is left moving after the sweep (every breakpoint crossed,
  every free `d` zero), the last step is skipped: in exact arithmetic
  `p = 0` there, and in float `−fp/fpp · p` is residue over residue.

Bindings (set 0):

  0   ConstantBuffer<LbCauchyParams> { uint n; uint mcap; }
  1-4 StructuredBuffer<float>   x, g, lb, ub
  5-6 StructuredBuffer<float>   S, Y          (n × mcap, column-major)
  7   StructuredBuffer<uint>    st
  8   StructuredBuffer<float>   bf
  9   RWStructuredBuffer<float> xcp           (n)
  10  RWStructuredBuffer<float> vecc          (2·ncorr: c = Wᵀ(xcp − x))
  11  RWStructuredBuffer<float> wk            (≥ 2n: brk, then d)
  12  RWStructuredBuffer<uint>  iset          (8 + 3n)

`iset`: [0] |newact|, [1] |fv|, [2] 0 / 1 crossed_all / 2 every
coordinate at a bound, [3] |ord|; newact from 8, fv from 8 + n, the
sorted `ord` from 8 + 2n (the free set is `brk = inf` first, then the
uncrossed breakpoints in sort order, as in LBFGSpp).
-/

namespace Drape.SlangCodegen.LbCauchy

open LeanSlang
open Drape.SlangCodegen.Dsl
open Drape.SlangCodegen.Common

private def n : E := v "n"
private def nc : E := v "nc"
private def ordAt (k : E) : E := at_ "iset" (v "ordb" + k)
private def brk (c : E) : E := at_ "wk" c
private def dd (c : E) : E := at_ "wk" (n + c)
private def yAt (j i : E) : E := at_ "Y" (j * n + i)
private def sAt (j i : E) : E := at_ "S" (j * n + i)

private def breakpoints : List St :=
  [ let_ uT "nfree" (u 0)
  , let_ uT "nord" (u 0)
  , for_ "i" (u 0) n
      [ let_ fT "li" (at_ "lb" (v "i"))
      , let_ fT "ui" (at_ "ub" (v "i"))
      , let_ fT "gi" (at_ "g" (v "i"))
      , let_ fT "xi" (at_ "x" (v "i"))
      , let_ fT "b" fltMax
      , if_ (eq (v "li") (v "ui"))
          [ setv "b" (fl 0.0) ]
          [ if_ (lt (v "gi") (fl 0.0))
              [ if_ (lt (v "ui") unbounded) [ setv "b" ((v "xi" - v "ui") / v "gi") ] ]
              [ if_ (gt (v "gi") (fl 0.0))
                  [ if_ (gt (v "li") (-unbounded)) [ setv "b" ((v "xi" - v "li") / v "gi") ] ] ] ]
      , setAt "wk" (v "i") (v "b")
      , setAt "wk" (n + v "i") (sel (eq (v "b") (fl 0.0)) (fl 0.0) (-(v "gi")))
      , if_ (ge (v "b") fltMax)
          [ setAt "iset" (v "fvb" + v "nfree") (v "i"), setv "nfree" (v "nfree" + u 1) ]
          [ if_ (ne (v "b") (fl 0.0))
              [ setAt "iset" (v "ordb" + v "nord") (v "i"), setv "nord" (v "nord" + u 1) ] ] ] ]

private def heapsort : List St :=
  [ for_ "q" (u 0) (v "nord" / u 2)
      (siftStmts "h" (v "nord" / u 2 - u 1 - v "q") (v "nord") ordAt brk)
  , if_ (gt (v "nord") (u 1))
      [ for_ "q" (u 0) (v "nord" - u 1)
          ([ let_ uT "e" (v "nord" - u 1 - v "q")
           , let_ uT "sw" (ordAt (u 0))
           , set (ordAt (u 0)) (ordAt (v "e"))
           , set (ordAt (v "e")) (v "sw") ]
           ++ siftStmts "s" (u 0) (v "e") ordAt brk) ]
  , setAt "iset" (u 3) (v "nord") ]

/-- df32 accumulation of `Σ_i a(i)·b(i)` over n into `dst`. -/
private def dfSum (dst : String) (a b : E → E) : List St :=
  [ setv "a_hi" (fl 0.0)
  , setv "a_lo" (fl 0.0)
  , for_ "i" (u 0) n [ do_ (call "df_acc" [v "a_hi", v "a_lo", a (v "i"), b (v "i")]) ]
  , setv dst (v "a_hi" + v "a_lo") ]

private def firstInterval : List St :=
  [ decl fT "a_hi"
  , decl fT "a_lo"
  , decl fT "sum"
  , for_ "j" (u 0) nc
      (dfSum "sum" (yAt (v "j")) dd ++ [ setAt "vp" (v "j") (v "sum") ]
       ++ dfSum "sum" (sAt (v "j")) dd ++ [ setAt "vp" (nc + v "j") (v "theta" * v "sum") ])
  ]
  ++ dfSum "sum" dd dd ++
  [ let_ fT "fp" (-(v "sum"))
  , for_ "j" (u 0) (u 2 * nc) [ setAt "ch" (v "j") (at_ "vp" (v "j")) ]
  , do_ (call "lb_mv" [nc, v "mc", v "ch"])
  , let_ fT "pm" (fl 0.0)
  , for_ "j" (u 0) (u 2 * nc) [ setv "pm" (v "pm" + at_ "vp" (v "j") * at_ "ch" (v "j")) ]
  , let_ fT "fpp" (-(v "theta") * v "fp" - v "pm")
  , let_ fT "dtm" (-(v "fp") / v "fpp")
  , let_ fT "il" (fl 0.0)
  , let_ uT "bb" (u 0)
  , let_ fT "iu" (sel (lt (v "nord") (u 1)) fltMax (brk (ordAt (u 0))))
  , let_ fT "dt" (v "iu" - v "il")
  , let_ uT "crossed" (u 0)
  , let_ uT "go" (u 1)
  , let_ uT "nact" (u 0) ]

/-- Bound a coordinate reaches when it becomes active. -/
private def boundOf (a : E) : E := sel (gt (dd a) (fl 0.0)) (at_ "ub" a) (at_ "lb" a)

private def crossActive : List St :=
  [ let_ uT "a" (ordAt (v "k"))
  , let_ fT "xa" (boundOf (v "a"))
  , setAt "xcp" (v "a") (v "xa")
  , let_ fT "z" (v "xa" - at_ "x" (v "a"))
  , let_ fT "ga" (at_ "g" (v "a"))
  , let_ fT "gg" (v "ga" * v "ga")
  , for_ "j" (u 0) nc
      [ setAt "wa" (v "j") (yAt (v "j") (v "a"))
      , setAt "wa" (nc + v "j") (v "theta" * sAt (v "j") (v "a")) ]
  , for_ "j" (u 0) (u 2 * nc) [ setAt "ch" (v "j") (at_ "wa" (v "j")) ]
  , do_ (call "lb_mv" [nc, v "mc", v "ch"])
  , let_ fT "cv" (fl 0.0)
  , let_ fT "cp" (fl 0.0)
  , let_ fT "cw" (fl 0.0)
  , for_ "j" (u 0) (u 2 * nc)
      [ setv "cv" (v "cv" + at_ "ch" (v "j") * at_ "vc" (v "j"))
      , setv "cp" (v "cp" + at_ "ch" (v "j") * at_ "vp" (v "j"))
      , setv "cw" (v "cw" + at_ "ch" (v "j") * at_ "wa" (v "j")) ]
  , setv "fp" (v "fp" + (v "gg" + v "theta" * v "ga" * v "z" - v "ga" * v "cv"))
  , setv "fpp" (v "fpp" - (v "theta" * v "gg" + fl 2.0 * v "ga" * v "cp" + v "gg" * v "cw"))
  , for_ "j" (u 0) (u 2 * nc) [ setAt "vp" (v "j") (at_ "vp" (v "j") + v "ga" * at_ "wa" (v "j")) ]
  , setAt "wk" (n + v "a") (fl 0.0)
  , setAt "iset" (u 8 + v "nact") (v "a")
  , setv "nact" (v "nact" + u 1) ]

private def sweep : List St :=
  [ while_ (and_ (eq (v "go") (u 1)) (ge (v "dtm") (v "dt")))
      [ for_ "j" (u 0) (u 2 * nc) [ setAt "vc" (v "j") (at_ "vc" (v "j") + v "dt" * at_ "vp" (v "j")) ]
      , let_ uT "ae" (v "bb")
      , let_ uT "sg" (u 1)
      , while_ (and_ (eq (v "sg") (u 1)) (lt (v "ae") (v "nord")))
          [ if_ (gt (brk (ordAt (v "ae"))) (v "iu")) [ setv "sg" (u 0) ] [ setv "ae" (v "ae" + u 1) ] ]
      , if_ (and_ (eq (v "nfree") (u 0)) (eq (v "ae") (v "nord")))
          [ for_ "k" (v "bb") (v "ae")
              [ let_ uT "a" (ordAt (v "k"))
              , setAt "xcp" (v "a") (boundOf (v "a"))
              , setAt "iset" (u 8 + v "nact") (v "a")
              , setv "nact" (v "nact" + u 1) ]
          , setv "crossed" (u 1)
          , setv "go" (u 0) ]
          [ setv "fp" (v "fp" + v "dt" * v "fpp")
          , for_ "k" (v "bb") (v "ae") crossActive
          , setv "dtm" (-(v "fp") / v "fpp")
          , setv "il" (v "iu")
          , setv "bb" (v "ae")
          , if_ (ge (v "bb") (v "nord"))
              [ setv "go" (u 0) ]
              [ setv "iu" (brk (ordAt (v "bb"))), setv "dt" (v "iu" - v "il") ] ] ] ]

private def lastStep : List St :=
  [ if_ (lt (v "fpp") dblEps) [ setv "dtm" (-(v "fp") / dblEps) ]
  , if_ (eq (v "crossed") (u 0))
      [ setv "dtm" (fmax (v "dtm") (fl 0.0))
      -- Float guard: with no coordinate left moving (every breakpoint
      -- crossed, every free d zero) p is exactly 0 and the last step is
      -- void; in float p, fp and fpp are rounding residue, and
      -- dtm = -fp/fpp times that residue would perturb c by ~1e-4.
      , let_ uT "live" (sel (lt (v "bb") (v "nord")) (u 1) (u 0))
      , for_ "k" (u 0) (v "nfree")
          [ if_ (ne (dd (at_ "iset" (v "fvb" + v "k"))) (fl 0.0)) [ setv "live" (u 1) ] ]
      , if_ (eq (v "live") (u 0)) [ setv "dtm" (fl 0.0) ]
      , for_ "j" (u 0) (u 2 * nc) [ setAt "vc" (v "j") (at_ "vc" (v "j") + v "dtm" * at_ "vp" (v "j")) ]
      , let_ fT "tf" (v "il" + v "dtm")
      , for_ "k" (u 0) (v "nfree")
          [ let_ uT "c" (at_ "iset" (v "fvb" + v "k"))
          , setAt "xcp" (v "c") (at_ "x" (v "c") + v "tf" * dd (v "c")) ]
      , for_ "k" (v "bb") (v "nord")
          [ let_ uT "c" (ordAt (v "k"))
          , setAt "xcp" (v "c") (at_ "x" (v "c") + v "tf" * dd (v "c"))
          , setAt "iset" (v "fvb" + v "nfree") (v "c")
          , setv "nfree" (v "nfree" + u 1) ] ]
  , for_ "j" (u 0) (u 2 * nc) [ setAt "vecc" (v "j") (at_ "vc" (v "j")) ]
  , setAt "iset" (u 0) (v "nact")
  , setAt "iset" (u 1) (v "nfree")
  , setAt "iset" (u 2) (v "crossed") ]

def shader : SlangShaderModule :=
  { structs := [ { name := "LbCauchyParams", fields := [fld "n" uT, fld "mcap" uT] } ]
  , globals :=
      [ paramsCB "LbCauchyParams", roF "x" 1, roF "g" 2, roF "lb" 3, roF "ub" 4
      , roF "S" 5, roF "Y" 6, roU "st" 7, roF "bf" 8
      , rwF "xcp" 9, rwF "vecc" 10, rwF "wk" 11, rwU "iset" 12 ]
  , functions := dfHelpers ++
      [ lbMv
      , entry 1 [dtid] (
          [ let_ uT "n" (p "n")
          , let_ uT "mc" (p "mcap")
          , let_ uT "nc" (at_ "st" (u 0))
          , let_ fT "theta" (at_ "bf" (u 0))
          , let_ uT "fvb" (u 8 + n)
          , let_ uT "ordb" (u 8 + u 2 * n)
          , for_ "i" (u 0) n [ setAt "xcp" (v "i") (at_ "x" (v "i")) ]
          , arr fT "vc" (2 * maxM)
          , arr fT "vp" (2 * maxM)
          , arr fT "ch" (2 * maxM)
          , arr fT "wa" (2 * maxM)
          , for_ "j" (u 0) (u (2 * maxM))
              [ setAt "vc" (v "j") (fl 0.0), setAt "vp" (v "j") (fl 0.0)
              , setAt "ch" (v "j") (fl 0.0), setAt "wa" (v "j") (fl 0.0) ] ]
          ++ breakpoints ++ heapsort ++
          [ if_ (and_ (eq (v "nfree") (u 0)) (eq (v "nord") (u 0)))
              [ for_ "j" (u 0) (u 2 * nc) [ setAt "vecc" (v "j") (fl 0.0) ]
              , setAt "iset" (u 0) (u 0)
              , setAt "iset" (u 1) (u 0)
              , setAt "iset" (u 2) (u 2)
              , ret ] ]
          ++ firstInterval ++ sweep ++ lastStep) ] }

-- BEGIN PIN
def expected : String :=
"struct LbCauchyParams {
  uint n;
  uint mcap;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LbCauchyParams> params;
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
RWStructuredBuffer<float> xcp;
[[vk::binding(10, 0)]]
RWStructuredBuffer<float> vecc;
[[vk::binding(11, 0)]]
RWStructuredBuffer<float> wk;
[[vk::binding(12, 0)]]
RWStructuredBuffer<uint> iset;

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

[shader(\"compute\")] [numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint n = params.n;
  uint mc = params.mcap;
  uint nc = st[0u];
  float theta = bf[0u];
  uint fvb = (8u + n);
  uint ordb = (8u + (2u * n));
  for (uint i = 0u; i < n; ++i) {
    xcp[i] = x[i];
  }
  float vc[32];
  float vp[32];
  float ch[32];
  float wa[32];
  for (uint j = 0u; j < 32u; ++j) {
    vc[j] = 0.000000;
    vp[j] = 0.000000;
    ch[j] = 0.000000;
    wa[j] = 0.000000;
  }
  uint nfree = 0u;
  uint nord = 0u;
  for (uint i = 0u; i < n; ++i) {
    float li = lb[i];
    float ui = ub[i];
    float gi = g[i];
    float xi = x[i];
    float b = asfloat(2139095039u);
    if ((li == ui)) {
      b = 0.000000;
    } else {
      if ((gi < 0.000000)) {
        if ((ui < asfloat(1900671690u))) {
          b = ((xi - ui) / gi);
        }
      } else {
        if ((gi > 0.000000)) {
          if ((li > (-asfloat(1900671690u)))) {
            b = ((xi - li) / gi);
          }
        }
      }
    }
    wk[i] = b;
    wk[(n + i)] = ((b == 0.000000) ? 0.000000 : (-gi));
    if ((b >= asfloat(2139095039u))) {
      iset[(fvb + nfree)] = i;
      nfree = (nfree + 1u);
    } else {
      if ((b != 0.000000)) {
        iset[(ordb + nord)] = i;
        nord = (nord + 1u);
      }
    }
  }
  for (uint q = 0u; q < (nord / 2u); ++q) {
    uint hr = (((nord / 2u) - 1u) - q);
    uint hgo = 1u;
    while ((hgo == 1u)) {
      uint hc = ((2u * hr) + 1u);
      if ((hc >= nord)) {
        hgo = 0u;
      } else {
        if ((((hc + 1u) < nord) && (wk[iset[(ordb + hc)]] < wk[iset[(ordb + (hc + 1u))]]))) {
          hc = (hc + 1u);
        }
        if ((wk[iset[(ordb + hr)]] < wk[iset[(ordb + hc)]])) {
          uint htmp = iset[(ordb + hr)];
          iset[(ordb + hr)] = iset[(ordb + hc)];
          iset[(ordb + hc)] = htmp;
          hr = hc;
        } else {
          hgo = 0u;
        }
      }
    }
  }
  if ((nord > 1u)) {
    for (uint q = 0u; q < (nord - 1u); ++q) {
      uint e = ((nord - 1u) - q);
      uint sw = iset[(ordb + 0u)];
      iset[(ordb + 0u)] = iset[(ordb + e)];
      iset[(ordb + e)] = sw;
      uint sr = 0u;
      uint sgo = 1u;
      while ((sgo == 1u)) {
        uint sc = ((2u * sr) + 1u);
        if ((sc >= e)) {
          sgo = 0u;
        } else {
          if ((((sc + 1u) < e) && (wk[iset[(ordb + sc)]] < wk[iset[(ordb + (sc + 1u))]]))) {
            sc = (sc + 1u);
          }
          if ((wk[iset[(ordb + sr)]] < wk[iset[(ordb + sc)]])) {
            uint stmp = iset[(ordb + sr)];
            iset[(ordb + sr)] = iset[(ordb + sc)];
            iset[(ordb + sc)] = stmp;
            sr = sc;
          } else {
            sgo = 0u;
          }
        }
      }
    }
  }
  iset[3u] = nord;
  if (((nfree == 0u) && (nord == 0u))) {
    for (uint j = 0u; j < (2u * nc); ++j) {
      vecc[j] = 0.000000;
    }
    iset[0u] = 0u;
    iset[1u] = 0u;
    iset[2u] = 2u;
    return;
  }
  float a_hi;
  float a_lo;
  float sum;
  for (uint j = 0u; j < nc; ++j) {
    a_hi = 0.000000;
    a_lo = 0.000000;
    for (uint i = 0u; i < n; ++i) {
      df_acc(a_hi, a_lo, Y[((j * n) + i)], wk[(n + i)]);
    }
    sum = (a_hi + a_lo);
    vp[j] = sum;
    a_hi = 0.000000;
    a_lo = 0.000000;
    for (uint i = 0u; i < n; ++i) {
      df_acc(a_hi, a_lo, S[((j * n) + i)], wk[(n + i)]);
    }
    sum = (a_hi + a_lo);
    vp[(nc + j)] = (theta * sum);
  }
  a_hi = 0.000000;
  a_lo = 0.000000;
  for (uint i = 0u; i < n; ++i) {
    df_acc(a_hi, a_lo, wk[(n + i)], wk[(n + i)]);
  }
  sum = (a_hi + a_lo);
  float fp = (-sum);
  for (uint j = 0u; j < (2u * nc); ++j) {
    ch[j] = vp[j];
  }
  lb_mv(nc, mc, ch);
  float pm = 0.000000;
  for (uint j = 0u; j < (2u * nc); ++j) {
    pm = (pm + (vp[j] * ch[j]));
  }
  float fpp = (((-theta) * fp) - pm);
  float dtm = ((-fp) / fpp);
  float il = 0.000000;
  uint bb = 0u;
  float iu = ((nord < 1u) ? asfloat(2139095039u) : wk[iset[(ordb + 0u)]]);
  float dt = (iu - il);
  uint crossed = 0u;
  uint go = 1u;
  uint nact = 0u;
  while (((go == 1u) && (dtm >= dt))) {
    for (uint j = 0u; j < (2u * nc); ++j) {
      vc[j] = (vc[j] + (dt * vp[j]));
    }
    uint ae = bb;
    uint sg = 1u;
    while (((sg == 1u) && (ae < nord))) {
      if ((wk[iset[(ordb + ae)]] > iu)) {
        sg = 0u;
      } else {
        ae = (ae + 1u);
      }
    }
    if (((nfree == 0u) && (ae == nord))) {
      for (uint k = bb; k < ae; ++k) {
        uint a = iset[(ordb + k)];
        xcp[a] = ((wk[(n + a)] > 0.000000) ? ub[a] : lb[a]);
        iset[(8u + nact)] = a;
        nact = (nact + 1u);
      }
      crossed = 1u;
      go = 0u;
    } else {
      fp = (fp + (dt * fpp));
      for (uint k = bb; k < ae; ++k) {
        uint a = iset[(ordb + k)];
        float xa = ((wk[(n + a)] > 0.000000) ? ub[a] : lb[a]);
        xcp[a] = xa;
        float z = (xa - x[a]);
        float ga = g[a];
        float gg = (ga * ga);
        for (uint j = 0u; j < nc; ++j) {
          wa[j] = Y[((j * n) + a)];
          wa[(nc + j)] = (theta * S[((j * n) + a)]);
        }
        for (uint j = 0u; j < (2u * nc); ++j) {
          ch[j] = wa[j];
        }
        lb_mv(nc, mc, ch);
        float cv = 0.000000;
        float cp = 0.000000;
        float cw = 0.000000;
        for (uint j = 0u; j < (2u * nc); ++j) {
          cv = (cv + (ch[j] * vc[j]));
          cp = (cp + (ch[j] * vp[j]));
          cw = (cw + (ch[j] * wa[j]));
        }
        fp = (fp + ((gg + ((theta * ga) * z)) - (ga * cv)));
        fpp = (fpp - (((theta * gg) + ((2.000000 * ga) * cp)) + (gg * cw)));
        for (uint j = 0u; j < (2u * nc); ++j) {
          vp[j] = (vp[j] + (ga * wa[j]));
        }
        wk[(n + a)] = 0.000000;
        iset[(8u + nact)] = a;
        nact = (nact + 1u);
      }
      dtm = ((-fp) / fpp);
      il = iu;
      bb = ae;
      if ((bb >= nord)) {
        go = 0u;
      } else {
        iu = wk[iset[(ordb + bb)]];
        dt = (iu - il);
      }
    }
  }
  if ((fpp < asfloat(629145600u))) {
    dtm = ((-fp) / asfloat(629145600u));
  }
  if ((crossed == 0u)) {
    dtm = max(dtm, 0.000000);
    uint live = ((bb < nord) ? 1u : 0u);
    for (uint k = 0u; k < nfree; ++k) {
      if ((wk[(n + iset[(fvb + k)])] != 0.000000)) {
        live = 1u;
      }
    }
    if ((live == 0u)) {
      dtm = 0.000000;
    }
    for (uint j = 0u; j < (2u * nc); ++j) {
      vc[j] = (vc[j] + (dtm * vp[j]));
    }
    float tf = (il + dtm);
    for (uint k = 0u; k < nfree; ++k) {
      uint c = iset[(fvb + k)];
      xcp[c] = (x[c] + (tf * wk[(n + c)]));
    }
    for (uint k = bb; k < nord; ++k) {
      uint c = iset[(ordb + k)];
      xcp[c] = (x[c] + (tf * wk[(n + c)]));
      iset[(fvb + nfree)] = c;
      nfree = (nfree + 1u);
    }
  }
  for (uint j = 0u; j < (2u * nc); ++j) {
    vecc[j] = vc[j];
  }
  iset[0u] = nact;
  iset[1u] = nfree;
  iset[2u] = crossed;
}"

example : LeanSlang.emit shader = expected := by native_decide
example : shader.entryPointName = "main" := by native_decide
-- END PIN

end Drape.SlangCodegen.LbCauchy

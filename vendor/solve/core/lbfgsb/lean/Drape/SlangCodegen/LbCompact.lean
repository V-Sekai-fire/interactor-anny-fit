import Drape.SlangCodegen.Common

/-!
# `Drape.SlangCodegen.LbCompact` — BFGSMat update and T's Cholesky

One thread. LBFGSpp `BFGSMat::add_correction` (BFGSMat.h) and
`BFGSMat::reset`, on the compact state in `bf`/`st` (layout in
`Drape.SlangCodegen.Common`).

`mode = 1` is `reset`: ncorr 0, ptr mcap, θ 1.

`mode = 0` takes the products `lb_multi_dot` left in `dots`
(df32 pairs: `Sᵀs` at 0, `Yᵀs` at 2·mcap, then `sᵀy`, `yᵀy`, `sᵀs` at
4·mcap, +2, +4) and

1. applies LBFGSpp's curvature filter `sᵀy > ε·yᵀy` (ε = FLT_EPSILON;
   the solver's own test, LBFGSB.h, moved to the device so the store in
   `lb_ring_store` needs no readback); `st[3]` records the verdict;
2. if accepted: `loc = ptr % mcap`, row and column `loc` of SS, row `loc`
   of SY (column `loc` of SY is cleared, as LBFGSpp zeroes the stale
   `L` column), `D[loc] = sᵀy`, `θ = yᵀy / sᵀy`, `ptr = loc + 1`;
3. rebuilds `L` in slot order (pair i newer than pair j, by chronological
   rank from `ptr` and `ncorr`), `T = θ·SS + L·D⁻¹·Lᵀ` and its lower
   Cholesky factor; `st[4] = 0` flags a non-positive pivot.

LBFGSpp instead keeps the permuted `M⁻¹ = [[−D, Lᵀ], [L, θ·SS]]` and
factors it with Bunch–Kaufman; `T` is that matrix's Schur complement of
the `−D` block, SPD whenever every `D > 0`, which the filter ensures.

Bindings (set 0):

  0  ConstantBuffer<LbCompactParams> { uint mcap; uint mode; }
  1  StructuredBuffer<float>   dots
  2  RWStructuredBuffer<uint>  st
  3  RWStructuredBuffer<float> bf
-/

namespace Drape.SlangCodegen.LbCompact

open LeanSlang
open Drape.SlangCodegen.Dsl
open Drape.SlangCodegen.Common

private def mc : E := v "mc"
private def dotPair (k : E) : E := at_ "dots" k + at_ "dots" (k + u 1)
private def bfAt (k : E) : E := at_ "bf" k

def shader : SlangShaderModule :=
  { structs := [ { name := "LbCompactParams", fields := [fld "mcap" uT, fld "mode" uT] } ]
  , globals := [ paramsCB "LbCompactParams", roF "dots" 1, rwU "st" 2, rwF "bf" 3 ]
  , functions :=
      [ entry 1 [dtid] (
          [ let_ uT "mc" (p "mcap")
          , if_ (eq (p "mode") (u 1))
              [ setAt "st" (u 0) (u 0), setAt "st" (u 1) mc, setAt "st" (u 2) (u 0)
              , setAt "st" (u 3) (u 0), setAt "st" (u 4) (u 1)
              , setAt "bf" (u 0) (fl 1.0)
              , ret ]
          , let_ fT "sy" (dotPair (u 4 * mc))
          , let_ fT "yy" (dotPair (u 4 * mc + u 2))
          , let_ fT "ss" (dotPair (u 4 * mc + u 4))
          , let_ uT "ncorr" (at_ "st" (u 0))
          , let_ uT "ptr" (at_ "st" (u 1))
          , let_ uT "ok" (sel (gt (v "sy") (fltEps * v "yy")) (u 1) (u 0))
          , setAt "st" (u 3) (v "ok")
          , if_ (eq (v "ok") (u 1))
              [ let_ uT "loc" (v "ptr" % mc)
              , setv "ncorr" (fmin (v "ncorr" + u 1) mc)
              , for_ "j" (u 0) (v "ncorr")
                  [ let_ fT "sj" (sel (eq (v "j") (v "loc")) (v "ss") (dotPair (u 2 * v "j")))
                  , setAt "bf" (ssIx mc (v "loc") (v "j")) (v "sj")
                  , setAt "bf" (ssIx mc (v "j") (v "loc")) (v "sj") ]
              , for_ "i" (u 0) mc [ setAt "bf" (syIx mc (v "i") (v "loc")) (fl 0.0) ]
              , for_ "j" (u 0) (v "ncorr")
                  [ setAt "bf" (syIx mc (v "loc") (v "j"))
                      (sel (eq (v "j") (v "loc")) (v "sy") (dotPair (u 2 * mc + u 2 * v "j"))) ]
              , setAt "bf" (dIx mc (v "loc")) (v "sy")
              , setAt "bf" (u 0) (v "yy" / v "sy")
              , setv "ptr" (v "loc" + u 1)
              , setAt "st" (u 0) (v "ncorr")
              , setAt "st" (u 1) (v "ptr")
              , setAt "st" (u 2) (v "loc") ]
          , let_ fT "theta" (bfAt (u 0))
          , let_ uT "oldest" ((v "ptr" + mc - v "ncorr") % mc)
          , for_ "i" (u 0) (v "ncorr")
              [ let_ uT "ri" ((v "i" + mc - v "oldest") % mc)
              , for_ "j" (u 0) (v "ncorr")
                  [ let_ uT "rj" ((v "j" + mc - v "oldest") % mc)
                  , setAt "bf" (lmIx mc (v "i") (v "j"))
                      (sel (gt (v "ri") (v "rj")) (bfAt (syIx mc (v "i") (v "j"))) (fl 0.0)) ] ]
          , for_ "i" (u 0) (v "ncorr")
              [ for_ "j" (u 0) (v "i" + u 1)
                  [ let_ fT "acc" (v "theta" * bfAt (ssIx mc (v "i") (v "j")))
                  , for_ "k" (u 0) (v "ncorr")
                      [ setv "acc" (v "acc" + bfAt (lmIx mc (v "i") (v "k")) * bfAt (lmIx mc (v "j") (v "k"))
                          / bfAt (dIx mc (v "k"))) ]
                  , setAt "bf" (rcIx mc (v "i") (v "j")) (v "acc") ] ]
          , let_ uT "cok" (u 1) ]
          ++ cholStmts "c" (v "ncorr") (fun i j => at_ "bf" (rcIx mc i j)) "cok"
          ++ [ setAt "st" (u 4) (v "cok") ]) ] }

-- BEGIN PIN
def expected : String :=
"struct LbCompactParams {
  uint mcap;
  uint mode;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LbCompactParams> params;
[[vk::binding(1, 0)]]
StructuredBuffer<float> dots;
[[vk::binding(2, 0)]]
RWStructuredBuffer<uint> st;
[[vk::binding(3, 0)]]
RWStructuredBuffer<float> bf;

[shader(\"compute\")] [numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint mc = params.mcap;
  if ((params.mode == 1u)) {
    st[0u] = 0u;
    st[1u] = mc;
    st[2u] = 0u;
    st[3u] = 0u;
    st[4u] = 1u;
    bf[0u] = 1.000000;
    return;
  }
  float sy = (dots[(4u * mc)] + dots[((4u * mc) + 1u)]);
  float yy = (dots[((4u * mc) + 2u)] + dots[(((4u * mc) + 2u) + 1u)]);
  float ss = (dots[((4u * mc) + 4u)] + dots[(((4u * mc) + 4u) + 1u)]);
  uint ncorr = st[0u];
  uint ptr = st[1u];
  uint ok = ((sy > (asfloat(872415232u) * yy)) ? 1u : 0u);
  st[3u] = ok;
  if ((ok == 1u)) {
    uint loc = (ptr % mc);
    ncorr = min((ncorr + 1u), mc);
    for (uint j = 0u; j < ncorr; ++j) {
      float sj = ((j == loc) ? ss : (dots[(2u * j)] + dots[((2u * j) + 1u)]));
      bf[((4u + (loc * mc)) + j)] = sj;
      bf[((4u + (j * mc)) + loc)] = sj;
    }
    for (uint i = 0u; i < mc; ++i) {
      bf[(((4u + (mc * mc)) + (i * mc)) + loc)] = 0.000000;
    }
    for (uint j = 0u; j < ncorr; ++j) {
      bf[(((4u + (mc * mc)) + (loc * mc)) + j)] = ((j == loc) ? sy : (dots[((2u * mc) + (2u * j))] + dots[(((2u * mc) + (2u * j)) + 1u)]));
    }
    bf[((4u + ((4u * mc) * mc)) + loc)] = sy;
    bf[0u] = (yy / sy);
    ptr = (loc + 1u);
    st[0u] = ncorr;
    st[1u] = ptr;
    st[2u] = loc;
  }
  float theta = bf[0u];
  uint oldest = (((ptr + mc) - ncorr) % mc);
  for (uint i = 0u; i < ncorr; ++i) {
    uint ri = (((i + mc) - oldest) % mc);
    for (uint j = 0u; j < ncorr; ++j) {
      uint rj = (((j + mc) - oldest) % mc);
      bf[(((4u + ((2u * mc) * mc)) + (i * mc)) + j)] = ((ri > rj) ? bf[(((4u + (mc * mc)) + (i * mc)) + j)] : 0.000000);
    }
  }
  for (uint i = 0u; i < ncorr; ++i) {
    for (uint j = 0u; j < (i + 1u); ++j) {
      float acc = (theta * bf[((4u + (i * mc)) + j)]);
      for (uint k = 0u; k < ncorr; ++k) {
        acc = (acc + ((bf[(((4u + ((2u * mc) * mc)) + (i * mc)) + k)] * bf[(((4u + ((2u * mc) * mc)) + (j * mc)) + k)]) / bf[((4u + ((4u * mc) * mc)) + k)]));
      }
      bf[(((4u + ((3u * mc) * mc)) + (i * mc)) + j)] = acc;
    }
  }
  uint cok = 1u;
  for (uint cj = 0u; cj < ncorr; ++cj) {
    float cs = bf[(((4u + ((3u * mc) * mc)) + (cj * mc)) + cj)];
    for (uint ct = 0u; ct < cj; ++ct) {
      cs = (cs - (bf[(((4u + ((3u * mc) * mc)) + (cj * mc)) + ct)] * bf[(((4u + ((3u * mc) * mc)) + (cj * mc)) + ct)]));
    }
    if ((cs <= 0.000000)) {
      cok = 0u;
      cs = (asfloat(872415232u) * asfloat(872415232u));
    }
    float cd = sqrt(cs);
    bf[(((4u + ((3u * mc) * mc)) + (cj * mc)) + cj)] = cd;
    for (uint ci = (cj + 1u); ci < ncorr; ++ci) {
      float cr = bf[(((4u + ((3u * mc) * mc)) + (ci * mc)) + cj)];
      for (uint ct = 0u; ct < cj; ++ct) {
        cr = (cr - (bf[(((4u + ((3u * mc) * mc)) + (ci * mc)) + ct)] * bf[(((4u + ((3u * mc) * mc)) + (cj * mc)) + ct)]));
      }
      bf[(((4u + ((3u * mc) * mc)) + (ci * mc)) + cj)] = (cr / cd);
    }
  }
  st[4u] = cok;
}"

example : LeanSlang.emit shader = expected := by native_decide
example : shader.entryPointName = "main" := by native_decide
-- END PIN

end Drape.SlangCodegen.LbCompact

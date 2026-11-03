import Drape.SlangCodegen.Common

/-!
# `Drape.SlangCodegen.LbApplyM` — `M·v`

One thread. LBFGSpp `BFGSMat::apply_Mv` (BFGSMat.h) for a `2·ncorr`
vector `[Y part ; S part]` in ring-slot order, through the Schur/Cholesky
helper `lb_mv` (see `Drape.SlangCodegen.Common`), which `lb_cauchy` and
`lb_subspace` inline as well. `ncorr = 0` leaves `vout` untouched
(LBFGSpp returns an empty vector).

Bindings (set 0):

  0  ConstantBuffer<LbApplyMParams> { uint mcap; uint inOff; uint outOff; }
  1  StructuredBuffer<uint>    st
  2  StructuredBuffer<float>   bf
  3  StructuredBuffer<float>   vin
  4  RWStructuredBuffer<float> vout
-/

namespace Drape.SlangCodegen.LbApplyM

open LeanSlang
open Drape.SlangCodegen.Dsl
open Drape.SlangCodegen.Common

def shader : SlangShaderModule :=
  { structs := [ { name := "LbApplyMParams", fields := [fld "mcap" uT, fld "inOff" uT, fld "outOff" uT] } ]
  , globals := [ paramsCB "LbApplyMParams", roU "st" 1, roF "bf" 2, roF "vin" 3, rwF "vout" 4 ]
  , functions :=
      [ lbMv
      , entry 1 [dtid]
          [ let_ uT "nc" (at_ "st" (u 0))
          , if_ (eq (v "nc") (u 0)) [ ret ]
          , arr fT "w" (2 * maxM)
          , for_ "j" (u 0) (u 2 * v "nc") [ setAt "w" (v "j") (at_ "vin" (p "inOff" + v "j")) ]
          , do_ (call "lb_mv" [v "nc", p "mcap", v "w"])
          , for_ "j" (u 0) (u 2 * v "nc") [ setAt "vout" (p "outOff" + v "j") (at_ "w" (v "j")) ] ] ] }

-- BEGIN PIN
def expected : String :=
"struct LbApplyMParams {
  uint mcap;
  uint inOff;
  uint outOff;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LbApplyMParams> params;
[[vk::binding(1, 0)]]
StructuredBuffer<uint> st;
[[vk::binding(2, 0)]]
StructuredBuffer<float> bf;
[[vk::binding(3, 0)]]
StructuredBuffer<float> vin;
[[vk::binding(4, 0)]]
RWStructuredBuffer<float> vout;

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
  uint nc = st[0u];
  if ((nc == 0u)) {
    return;
  }
  float w[32];
  for (uint j = 0u; j < (2u * nc); ++j) {
    w[j] = vin[(params.inOff + j)];
  }
  lb_mv(nc, params.mcap, w);
  for (uint j = 0u; j < (2u * nc); ++j) {
    vout[(params.outOff + j)] = w[j];
  }
}"

example : LeanSlang.emit shader = expected := by native_decide
example : shader.entryPointName = "main" := by native_decide
-- END PIN

end Drape.SlangCodegen.LbApplyM

import Drape.SlangCodegen.LbPgInf

/-!
# `Drape.SlangCodegen.LbPgInfSerial` — `LbPgInf` in one thread

The cpp-target sibling of `LbPgInf` (the `DotReduceSerial` precedent):
same bindings, same per-coordinate term, a plain loop instead of the
`groupshared` tree. Max is exact, so both give the same bits.
-/

namespace Drape.SlangCodegen.LbPgInfSerial

open LeanSlang
open Drape.SlangCodegen.Dsl

def shader : SlangShaderModule :=
  { structs := [LbPgInf.params]
  , globals := LbPgInf.globals
  , functions :=
      [ entry 1 [dtid]
          [ let_ fT "acc" (fl 0.0)
          , for_ "i" (u 0) (p "n") [ setv "acc" (fmax (v "acc") (LbPgInf.term (v "i"))) ]
          , setAt "dst" (p "dstOff") (v "acc") ] ] }

-- BEGIN PIN
def expected : String :=
"struct LbPgInfParams {
  uint n;
  uint dstOff;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LbPgInfParams> params;
[[vk::binding(1, 0)]]
StructuredBuffer<float> x;
[[vk::binding(2, 0)]]
StructuredBuffer<float> g;
[[vk::binding(3, 0)]]
StructuredBuffer<float> lb;
[[vk::binding(4, 0)]]
StructuredBuffer<float> ub;
[[vk::binding(5, 0)]]
RWStructuredBuffer<float> dst;

[shader(\"compute\")] [numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  float acc = 0.000000;
  for (uint i = 0u; i < params.n; ++i) {
    acc = max(acc, abs((min(max((x[i] - g[i]), lb[i]), ub[i]) - x[i])));
  }
  dst[params.dstOff] = acc;
}"

example : LeanSlang.emit shader = expected := by native_decide
example : shader.entryPointName = "main" := by native_decide
-- END PIN

end Drape.SlangCodegen.LbPgInfSerial

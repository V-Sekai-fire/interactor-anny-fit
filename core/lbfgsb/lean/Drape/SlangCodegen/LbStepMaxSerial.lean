import Drape.SlangCodegen.LbStepMax

/-!
# `Drape.SlangCodegen.LbStepMaxSerial` — `LbStepMax` in one thread

The cpp-target sibling of `LbStepMax`: same bindings and per-coordinate
update, one loop. Min is exact, so both give the same bits.
-/

namespace Drape.SlangCodegen.LbStepMaxSerial

open LeanSlang
open Drape.SlangCodegen.Dsl

def shader : SlangShaderModule :=
  { structs := [LbStepMax.params]
  , globals := LbStepMax.globals
  , functions :=
      [ entry 1 [dtid]
          [ let_ fT "acc" fltMax
          , for_ "i" (u 0) (p "n") (LbStepMax.update (v "i"))
          , setAt "dst" (p "dstOff") (v "acc") ] ] }

-- BEGIN PIN
def expected : String :=
"struct LbStepMaxParams {
  uint n;
  uint dstOff;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LbStepMaxParams> params;
[[vk::binding(1, 0)]]
StructuredBuffer<float> x;
[[vk::binding(2, 0)]]
StructuredBuffer<float> d;
[[vk::binding(3, 0)]]
StructuredBuffer<float> lb;
[[vk::binding(4, 0)]]
StructuredBuffer<float> ub;
[[vk::binding(5, 0)]]
RWStructuredBuffer<float> dst;

[shader(\"compute\")] [numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  float acc = asfloat(2139095039u);
  for (uint i = 0u; i < params.n; ++i) {
    float di = d[i];
    if (((di > 0.000000) && (ub[i] < asfloat(1900671690u)))) {
      acc = min(acc, ((ub[i] - x[i]) / di));
    }
    if (((di < 0.000000) && (lb[i] > (-asfloat(1900671690u))))) {
      acc = min(acc, ((lb[i] - x[i]) / di));
    }
  }
  dst[params.dstOff] = acc;
}"

example : LeanSlang.emit shader = expected := by native_decide
example : shader.entryPointName = "main" := by native_decide
-- END PIN

end Drape.SlangCodegen.LbStepMaxSerial

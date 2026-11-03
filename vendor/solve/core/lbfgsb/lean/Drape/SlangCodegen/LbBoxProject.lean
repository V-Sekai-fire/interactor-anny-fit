import Drape.SlangCodegen.Dsl

/-!
# `Drape.SlangCodegen.LbBoxProject` — `x ← clamp(x, lb, ub)`

LBFGSpp `LBFGSBSolver::force_bounds` (LBFGSB.h): the projection onto the
box, applied to x0 before the first evaluation and after every line
search. One thread per coordinate, in place.

Bindings (set 0):

  0  ConstantBuffer<LbBoxProjectParams> { uint n; }
  1  RWStructuredBuffer<float> x
  2  StructuredBuffer<float>   lb
  3  StructuredBuffer<float>   ub

An absent bound is carried as ±FLT_MAX (the host maps ±inf), so the
clamp needs no special case.
-/

namespace Drape.SlangCodegen.LbBoxProject

open LeanSlang
open Drape.SlangCodegen.Dsl

def shader : SlangShaderModule :=
  { structs := [ { name := "LbBoxProjectParams", fields := [fld "n" uT] } ]
  , globals := [ paramsCB "LbBoxProjectParams", rwF "x" 1, roF "lb" 2, roF "ub" 3 ]
  , functions :=
      [ entry 256 [dtid]
          [ let_ uT "i" (.member (v "tid") "x")
          , if_ (ge (v "i") (p "n")) [ ret ]
          , setAt "x" (v "i") (fmin (fmax (at_ "x" (v "i")) (at_ "lb" (v "i"))) (at_ "ub" (v "i"))) ] ] }

-- BEGIN PIN
def expected : String :=
"struct LbBoxProjectParams {
  uint n;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LbBoxProjectParams> params;
[[vk::binding(1, 0)]]
RWStructuredBuffer<float> x;
[[vk::binding(2, 0)]]
StructuredBuffer<float> lb;
[[vk::binding(3, 0)]]
StructuredBuffer<float> ub;

[shader(\"compute\")] [numthreads(256, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint i = tid.x;
  if ((i >= params.n)) {
    return;
  }
  x[i] = min(max(x[i], lb[i]), ub[i]);
}"

example : LeanSlang.emit shader = expected := by native_decide
example : shader.entryPointName = "main" := by native_decide
-- END PIN

end Drape.SlangCodegen.LbBoxProject

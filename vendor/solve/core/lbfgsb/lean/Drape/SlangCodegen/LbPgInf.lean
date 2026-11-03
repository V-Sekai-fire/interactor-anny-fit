import Drape.SlangCodegen.Dsl

/-!
# `Drape.SlangCodegen.LbPgInf` — `‖P(x − g) − x‖∞`

LBFGSpp `LBFGSBSolver::proj_grad_norm` (LBFGSB.h): the infinity norm of
the projected gradient step, the solver's gradient convergence test.
One workgroup of 256 threads, grid-strided, then a `groupshared` max
tree. `slangc -target cpp` rejects the barrier, so the in-guest CPU path
runs `LbPgInfSerial`, the same sum in one thread.

Bindings (set 0):

  0  ConstantBuffer<LbPgInfParams> { uint n; uint dstOff; }
  1  StructuredBuffer<float>   x
  2  StructuredBuffer<float>   g
  3  StructuredBuffer<float>   lb
  4  StructuredBuffer<float>   ub
  5  RWStructuredBuffer<float> dst   (dst[dstOff] = the norm)
-/

namespace Drape.SlangCodegen.LbPgInf

open LeanSlang
open Drape.SlangCodegen.Dsl

/-- `|clamp(x[i] − g[i], lb[i], ub[i]) − x[i]|` -/
def term (i : E) : E :=
  fabs (fmin (fmax (at_ "x" i - at_ "g" i) (at_ "lb" i)) (at_ "ub" i) - at_ "x" i)

def params : SlangStructDecl := { name := "LbPgInfParams", fields := [fld "n" uT, fld "dstOff" uT] }
def globals : List SlangBinding :=
  [ paramsCB "LbPgInfParams", roF "x" 1, roF "g" 2, roF "lb" 3, roF "ub" 4, rwF "dst" 5 ]

def shader : SlangShaderModule :=
  { structs := [params]
  , groupShared := [ { name := "s_m", elemType := fT, dims := [256] } ]
  , globals := globals
  , functions :=
      [ entry 256 [gtid]
          [ let_ uT "t" (.member (v "gtid") "x")
          , let_ fT "acc" (fl 0.0)
          , let_ uT "i" (v "t")
          , while_ (lt (v "i") (p "n"))
              [ setv "acc" (fmax (v "acc") (term (v "i")))
              , setv "i" (v "i" + u 256) ]
          , setAt "s_m" (v "t") (v "acc")
          , do_ (call "GroupMemoryBarrierWithGroupSync" [])
          , let_ uT "step" (u 128)
          , while_ (gt (v "step") (u 0))
              [ if_ (lt (v "t") (v "step"))
                  [ setAt "s_m" (v "t") (fmax (at_ "s_m" (v "t")) (at_ "s_m" (v "t" + v "step"))) ]
              , do_ (call "GroupMemoryBarrierWithGroupSync" [])
              , setv "step" (.bin ">>" (v "step") (u 1)) ]
          , if_ (eq (v "t") (u 0)) [ setAt "dst" (p "dstOff") (at_ "s_m" (u 0)) ] ] ] }

-- BEGIN PIN
def expected : String :=
"struct LbPgInfParams {
  uint n;
  uint dstOff;
};

groupshared float s_m[256];

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

[shader(\"compute\")] [numthreads(256, 1, 1)]
void main(uint3 gtid : SV_GroupThreadID) {
  uint t = gtid.x;
  float acc = 0.000000;
  uint i = t;
  while ((i < params.n)) {
    acc = max(acc, abs((min(max((x[i] - g[i]), lb[i]), ub[i]) - x[i])));
    i = (i + 256u);
  }
  s_m[t] = acc;
  GroupMemoryBarrierWithGroupSync();
  uint step = 128u;
  while ((step > 0u)) {
    if ((t < step)) {
      s_m[t] = max(s_m[t], s_m[(t + step)]);
    }
    GroupMemoryBarrierWithGroupSync();
    step = (step >> 1u);
  }
  if ((t == 0u)) {
    dst[params.dstOff] = s_m[0u];
  }
}"

example : LeanSlang.emit shader = expected := by native_decide
example : shader.entryPointName = "main" := by native_decide
-- END PIN

end Drape.SlangCodegen.LbPgInf

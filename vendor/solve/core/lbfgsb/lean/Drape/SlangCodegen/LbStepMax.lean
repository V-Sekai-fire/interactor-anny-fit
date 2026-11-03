import Drape.SlangCodegen.Dsl

/-!
# `Drape.SlangCodegen.LbStepMax` — largest feasible step along d

LBFGSpp `LBFGSBSolver::max_step_size` (LBFGSB.h):
`min_i (ub−x)/d` over `d_i > 0` and `(lb−x)/d` over `d_i < 0`. LBFGSpp
starts from `+inf`; LeanSlang has no inf literal, so the start value is
FLT_MAX (`asfloat(0x7F7FFFFF)`), and a bound at or beyond 1e30 in
magnitude is absent (the host maps ±inf to ±FLT_MAX), so an
unconstrained direction returns FLT_MAX.

One workgroup of 256 threads and a `groupshared` min tree;
`LbStepMaxSerial` is the cpp-target sibling.

Bindings (set 0):

  0  ConstantBuffer<LbStepMaxParams> { uint n; uint dstOff; }
  1  StructuredBuffer<float>   x
  2  StructuredBuffer<float>   d
  3  StructuredBuffer<float>   lb
  4  StructuredBuffer<float>   ub
  5  RWStructuredBuffer<float> dst   (dst[dstOff] = the step)
-/

namespace Drape.SlangCodegen.LbStepMax

open LeanSlang
open Drape.SlangCodegen.Dsl

/-- `acc = min(acc, step_i)` for coordinate `i`. -/
def update (i : E) : List St :=
  [ let_ fT "di" (at_ "d" i)
  , if_ (and_ (gt (v "di") (fl 0.0)) (lt (at_ "ub" i) unbounded))
      [ setv "acc" (fmin (v "acc") ((at_ "ub" i - at_ "x" i) / v "di")) ]
  , if_ (and_ (lt (v "di") (fl 0.0)) (gt (at_ "lb" i) (-unbounded)))
      [ setv "acc" (fmin (v "acc") ((at_ "lb" i - at_ "x" i) / v "di")) ] ]

def params : SlangStructDecl := { name := "LbStepMaxParams", fields := [fld "n" uT, fld "dstOff" uT] }
def globals : List SlangBinding :=
  [ paramsCB "LbStepMaxParams", roF "x" 1, roF "d" 2, roF "lb" 3, roF "ub" 4, rwF "dst" 5 ]

def shader : SlangShaderModule :=
  { structs := [params]
  , groupShared := [ { name := "s_m", elemType := fT, dims := [256] } ]
  , globals := globals
  , functions :=
      [ entry 256 [gtid]
          [ let_ uT "t" (.member (v "gtid") "x")
          , let_ fT "acc" fltMax
          , let_ uT "i" (v "t")
          , while_ (lt (v "i") (p "n")) (update (v "i") ++ [ setv "i" (v "i" + u 256) ])
          , setAt "s_m" (v "t") (v "acc")
          , do_ (call "GroupMemoryBarrierWithGroupSync" [])
          , let_ uT "step" (u 128)
          , while_ (gt (v "step") (u 0))
              [ if_ (lt (v "t") (v "step"))
                  [ setAt "s_m" (v "t") (fmin (at_ "s_m" (v "t")) (at_ "s_m" (v "t" + v "step"))) ]
              , do_ (call "GroupMemoryBarrierWithGroupSync" [])
              , setv "step" (.bin ">>" (v "step") (u 1)) ]
          , if_ (eq (v "t") (u 0)) [ setAt "dst" (p "dstOff") (at_ "s_m" (u 0)) ] ] ] }

-- BEGIN PIN
def expected : String :=
"struct LbStepMaxParams {
  uint n;
  uint dstOff;
};

groupshared float s_m[256];

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

[shader(\"compute\")] [numthreads(256, 1, 1)]
void main(uint3 gtid : SV_GroupThreadID) {
  uint t = gtid.x;
  float acc = asfloat(2139095039u);
  uint i = t;
  while ((i < params.n)) {
    float di = d[i];
    if (((di > 0.000000) && (ub[i] < asfloat(1900671690u)))) {
      acc = min(acc, ((ub[i] - x[i]) / di));
    }
    if (((di < 0.000000) && (lb[i] > (-asfloat(1900671690u))))) {
      acc = min(acc, ((lb[i] - x[i]) / di));
    }
    i = (i + 256u);
  }
  s_m[t] = acc;
  GroupMemoryBarrierWithGroupSync();
  uint step = 128u;
  while ((step > 0u)) {
    if ((t < step)) {
      s_m[t] = min(s_m[t], s_m[(t + step)]);
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

end Drape.SlangCodegen.LbStepMax

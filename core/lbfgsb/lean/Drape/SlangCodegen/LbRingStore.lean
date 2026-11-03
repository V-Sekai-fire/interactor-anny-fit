import Drape.SlangCodegen.Dsl

/-!
# `Drape.SlangCodegen.LbRingStore` — s, y into ring slot `loc`

LBFGSpp `BFGSMat::add_correction` (BFGSMat.h) stores the new pair in
column `loc = m_ptr % m` of the n×m histories S and Y. Here S and Y are
column-major float buffers (`S[loc·n + i]`), and the store is gated on
the device: `lb_compact` runs first, applies the curvature test
`sᵀy > ε·yᵀy` and writes `st[2] = loc`, `st[3] = ok`. A rejected pair
leaves the ring untouched, as in LBFGSpp, with no readback.

Bindings (set 0):

  0  ConstantBuffer<LbRingStoreParams> { uint n; }
  1  StructuredBuffer<uint>    st     (lb_compact state; see Common)
  2  StructuredBuffer<float>   s
  3  StructuredBuffer<float>   y
  4  RWStructuredBuffer<float> S      (n × mcap)
  5  RWStructuredBuffer<float> Y      (n × mcap)
-/

namespace Drape.SlangCodegen.LbRingStore

open LeanSlang
open Drape.SlangCodegen.Dsl

def shader : SlangShaderModule :=
  { structs := [ { name := "LbRingStoreParams", fields := [fld "n" uT] } ]
  , globals := [ paramsCB "LbRingStoreParams", roU "st" 1, roF "s" 2, roF "y" 3, rwF "S" 4, rwF "Y" 5 ]
  , functions :=
      [ entry 256 [dtid]
          [ let_ uT "i" (.member (v "tid") "x")
          , if_ (ge (v "i") (p "n")) [ ret ]
          , if_ (eq (at_ "st" (u 3)) (u 0)) [ ret ]
          , let_ uT "k" (at_ "st" (u 2) * p "n" + v "i")
          , setAt "S" (v "k") (at_ "s" (v "i"))
          , setAt "Y" (v "k") (at_ "y" (v "i")) ] ] }

-- BEGIN PIN
def expected : String :=
"struct LbRingStoreParams {
  uint n;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LbRingStoreParams> params;
[[vk::binding(1, 0)]]
StructuredBuffer<uint> st;
[[vk::binding(2, 0)]]
StructuredBuffer<float> s;
[[vk::binding(3, 0)]]
StructuredBuffer<float> y;
[[vk::binding(4, 0)]]
RWStructuredBuffer<float> S;
[[vk::binding(5, 0)]]
RWStructuredBuffer<float> Y;

[shader(\"compute\")] [numthreads(256, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint i = tid.x;
  if ((i >= params.n)) {
    return;
  }
  if ((st[3u] == 0u)) {
    return;
  }
  uint k = ((st[2u] * params.n) + i);
  S[k] = s[i];
  Y[k] = y[i];
}"

example : LeanSlang.emit shader = expected := by native_decide
example : shader.entryPointName = "main" := by native_decide
-- END PIN

end Drape.SlangCodegen.LbRingStore

import Drape.SlangCodegen.LbMultiDot

/-!
# `Drape.SlangCodegen.LbMultiDotSerial` — `LbMultiDot`, one thread per column

The cpp-target sibling of `LbMultiDot`: same bindings and output layout;
thread j (`SV_DispatchThreadID.x`) accumulates column j over n in df32,
left to right.
-/

namespace Drape.SlangCodegen.LbMultiDotSerial

open LeanSlang
open Drape.SlangCodegen.Dsl
open Drape.SlangCodegen.Common

def shader : SlangShaderModule :=
  { structs := [LbMultiDot.params]
  , globals := LbMultiDot.globals
  , functions := dfHelpers ++
      [ entry 64 [dtid]
          [ let_ uT "j" (.member (v "tid") "x")
          , if_ (ge (v "j") (p "ncols")) [ ret ]
          , let_ fT "acc_hi" (fl 0.0)
          , let_ fT "acc_lo" (fl 0.0)
          , for_ "i" (u 0) (p "n") [ LbMultiDot.accStmt (v "j") (v "i") ]
          , setAt "dst" (p "dstOff" + u 2 * v "j") (v "acc_hi")
          , setAt "dst" (p "dstOff" + u 2 * v "j" + u 1) (v "acc_lo") ] ] }

-- BEGIN PIN
def expected : String :=
"struct LbMultiDotParams {
  uint n;
  uint ncols;
  uint aOff;
  uint aStride;
  uint vOff;
  uint dstOff;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LbMultiDotParams> params;
[[vk::binding(1, 0)]]
StructuredBuffer<float> a;
[[vk::binding(2, 0)]]
StructuredBuffer<float> vv;
[[vk::binding(3, 0)]]
RWStructuredBuffer<float> dst;

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

[shader(\"compute\")] [numthreads(64, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint j = tid.x;
  if ((j >= params.ncols)) {
    return;
  }
  float acc_hi = 0.000000;
  float acc_lo = 0.000000;
  for (uint i = 0u; i < params.n; ++i) {
    df_acc(acc_hi, acc_lo, a[((params.aOff + (j * params.aStride)) + i)], vv[(params.vOff + i)]);
  }
  dst[(params.dstOff + (2u * j))] = acc_hi;
  dst[((params.dstOff + (2u * j)) + 1u)] = acc_lo;
}"

example : LeanSlang.emit shader = expected := by native_decide
example : shader.entryPointName = "main" := by native_decide
-- END PIN

end Drape.SlangCodegen.LbMultiDotSerial

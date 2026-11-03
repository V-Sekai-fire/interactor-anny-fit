import Drape.SlangCodegen.Common

/-!
# `Drape.SlangCodegen.LbMultiDot` — `Aᵀv` for column-major A, df32

The products BFGSMat keeps up to date on every `add_correction`:
`Sᵀs` (a row of SᵀS), `Yᵀs` (a row of SᵀY), and the new pair's own
`sᵀy`, `yᵀy`, `sᵀs`. Column j of A starts at `aOff + j·aStride`;
one workgroup per column (`SV_GroupID.x = j`), 256 threads grid-strided
over n with df32 accumulation, then the same df32 tree as
`Cloth.SlangCodegen.DotReduce`. The result is a `(hi, lo)` pair at
`dst[dstOff + 2j]`, `dst[dstOff + 2j + 1]`.

`slangc -target cpp` rejects the barrier; `LbMultiDotSerial` is the
cpp-target sibling (one thread per column).

Bindings (set 0):

  0  ConstantBuffer<LbMultiDotParams>
       { uint n; uint ncols; uint aOff; uint aStride; uint vOff; uint dstOff; }
  1  StructuredBuffer<float>   a
  2  StructuredBuffer<float>   vv
  3  RWStructuredBuffer<float> dst
-/

namespace Drape.SlangCodegen.LbMultiDot

open LeanSlang
open Drape.SlangCodegen.Dsl
open Drape.SlangCodegen.Common

def params : SlangStructDecl :=
  { name := "LbMultiDotParams"
  , fields := [fld "n" uT, fld "ncols" uT, fld "aOff" uT, fld "aStride" uT, fld "vOff" uT, fld "dstOff" uT] }
def globals : List SlangBinding :=
  [ paramsCB "LbMultiDotParams", roF "a" 1, roF "vv" 2, rwF "dst" 3 ]

/-- `acc += a[aOff + j·aStride + i] · vv[vOff + i]` -/
def accStmt (j i : E) : St :=
  do_ (call "df_acc" [v "acc_hi", v "acc_lo",
    at_ "a" (p "aOff" + j * p "aStride" + i), at_ "vv" (p "vOff" + i)])

def shader : SlangShaderModule :=
  { structs := [params]
  , groupShared :=
      [ { name := "s_hi", elemType := fT, dims := [256] }
      , { name := "s_lo", elemType := fT, dims := [256] } ]
  , globals := globals
  , functions := dfHelpers ++
      [ entry 256 [gtid, gid]
          [ let_ uT "t" (.member (v "gtid") "x")
          , let_ uT "j" (.member (v "gid") "x")
          , let_ fT "acc_hi" (fl 0.0)
          , let_ fT "acc_lo" (fl 0.0)
          , if_ (lt (v "j") (p "ncols"))
              [ let_ uT "i" (v "t")
              , while_ (lt (v "i") (p "n")) [ accStmt (v "j") (v "i"), setv "i" (v "i" + u 256) ] ]
          , setAt "s_hi" (v "t") (v "acc_hi")
          , setAt "s_lo" (v "t") (v "acc_lo")
          , do_ (call "GroupMemoryBarrierWithGroupSync" [])
          , let_ uT "step" (u 128)
          , while_ (gt (v "step") (u 0))
              [ if_ (lt (v "t") (v "step"))
                  [ decl fT "n_hi"
                  , decl fT "n_lo"
                  , do_ (call "df_add" [at_ "s_hi" (v "t"), at_ "s_lo" (v "t"),
                      at_ "s_hi" (v "t" + v "step"), at_ "s_lo" (v "t" + v "step"), v "n_hi", v "n_lo"])
                  , setAt "s_hi" (v "t") (v "n_hi")
                  , setAt "s_lo" (v "t") (v "n_lo") ]
              , do_ (call "GroupMemoryBarrierWithGroupSync" [])
              , setv "step" (.bin ">>" (v "step") (u 1)) ]
          , if_ (and_ (eq (v "t") (u 0)) (lt (v "j") (p "ncols")))
              [ setAt "dst" (p "dstOff" + u 2 * v "j") (at_ "s_hi" (u 0))
              , setAt "dst" (p "dstOff" + u 2 * v "j" + u 1) (at_ "s_lo" (u 0)) ] ] ] }

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

groupshared float s_hi[256];
groupshared float s_lo[256];

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

[shader(\"compute\")] [numthreads(256, 1, 1)]
void main(uint3 gtid : SV_GroupThreadID, uint3 gid : SV_GroupID) {
  uint t = gtid.x;
  uint j = gid.x;
  float acc_hi = 0.000000;
  float acc_lo = 0.000000;
  if ((j < params.ncols)) {
    uint i = t;
    while ((i < params.n)) {
      df_acc(acc_hi, acc_lo, a[((params.aOff + (j * params.aStride)) + i)], vv[(params.vOff + i)]);
      i = (i + 256u);
    }
  }
  s_hi[t] = acc_hi;
  s_lo[t] = acc_lo;
  GroupMemoryBarrierWithGroupSync();
  uint step = 128u;
  while ((step > 0u)) {
    if ((t < step)) {
      float n_hi;
      float n_lo;
      df_add(s_hi[t], s_lo[t], s_hi[(t + step)], s_lo[(t + step)], n_hi, n_lo);
      s_hi[t] = n_hi;
      s_lo[t] = n_lo;
    }
    GroupMemoryBarrierWithGroupSync();
    step = (step >> 1u);
  }
  if (((t == 0u) && (j < params.ncols))) {
    dst[(params.dstOff + (2u * j))] = s_hi[0u];
    dst[((params.dstOff + (2u * j)) + 1u)] = s_lo[0u];
  }
}"

example : LeanSlang.emit shader = expected := by native_decide
example : shader.entryPointName = "main" := by native_decide
-- END PIN

end Drape.SlangCodegen.LbMultiDot

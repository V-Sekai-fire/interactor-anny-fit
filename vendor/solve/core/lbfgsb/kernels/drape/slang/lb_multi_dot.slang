struct LbMultiDotParams {
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

[shader("compute")] [numthreads(256, 1, 1)]
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
}

struct DotReduceParams {
  uint n;
};

groupshared float s_hi[256];
groupshared float s_lo[256];

[[vk::binding(0, 0)]]
ConstantBuffer<DotReduceParams> params;
[[vk::binding(1, 0)]]
StructuredBuffer<float> a;
[[vk::binding(2, 0)]]
StructuredBuffer<float> b;
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

[shader("compute")] [numthreads(256, 1, 1)]
void main(uint3 tid : SV_GroupThreadID) {
  uint t = tid.x;
  uint stride = 256u;
  float acc_hi = 0.000000;
  float acc_lo = 0.000000;
  uint i = t;
  while ((i < params.n)) {
    float p_hi;
    float p_lo;
    two_prod(a[i], b[i], p_hi, p_lo);
    float new_hi;
    float new_lo;
    df_add(acc_hi, acc_lo, p_hi, p_lo, new_hi, new_lo);
    acc_hi = new_hi;
    acc_lo = new_lo;
    i = (i + stride);
  }
  s_hi[t] = acc_hi;
  s_lo[t] = acc_lo;
  GroupMemoryBarrierWithGroupSync();
  uint step = 128u;
  while ((step > 0u)) {
    if ((t < step)) {
      float new_hi;
      float new_lo;
      df_add(s_hi[t], s_lo[t], s_hi[(t + step)], s_lo[(t + step)], new_hi, new_lo);
      s_hi[t] = new_hi;
      s_lo[t] = new_lo;
    }
    GroupMemoryBarrierWithGroupSync();
    step = (step >> 1u);
  }
  if ((t == 0u)) {
    dst[0u] = s_hi[0u];
    dst[1u] = s_lo[0u];
  }
  return;
}

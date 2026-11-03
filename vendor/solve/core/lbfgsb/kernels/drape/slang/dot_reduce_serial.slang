struct DotReduceSerialParams {
  uint n;
};

[[vk::binding(0, 0)]]
ConstantBuffer<DotReduceSerialParams> params;
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

[shader("compute")] [numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  float acc_hi = 0.000000;
  float acc_lo = 0.000000;
  for (uint i = 0u; i < params.n; ++i) {
    float p_hi;
    float p_lo;
    two_prod(a[i], b[i], p_hi, p_lo);
    float new_hi;
    float new_lo;
    df_add(acc_hi, acc_lo, p_hi, p_lo, new_hi, new_lo);
    acc_hi = new_hi;
    acc_lo = new_lo;
  }
  dst[0u] = acc_hi;
  dst[1u] = acc_lo;
  return;
}

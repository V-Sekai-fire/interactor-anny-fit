struct LbStepMaxParams {
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

[shader("compute")] [numthreads(1, 1, 1)]
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
}

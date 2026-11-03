struct LbPgInfParams {
  uint n;
  uint dstOff;
};

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

[shader("compute")] [numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  float acc = 0.000000;
  for (uint i = 0u; i < params.n; ++i) {
    acc = max(acc, abs((min(max((x[i] - g[i]), lb[i]), ub[i]) - x[i])));
  }
  dst[params.dstOff] = acc;
}

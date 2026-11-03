struct LbPgInfParams {
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

[shader("compute")] [numthreads(256, 1, 1)]
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
}

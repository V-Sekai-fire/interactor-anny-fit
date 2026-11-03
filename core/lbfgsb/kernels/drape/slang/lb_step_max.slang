struct LbStepMaxParams {
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

[shader("compute")] [numthreads(256, 1, 1)]
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
}

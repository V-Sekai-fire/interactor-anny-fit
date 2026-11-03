struct LbBoxProjectParams {
  uint n;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LbBoxProjectParams> params;
[[vk::binding(1, 0)]]
RWStructuredBuffer<float> x;
[[vk::binding(2, 0)]]
StructuredBuffer<float> lb;
[[vk::binding(3, 0)]]
StructuredBuffer<float> ub;

[shader("compute")] [numthreads(256, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint i = tid.x;
  if ((i >= params.n)) {
    return;
  }
  x[i] = min(max(x[i], lb[i]), ub[i]);
}

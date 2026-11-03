struct LbRingStoreParams {
  uint n;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LbRingStoreParams> params;
[[vk::binding(1, 0)]]
StructuredBuffer<uint> st;
[[vk::binding(2, 0)]]
StructuredBuffer<float> s;
[[vk::binding(3, 0)]]
StructuredBuffer<float> y;
[[vk::binding(4, 0)]]
RWStructuredBuffer<float> S;
[[vk::binding(5, 0)]]
RWStructuredBuffer<float> Y;

[shader("compute")] [numthreads(256, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint i = tid.x;
  if ((i >= params.n)) {
    return;
  }
  if ((st[3u] == 0u)) {
    return;
  }
  uint k = ((st[2u] * params.n) + i);
  S[k] = s[i];
  Y[k] = y[i];
}

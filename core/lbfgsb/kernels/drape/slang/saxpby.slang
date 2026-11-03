struct SaxpbyParams {
  uint n;
  float alpha;
  float beta;
};

[[vk::binding(0, 0)]]
ConstantBuffer<SaxpbyParams> params;
[[vk::binding(1, 0)]]
StructuredBuffer<float> x;
[[vk::binding(2, 0)]]
StructuredBuffer<float> y;
[[vk::binding(3, 0)]]
RWStructuredBuffer<float> dst;

[shader("compute")] [numthreads(256, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint i = tid.x;
  if ((i >= params.n)) {
    return;
  }
  dst[i] = fma(params.alpha, x[i], (params.beta * y[i]));
}

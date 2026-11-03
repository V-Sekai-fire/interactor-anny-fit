struct LbApplyMParams {
  uint mcap;
  uint inOff;
  uint outOff;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LbApplyMParams> params;
[[vk::binding(1, 0)]]
StructuredBuffer<uint> st;
[[vk::binding(2, 0)]]
StructuredBuffer<float> bf;
[[vk::binding(3, 0)]]
StructuredBuffer<float> vin;
[[vk::binding(4, 0)]]
RWStructuredBuffer<float> vout;

void lb_mv(uint nc, uint mc, inout float w[32]) {
  float r[16];
  for (uint i = 0u; i < nc; ++i) {
    float acc = w[(nc + i)];
    for (uint j = 0u; j < nc; ++j) {
      acc = (acc + ((bf[(((4u + ((2u * mc) * mc)) + (i * mc)) + j)] * w[j]) / bf[((4u + ((4u * mc) * mc)) + j)]));
    }
    r[i] = acc;
  }
  for (uint fi = 0u; fi < nc; ++fi) {
    float fr = r[fi];
    for (uint ft = 0u; ft < fi; ++ft) {
      fr = (fr - (bf[(((4u + ((3u * mc) * mc)) + (fi * mc)) + ft)] * r[ft]));
    }
    r[fi] = (fr / bf[(((4u + ((3u * mc) * mc)) + (fi * mc)) + fi)]);
  }
  for (uint bq = 0u; bq < nc; ++bq) {
    uint bi = ((nc - 1u) - bq);
    float br = r[bi];
    for (uint bt = (bi + 1u); bt < nc; ++bt) {
      br = (br - (bf[(((4u + ((3u * mc) * mc)) + (bt * mc)) + bi)] * r[bt]));
    }
    r[bi] = (br / bf[(((4u + ((3u * mc) * mc)) + (bi * mc)) + bi)]);
  }
  for (uint j = 0u; j < nc; ++j) {
    float acc = (-w[j]);
    for (uint i = 0u; i < nc; ++i) {
      acc = (acc + (bf[(((4u + ((2u * mc) * mc)) + (i * mc)) + j)] * r[i]));
    }
    w[j] = (acc / bf[((4u + ((4u * mc) * mc)) + j)]);
  }
  for (uint i = 0u; i < nc; ++i) {
    w[(nc + i)] = r[i];
  }
  return;
}

[shader("compute")] [numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint nc = st[0u];
  if ((nc == 0u)) {
    return;
  }
  float w[32];
  for (uint j = 0u; j < (2u * nc); ++j) {
    w[j] = vin[(params.inOff + j)];
  }
  lb_mv(nc, params.mcap, w);
  for (uint j = 0u; j < (2u * nc); ++j) {
    vout[(params.outOff + j)] = w[j];
  }
}
